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

#include "GenX.h"

#include "common.h"
#include "common/igc_regkeys.hpp"

#include "visaBuilder_interface.h"

#include "Compiler/DebugInfo/StreamEmitter.hpp"
#include "Compiler/DebugInfo/VISAIDebugEmitter.hpp"

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Errc.h>
#include <llvm/Support/Error.h>
#include "Probe/Assertion.h"
#define DEBUG_TYPE "GENX_DEBUG_INFO"

namespace {

class DbgInfoError final : public llvm::ErrorInfo<DbgInfoError> {
public:
  static char ID;

private:
  std::string Msg;

public:
  DbgInfoError(llvm::StringRef ErrString) : Msg(ErrString.str()) {}

  void log(llvm::raw_ostream &OS) const override { OS << Msg; }
  std::error_code convertToErrorCode() const override {
    return llvm::make_error_code(llvm::errc::io_error);
  }
};

char DbgInfoError::ID = 0;

static void debugDump(const llvm::Twine &Name, const char *Content,
                      size_t Size) {
  std::error_code EC;
  // no error handling since this is debug output
  llvm::raw_fd_ostream OS(Name.str(), EC);
  OS << llvm::StringRef(Content, Size);
  OS.close();
}

static std::string generateScopeUID(const llvm::DIScope &scope) {
  return std::string(scope.getDirectory().str())
      .append("/")
      .append(scope.getFilename().str());
}

struct Gen2VisaIdx {
  unsigned GenOffset;
  unsigned VisaIdx;
};

struct FinalizerDbgInfo {

  FinalizerDbgInfo(VISAKernel &VK, const llvm::Function &F,
                   bool FinalizerDbgDump = true) {
    void *GenXdbgInfo = nullptr;
    void *VISAMap = nullptr;
    unsigned int DbgSize = 0;
    unsigned int NumElems = 0;
    if (VK.GetGenxDebugInfo(GenXdbgInfo, DbgSize, VISAMap, NumElems) !=
        VISA_SUCCESS) {
      ErrMsg = "visa info decode error";
      return;
    }
    IGC_ASSERT(GenXdbgInfo);

    if (FinalizerDbgDump) {
      debugDump("dbg_raw_" + F.getName() + ".dump",
                static_cast<const char *>(GenXdbgInfo), DbgSize);
    }

    DecodedInfo = std::make_unique<IGC::DbgDecoder>(GenXdbgInfo);

    if (NumElems > 0 && VISAMap) {
      LLVM_DEBUG(llvm::dbgs() << "---\n");

      unsigned int *VisaMapUI = reinterpret_cast<unsigned int *>(VISAMap);
      for (unsigned int i = 0; i < NumElems * 2; i += 2) {
        auto GenISAOffset = VisaMapUI[i];
        auto VISAIndex = VisaMapUI[i + 1];
        Gen2Visa.push_back({GenISAOffset, VISAIndex});
        LLVM_DEBUG(llvm::dbgs() << "GenOffset: " << GenISAOffset
                                << " -> VisaIdx: " << VISAIndex << "\n");
      }
      LLVM_DEBUG(llvm::dbgs() << "---\n");
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
  std::vector<Gen2VisaIdx> Gen2Visa;
  std::vector<char> GenBinary;
  std::string ErrMsg;
};

std::map<std::string, unsigned>
generateFileScopes(IGC::StreamEmitter &SE,
                   const llvm::genx::VisaDebugInfo &GenXDbg) {

  std::map<std::string, unsigned> ScopeIDMap;

  LLVM_DEBUG(llvm::dbgs() << "\nSource Info - start\n");
  for (const auto &Item : GenXDbg.Locations) {
    if (!Item.second)
      continue;
    const auto &DL = Item.second->getDebugLoc();
    if (!DL)
      continue;
    const auto &Scope = *llvm::cast<llvm::DIScope>(DL.getScope());
    const auto &StrUID = generateScopeUID(Scope);

    unsigned NewId = ScopeIDMap.size() + 1;
    if (ScopeIDMap.find(StrUID) == ScopeIDMap.end()) {
      LLVM_DEBUG(llvm::dbgs() << "   " << StrUID << "\n");
      ScopeIDMap.insert(std::make_pair(StrUID, NewId));
      SE.EmitDwarfFileDirective(NewId, Scope.getDirectory(),
                                Scope.getFilename());
    }
  }

  return std::move(ScopeIDMap);
}

bool generateLineTable(IGC::StreamEmitter &SE, const FinalizerDbgInfo &GenDbg,
                       const llvm::genx::VisaDebugInfo &GenXDbg,
                       const std::map<std::string, unsigned> &FileScopes) {

  // now iterate over GEN->VISA mapping and find corresponding llvm debug info
  unsigned PC = 0;
  unsigned PrevSrcID = -1;
  unsigned PrevSrcLine = -1;

  LLVM_DEBUG(llvm::dbgs() << "\nline table generation:\n");
  for (const auto &Gen2Visa : GenDbg.Gen2Visa) {

    for (unsigned i = PC; i != Gen2Visa.GenOffset; ++i)
      SE.EmitInt8(GenDbg.GenBinary[i]);

    PC = Gen2Visa.GenOffset;
    auto VisaIdx = Gen2Visa.VisaIdx;
    auto ItLocation = GenXDbg.Locations.find(VisaIdx);

    if (ItLocation == GenXDbg.Locations.end()) {
      LLVM_DEBUG(llvm::dbgs()
                 << "Warning: no debug info for VISA@" << VisaIdx << "\n");
      continue;
    }
    const auto *Inst = ItLocation->second;
    const auto &DL = Inst->getDebugLoc();
    if (!DL) {
      LLVM_DEBUG(llvm::dbgs()
                 << "Warning: no debug location attached to " << *Inst << "\n");
      continue;
    }
    const auto &Scope = *llvm::cast<llvm::DIScope>(DL.getScope());
    const auto &StrUID = generateScopeUID(Scope);
    unsigned SrcID = FileScopes.at(StrUID);

    if (SrcID != PrevSrcID) {
      PrevSrcLine = -1;
    }
    if (DL.getLine() != PrevSrcLine) {
      LLVM_DEBUG(llvm::dbgs()
                 << "emitting new LocDirective: [" << Scope.getFilename()
                 << "] "
                 << "#" << DL.getLine() << " |" << DL.getCol() << "\n");
      SE.EmitDwarfLocDirective(SrcID, DL.getLine(), DL.getCol(), 1, 0, 0,
                               Scope.getFilename());
    }

    PrevSrcID = SrcID;
    PrevSrcLine = DL.getLine();
  }

  return true;
}

} // namespace

namespace llvm {
namespace genx {

llvm::Error generateDebugInfo(SmallVectorImpl<char> &ElfImage, VISAKernel &VK,
                              const genx::VisaDebugInfo &DbgInfo,
                              const Function &F, const std::string &TripleStr) {

  FinalizerDbgInfo GenDbg(VK, F);
  if (!GenDbg.ErrMsg.empty()) {
    return make_error<DbgInfoError>(GenDbg.ErrMsg);
  }

  llvm::raw_svector_ostream OS(ElfImage);

  const auto *Module = F.getParent();
  auto SE = std::make_unique<IGC::StreamEmitter>(
      OS, Module->getDataLayout().getStringRepresentation(), TripleStr, true);

  SE->SwitchSection(SE->GetTextSection());

  // Function Start
  SE->SetDwarfCompileUnitID(0);
  auto *Sym_FBegin = SE->GetTempSymbol("func_begin", 1);
  SE->EmitLabel(Sym_FBegin);

  // Dump known Locs
  for (const auto &Item : DbgInfo.Locations) {
    LLVM_DEBUG(llvm::dbgs() << "visa_idx: " << Item.first << " inst: ";
               if (Item.second) {
                 Item.second->print(llvm::dbgs(), true);
               } llvm::dbgs()
               << "\n";);
  }

  const auto &FileScopes = generateFileScopes(*SE, DbgInfo);
  generateLineTable(*SE, GenDbg, DbgInfo, FileScopes);

  // Terminate
  SE->EmitInt8(0);

  // Function End
  auto *Sym_FEnd = SE->GetTempSymbol("func_end", 1);
  SE->EmitLabel(Sym_FEnd);
  SE->EmitELFDiffSize(Sym_FBegin, Sym_FEnd, Sym_FBegin);
  SE->SetDwarfCompileUnitID(0);
  SE->Finalize();

  return Error::success();
}

} // namespace genx
} // namespace llvm
