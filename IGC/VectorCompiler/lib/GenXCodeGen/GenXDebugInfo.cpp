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
#include "GenXBackendConfig.h"
#include "GenXTargetMachine.h"

#include "visa/include/visaBuilder_interface.h"

#include "DebugInfo/StreamEmitter.hpp"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "DebugInfo/VISAModule.hpp"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/InitializePasses.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/Error.h>

#include "Probe/Assertion.h"

#define DEBUG_TYPE "GENX_DEBUG_INFO"

using namespace llvm;

namespace {

static void debugDump(const Twine &Name, const char *Content, size_t Size) {
  std::error_code EC;
  // no error handling since this is debug output
  raw_fd_ostream OS(Name.str(), EC);
  OS << StringRef(Content, Size);
}

class VisaKernelInfo {

  using FinalizedDI = IGC::DbgDecoder::DbgInfoFormat;

  std::vector<char> GenBinary;
  std::vector<char> DbgInfoBlob;
  std::unique_ptr<IGC::DbgDecoder> DecodedDebugInfo;

  FINALIZER_INFO *JitInfo = nullptr;
  // underlying data is owned by DecodedDebugInfo
  const FinalizedDI *VisaKernelDI = nullptr;

  std::string ErrMsg;

public:
  const FINALIZER_INFO &getJitInfo() const {
    IGC_ASSERT(ErrMsg.empty() && JitInfo);
    return *JitInfo;
  };
  const FinalizedDI &getFinalizerDI() const {
    IGC_ASSERT(ErrMsg.empty() && VisaKernelDI);
    return *VisaKernelDI;
  }
  IGC::DbgDecoder *getDIDecoder() const { return DecodedDebugInfo.get(); }
  const std::vector<char> &getGenBinary() const { return GenBinary; }
  const std::vector<char> &getDbgInfoBlob() const { return DbgInfoBlob; }
  const std::string &getError() const { return ErrMsg; }
  bool hasErrors() const { return !getError().empty(); }

  struct Gen2VisaIdx {
    unsigned GenOffset;
    unsigned VisaIdx;
  };

  VisaKernelInfo(const Function &F, const VISAKernel &VK) {
    void *GenXdbgInfo = nullptr;
    void *VISAMap = nullptr;
    unsigned int DbgSize = 0;
    unsigned int NumElems = 0;
    if (VK.GetJitInfo(JitInfo) != 0) {
      ErrMsg = "could not extract jitter info";
      return;
    }
    IGC_ASSERT(JitInfo);

    if (VK.GetGenxDebugInfo(GenXdbgInfo, DbgSize, VISAMap, NumElems) != 0) {
      ErrMsg = "visa info decode error";
      return;
    }
    if (VISAMap)
      freeBlock(VISAMap);
    IGC_ASSERT(GenXdbgInfo);

    const char *DbgBlobBytes = static_cast<const char *>(GenXdbgInfo);
    DbgInfoBlob = std::vector<char>(DbgBlobBytes, DbgBlobBytes + DbgSize);
    freeBlock(GenXdbgInfo);

    DecodedDebugInfo = std::make_unique<IGC::DbgDecoder>(DbgInfoBlob.data());
    auto GetDebugInfoForKernel = [this](StringRef KernelName) {
      const auto &CO = DecodedDebugInfo->compiledObjs;
      auto FoundIt =
          std::find_if(CO.begin(), CO.end(), [&KernelName](const auto &DI) {
            return KernelName == StringRef(DI.kernelName);
          });
      const IGC::DbgDecoder::DbgInfoFormat *Result = nullptr;
      if (FoundIt != CO.end())
        Result = &*FoundIt;
      return Result;
    };
    VisaKernelDI = GetDebugInfoForKernel(F.getName());
    if (!VisaKernelDI) {
      ErrMsg = "could not find debug information for <" +
               std::string(F.getName()) + ">";
      return;
    }
    if (VisaKernelDI->CISAIndexMap.empty()) {
      ErrMsg = "empty CisaIndexMap for <" + std::string(F.getName()) + ">";
      return;
    }

    std::vector<Gen2VisaIdx> Gen2Visa;
    LLVM_DEBUG(dbgs() << "\nCISA Index Map:\n---\n");
    for (const auto &Item : VisaKernelDI->CISAIndexMap) {
      auto VisaIndex = Item.first;
      auto GenOff = Item.second;
      Gen2Visa.push_back({GenOff, VisaIndex});
      LLVM_DEBUG(dbgs() << "GI: " << GenOff << " -> VI:" << VisaIndex << "\n");
    }
    LLVM_DEBUG(dbgs() << "---\n");

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
          // <= Is because last index can be equal to the binary size
          return Idx.GenOffset <= GenBinary.size();
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
};

class GenXVisaModule final : public IGC::VISAModule {

public:
  GenXVisaModule(const Function &F, const GenXSubtarget &STIn,
                 const genx::di::VisaMapping &V2I, const VisaKernelInfo &VKI)
      : F{F}, ST{STIn}, VisaMapping{V2I}, VisaKernelInfo{VKI},
        VISAModule(const_cast<Function *>(&F)) {

    isDirectElfInput = true;
  }

  unsigned int getUnpaddedProgramSize() const override {
    return VisaKernelInfo.getGenBinary().size();
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
    return ST.getGRFWidth();
  }
  unsigned getNumGRFs() const override {
    return VisaKernelInfo.getJitInfo().numGRFTotal;
  }
  unsigned getPointerSize() const override {
    return F.getParent()->getDataLayout().getPointerSize();
  }
  ArrayRef<char> getGenDebug() const override {
    return VisaKernelInfo.getDbgInfoBlob();
  }
  ArrayRef<char> getGenBinary() const override {
    return VisaKernelInfo.getGenBinary();
  }
  std::vector<IGC::VISAVariableLocation>
  GetVariableLocation(const Instruction *pInst) const override {
    return {};
  }

  VISAModule *makeNew() const override {
    return new GenXVisaModule(F, ST, VisaMapping, VisaKernelInfo);
  }
  void UpdateVisaId() override {
    // do nothing (the moment we need to advance index is controlled explicitly)
  }
  void ValidateVisaId() override {
    // do nothing (we don't need validation since VISA is built already)
  }
  uint16_t GetSIMDSize() const override { return 1; }

private:
  const Function &F;
  const GenXSubtarget &ST;
  const genx::di::VisaMapping &VisaMapping;
  const VisaKernelInfo &VisaKernelInfo;
};

} // namespace

namespace llvm {

void GenXDebugInfo::processKernel(const Function &KF, const VISAKernel &VK,
                                  const genx::di::VisaMapping &VisaMapping) {

  VisaKernelInfo GenInfo(KF, VK);
  if (GenInfo.hasErrors())
    report_fatal_error(GenInfo.getError(), false);

  IGC_ASSERT(DebugInfo.count(&KF) == 0);
  raw_svector_ostream OS(DebugInfo[&KF]);

  IGC::DebugEmitterOpts DebugOpts;
  DebugOpts.isDirectElf = true;

  auto Deleter = [](IGC::IDebugEmitter *Emitter) {
    IGC::IDebugEmitter::Release(Emitter);
  };
  using EmitterHolder = std::unique_ptr<IGC::IDebugEmitter, decltype(Deleter)>;
  EmitterHolder Emitter(IGC::IDebugEmitter::Create(), Deleter);

  const auto &ST = getAnalysis<TargetPassConfig>()
      .getTM<GenXTargetMachine>()
      .getGenXSubtarget();
  // Onwership of GenXVisaModule object is transfered to IDebugEmitter
  auto *VM = new GenXVisaModule(KF, ST, VisaMapping, GenInfo);
  Emitter->Initialize(VM, DebugOpts, true);

  Emitter->AddVISAModFunc(VM, const_cast<Function *>(&KF));
  Emitter->SetVISAModule(VM);
  Emitter->setFunction(const_cast<Function *>(&KF), false /*cloned*/);

  IGC_ASSERT(!GenInfo.getFinalizerDI().CISAIndexMap.empty());
  LLVM_DEBUG(dbgs() << "\nV2I Dump\n----\n");
  const auto &V2I = VisaMapping.V2I;
  for (auto MappingIt = V2I.cbegin(); MappingIt != V2I.cend(); ++MappingIt) {

    auto NextMapping = std::next(MappingIt);
    auto VisaIndexCurr = MappingIt->first;
    auto VisaIndexNext =
        (NextMapping != V2I.cend())
            ? NextMapping->first
            : GenInfo.getFinalizerDI().CISAIndexMap.back().first;

    // Note: "index - 1" is becuse we mimic index values as if they were
    // before corresponding instructions were inserted
    VM->SetVISAId(VisaIndexCurr - 1);
    // we need this const_cast because of the flawed VISA Emitter flawed API
    auto *Inst = const_cast<Instruction *>(MappingIt->second);
    Emitter->BeginInstruction(Inst);
    VM->SetVISAId(VisaIndexNext - 1);
    Emitter->EndInstruction(Inst);

    LLVM_DEBUG(dbgs() << "VisaMap: [" << VisaIndexCurr << ";" << VisaIndexNext
                      << "):" << *Inst << "\n");
  }
  LLVM_DEBUG(dbgs() << "\n");

  const bool FinalizeDebugInfo = true;
  const auto &ElfBin =
      Emitter->Finalize(FinalizeDebugInfo, GenInfo.getDIDecoder());
  if (ElfBin.empty())
    report_fatal_error("could not emit .elf image");

  OS.write(ElfBin.data(), ElfBin.size());

  const auto &KernelName = KF.getName();
  LLVM_DEBUG(dbgs() << "got Debug Info for <" << KernelName << "> "
                    << "- " << ElfBin.size() << " bytes\n");

  const auto &BC = getAnalysis<GenXBackendConfig>();
  if (BC.dbgInfoDumpsEnabled()) {
    StringRef NameSuffix = KernelName;
    if (!BC.dbgInfoDumpsNameOverride().empty())
      NameSuffix = BC.dbgInfoDumpsNameOverride();

    auto DwarfDumpName = ("dbginfo_" + NameSuffix + "_dwarf.elf").str();
    auto GendbgDumpName = ("dbginfo_" + NameSuffix + "_gen.dump").str();
    if (BC.hasShaderDumper()) {
      BC.getShaderDumper().dumpBinary(ElfBin, DwarfDumpName);
      BC.getShaderDumper().dumpBinary(GenInfo.getDbgInfoBlob(), GendbgDumpName);
    } else {
      debugDump(DwarfDumpName, ElfBin.data(), ElfBin.size());
      debugDump(GendbgDumpName, GenInfo.getDbgInfoBlob().data(),
                GenInfo.getDbgInfoBlob().size());
    }
  }
  return;
}

void GenXDebugInfo::cleanup() { DebugInfo.clear(); }

void GenXDebugInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<GenXModule>();
  AU.addRequired<TargetPassConfig>();
  AU.setPreservesAll();
}
bool GenXDebugInfo::runOnModule(Module &M) {

  const auto &BC = getAnalysis<GenXBackendConfig>();
  if (!BC.kernelDebugEnabled())
    return false;

  const FunctionGroupAnalysis &FGA = getAnalysis<FunctionGroupAnalysis>();
  auto &GM = getAnalysis<GenXModule>();

  VISABuilder *VB = GM.GetCisaBuilder();
  if (GM.HasInlineAsm())
    VB = GM.GetVISAAsmReader();

  for (const auto *FG : FGA) {
    const auto *KF = FG->getHead();

    const genx::di::VisaMapping &VisaDbgInfo = *GM.getVisaMapping(KF);
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
INITIALIZE_PASS_DEPENDENCY(FunctionGroupAnalysis)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_DEPENDENCY(GenXModule)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(GenXDebugInfo, "GenXDebugInfo", "GenXDebugInfo", false,
                    true /*analysis*/)


