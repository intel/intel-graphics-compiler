/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "GenXDebugInfo.h"
#include "FunctionGroup.h"
#include "GenXTargetMachine.h"

#include "visa/include/visaBuilder_interface.h"
#include "visa/common.h"

#include "DebugInfo/StreamEmitter.hpp"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "DebugInfo/VISAModule.hpp"

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Function.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/CommandLine.h>
#include "Probe/Assertion.h"

#define DEBUG_TYPE "GENX_DEBUG_INFO"

using namespace llvm;

static cl::opt<bool> GenerateDebugInfo(
    "emit-debug-info", cl::init(false), cl::Hidden,
    cl::desc("Generate DWARF debug info for each compiled kernel"));

static cl::opt<bool> DebugInfoDumpGendbg(
    "debug-info-dump-gendbg", cl::init(false), cl::Hidden,
    cl::desc("Dump raw gendbg .dump file produced by finalizer"));
static cl::opt<std::string> DebugInfoGendbgName("deubg-info-gendbg-output-name",
                                                cl::Hidden);

static cl::opt<bool>
    DebugInfoDumpsElf("debug-info-dump-elf", cl::init(false), cl::Hidden,
                      cl::desc("Dump raw elf file containing debug info"));
static cl::opt<std::string> DebugInfoElfName("debug-info-elf-output-name",
                                             cl::Hidden);

namespace {

static void debugDump(const Twine &Name, const char *Content, size_t Size) {
  std::error_code EC;
  // no error handling since this is debug output
  raw_fd_ostream OS(Name.str(), EC);
  OS << StringRef(Content, Size);
}

struct FinalizerDbgInfo {

  struct Gen2VisaIdx {
    unsigned GenOffset;
    unsigned VisaIdx;
  };

  FinalizerDbgInfo(const Function &F, const VISAKernel &VK) {
    void *GenXdbgInfo = nullptr;
    void *VISAMap = nullptr;
    unsigned int DbgSize = 0;
    unsigned int NumElems = 0;
    if (const_cast<VISAKernel &>(VK).GetGenxDebugInfo(
            GenXdbgInfo, DbgSize, VISAMap, NumElems) != VISA_SUCCESS) {
      ErrMsg = "visa info decode error";
      return;
    }
    IGC_ASSERT(GenXdbgInfo);

    const char *DbgBlobBytes = static_cast<const char *>(GenXdbgInfo);
    BinaryDump = std::vector<char>(DbgBlobBytes, DbgBlobBytes + DbgSize);

    DecodedInfo = std::make_unique<IGC::DbgDecoder>(GenXdbgInfo);

    std::vector<Gen2VisaIdx> Gen2Visa;
    if (NumElems > 0 && VISAMap) {
      LLVM_DEBUG(dbgs() << "---\n");

      unsigned int *VisaMapUI = reinterpret_cast<unsigned int *>(VISAMap);
      for (unsigned int i = 0; i < NumElems * 2; i += 2) {
        auto GenISAOffset = VisaMapUI[i];
        auto VISAIndex = VisaMapUI[i + 1];
        Gen2Visa.push_back({GenISAOffset, VISAIndex});
        LLVM_DEBUG(dbgs() << "GenOffset: " << GenISAOffset
                          << " -> VisaIdx: " << VISAIndex << "\n");
      }
      LLVM_DEBUG(dbgs() << "---\n");
    }

    if (VISAMap)
      freeBlock(VISAMap);
    freeBlock(GenXdbgInfo);

    // Extract Gen Binary (will need it for line table generation)
    void *GenBin = nullptr;
    int GenBinSize = 0; // Finalizer uses signed int for size...
    VK.GetGenxBinary(GenBin, GenBinSize);
    IGC_ASSERT(GenBinSize >= 0);
    const auto *GenBinBytes = reinterpret_cast<const char *>(GenBin);
    GenBinary.insert(GenBinary.end(), GenBinBytes, GenBinBytes + GenBinSize);

    // Make Sure that gen isa indeces are inside GenBinary
    const bool InBounds =
        std::all_of(Gen2Visa.begin(), Gen2Visa.end(), [&](const auto &Idx) {
          return Idx.GenOffset < GenBinary.size();
        });
    if (!InBounds) {
      ErrMsg = "fatal error (debug info). inconsistent gen->visa mapping: "
               "gen index is out of bounds";
      return;
    }

    // Make Sure that gen isa indeces are unique and sorted
    const bool Sorted = std::is_sorted(
        Gen2Visa.begin(), Gen2Visa.end(),
        [](const auto &L, const auto &R) { return L.GenOffset < R.GenOffset; });
    const bool Validated =
        Sorted && (Gen2Visa.end() ==
                   std::adjacent_find(Gen2Visa.begin(), Gen2Visa.end(),
                                      [](const auto &L, const auto &R) {
                                        return L.GenOffset == R.GenOffset;
                                      }));
    if (!Validated) {
      ErrMsg = "fatal error (debug info). inconsistent gen->visa mapping: "
               "gen index are not ordered properly";
      return;
    }
  }

  std::unique_ptr<IGC::DbgDecoder> DecodedInfo;
  std::vector<char> GenBinary;
  std::string ErrMsg;

  std::vector<char> BinaryDump;
};

class GenXVisaModule final : public IGC::VISAModule {

public:
  GenXVisaModule(const Function &F, const genx::VisaDebugInfo &VisaDebugIn,
                 const FinalizerDbgInfo &GenDbgIn)
      : F{F}, VisaDebug{VisaDebugIn}, GenDbg{GenDbgIn},
        VISAModule(const_cast<Function *>(&F)) {

    isDirectElfInput = true;
  }

  unsigned int getUnpaddedProgramSize() const override {
    return GenDbg.GenBinary.size();
  }
  bool isLineTableOnly() const override {
    IGC_ASSERT_MESSAGE(0, "isLineTableOnly()");
    return false;
  }
  unsigned getPrivateBaseReg() const override {
    IGC_ASSERT_MESSAGE(0, "getPrivateBaseReg() - not implemented");
    return 0;
  }
  unsigned getGRFSize() const override {
    IGC_ASSERT_MESSAGE(0, "getGRFSize() - not implemented");
    return 0;
  }

  unsigned getPointerSize() const override {
    return F.getParent()->getDataLayout().getPointerSize();
  }

  ArrayRef<char> getGenDebug() const override { return GenDbg.BinaryDump; }
  ArrayRef<char> getGenBinary() const override { return GenDbg.GenBinary; }
  std::vector<IGC::VISAVariableLocation>
  GetVariableLocation(const Instruction *pInst) const override {
    return {};
  }

  VISAModule *makeNew() const override {
    return new GenXVisaModule(F, VisaDebug, GenDbg);
  }
  void UpdateVisaId() override {
    SetVISAId(GetCurrentVISAId() + 1);
    LLVM_DEBUG(dbgs() << "updateVisaId() called, CurrentID = "
                      << GetCurrentVISAId() << "\n");
  }
  void ValidateVisaId() override {
    LLVM_DEBUG(dbgs() << "validateIsID() called, CurrentID = "
                      << GetCurrentVISAId() << "\n");
  }
  uint16_t GetSIMDSize() const override { return 1; }

private:
  const Function &F;
  const genx::VisaDebugInfo &VisaDebug;
  const FinalizerDbgInfo &GenDbg;
};

} // namespace

namespace llvm {

void GenXDebugInfo::processKernel(const Function &KF, const VISAKernel &VK,
                                  const genx::VisaDebugInfo &DbgInfo) {

  FinalizerDbgInfo GenDbg(KF, VK);
  if (!GenDbg.ErrMsg.empty())
    report_fatal_error(GenDbg.ErrMsg);

  IGC_ASSERT(DebugInfo.count(&KF) == 0);
  raw_svector_ostream OS(DebugInfo[&KF]);

  IGC::DebugEmitterOpts DebugOpts;
  DebugOpts.isDirectElf = true;

  auto Deleter = [](IGC::IDebugEmitter *Emitter) {
    IGC::IDebugEmitter::Release(Emitter);
  };
  using EmitterHolder = std::unique_ptr<IGC::IDebugEmitter, decltype(Deleter)>;
  EmitterHolder Emitter(IGC::IDebugEmitter::Create(), Deleter);
  // Onwership of GenXVisaModule object is transfered to IDebugEmitter
  auto *VM = new GenXVisaModule(KF, DbgInfo, GenDbg);
  Emitter->Initialize(VM, DebugOpts, true);

  Emitter->AddVISAModFunc(VM, const_cast<Function *>(&KF));
  Emitter->SetVISAModule(VM);
  Emitter->setFunction(const_cast<Function *>(&KF), false /*cloned*/);

  for (const auto &Item : DbgInfo.Locations) {
    VM->SetVISAId(Item.first);
    Emitter->BeginInstruction((Instruction *)Item.second);
    Emitter->EndInstruction((Instruction *)Item.second);
  }

  const auto &ElfBin = Emitter->Finalize(true /* really finalize :)*/);
  if (ElfBin.empty())
    report_fatal_error("could not emit .elf image");

  OS.write(ElfBin.data(), ElfBin.size());

  const auto &KernelName = KF.getName();
  LLVM_DEBUG(dbgs() << "got Debug Info for <" << KernelName << "> "
                    << "- " << ElfBin.size() << " bytes\n");
  if (DebugInfoDumpsElf) {
    std::string DumpName = DebugInfoElfName.empty()
                               ? ("dbg_" + KernelName + ".elf").str()
                               : DebugInfoElfName;
    debugDump(DumpName, ElfBin.data(), ElfBin.size());
  }
  if (DebugInfoDumpGendbg) {
    std::string DumpName = DebugInfoGendbgName.empty()
                               ? ("gendbg_" + KernelName + ".dump").str()
                               : DebugInfoGendbgName;
    debugDump(DumpName, GenDbg.BinaryDump.data(), GenDbg.BinaryDump.size());
  }
  return;
}

void GenXDebugInfo::cleanup() { DebugInfo.clear(); }

void GenXDebugInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addRequired<GenXModule>();
  AU.setPreservesAll();
}
bool GenXDebugInfo::runOnModule(Module &M) {

  if (!GenerateDebugInfo)
    return false;

  const FunctionGroupAnalysis &FGA = getAnalysis<FunctionGroupAnalysis>();
  auto &GM = getAnalysis<GenXModule>();

  VISABuilder *VB = GM.GetCisaBuilder();
  if (GM.HasInlineAsm())
    VB = GM.GetVISAAsmReader();

  for (const auto *FG : FGA) {
    const auto *KF = FG->getHead();

    const genx::VisaDebugInfo &VisaDbgInfo = *GM.getVisaDebugInfo(KF);
    VISAKernel *VK = VB->GetVISAKernel(KF->getName().str());
    IGC_ASSERT_MESSAGE(VK, "Kernel is null");

    processKernel(*KF, *VK, VisaDbgInfo);
  }
  return false;
}

char GenXDebugInfo::ID = 0;

ModulePass *createGenXDebugInfoPass() {
  initializeGenXDebugInfoPass(*PassRegistry::getPassRegistry());
  return new GenXDebugInfo;
}

} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXDebugInfo, "GenXDebugInfo", "GenXDebugInfo", false,
                      true /*analysis*/)
INITIALIZE_PASS_END(GenXDebugInfo, "GenXDebugInfo", "GenXDebugInfo", false,
                    true /*analysis*/)


