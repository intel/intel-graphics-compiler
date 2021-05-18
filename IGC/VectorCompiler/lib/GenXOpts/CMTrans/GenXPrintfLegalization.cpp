/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPrintfLegalization
/// ----------------------
/// For indexed string - string that is used in @genx.print.format.index and
/// thus translated into index - only GV to GEP to @genx.print.format.index
/// value chains are possible.
/// This pass finds global variables that are both used as indexed strings and
/// as normal global variables and splits those global variables into 2:
///   indexed string - this variable is used only in described value chanes;
///   normal GV - used in all other cases.
/// Example:
///         GV                              GV        GV(clone)
///        /  \                            /  \           |
///  ptrtoint  GEP               =>  ptrtoint GEP        GEP
///           /   \                            |          |
///       ptrtoint format.index            ptrtoint  format.index
//===----------------------------------------------------------------------===//

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/GenXOpts/Utils/Printf.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/Support/Alignment.h"

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Value.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>

using namespace llvm;
using CallInstRef = std::reference_wrapper<CallInst>;

namespace {

using FormatIndexSeq = std::vector<CallInstRef>;

// If format index GEP is illegal, this GEP definitely should be considered.
// Format index GEP itself may be legal, but we still need to consider
// it if global variable is used not only in legal GEPs. Because in this case
// we still need to clone GV.
struct FormatIndexGEPInfo {
  GEPOperator *GEP;
  FormatIndexSeq FormatIndices;

  FormatIndexGEPInfo(GEPOperator &GEPIn) : GEP{&GEPIn} {
    // Using additional copy_if instead of make_filter_range as workaround,
    // because user_iterator returns pointer instead of reference.
    std::vector<User *> TMP;
    std::copy_if(GEPIn.user_begin(), GEPIn.user_end(), std::back_inserter(TMP),
                 [](User *Usr) { return isPrintFormatIndex(*Usr); });
    std::transform(
        TMP.begin(), TMP.end(), std::back_inserter(FormatIndices),
        [](User *Usr) -> CallInstRef { return *cast<CallInst>(Usr); });
    IGC_ASSERT_MESSAGE(!FormatIndices.empty(),
                       "format index GEP should have some format index users");
  }
};

using FormatIndexGEPInfoSeq = std::vector<FormatIndexGEPInfo>;
struct IndexedStringInfo {
  GlobalVariable *GV;
  FormatIndexGEPInfoSeq GEPs;

  IndexedStringInfo(GlobalVariable &GVIn) : GV{&GVIn} {
    // Using additional copy_if instead of make_filter_range as workaround,
    // because user_iterator returns pointer instead of reference.
    std::vector<User *> TMP;
    std::copy_if(GVIn.user_begin(), GVIn.user_end(), std::back_inserter(TMP),
                 [](User *Usr) { return isPrintFormatIndexGEP(*Usr); });
    std::transform(TMP.begin(), TMP.end(), std::back_inserter(GEPs),
                   [](User *Usr) -> FormatIndexGEPInfo {
                     return {*cast<GEPOperator>(Usr)};
                   });
    IGC_ASSERT_MESSAGE(
        !GEPs.empty(),
        "indexed string should have some format index GEP users");
  }
};
using IndexedStringInfoSeq = std::vector<IndexedStringInfo>;

class GenXPrintfLegalization final : public ModulePass {
public:
  static char ID;
  GenXPrintfLegalization() : ModulePass(ID) {}
  StringRef getPassName() const override { return "GenX printf legalization"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
};
} // namespace

char GenXPrintfLegalization::ID = 0;

INITIALIZE_PASS_BEGIN(GenXPrintfLegalization, "GenXPrintfLegalization",
                      "GenXPrintfLegalization", false, false)
INITIALIZE_PASS_END(GenXPrintfLegalization, "GenXPrintfLegalization",
                    "GenXPrintfLegalization", false, false)

ModulePass *llvm::createGenXPrintfLegalizationPass() {
  initializeGenXPrintfLegalizationPass(*PassRegistry::getPassRegistry());
  return new GenXPrintfLegalization;
};

void GenXPrintfLegalization::getAnalysisUsage(AnalysisUsage &AU) const {}

static bool isIllegalIndexedString(const GlobalVariable &GV) {
  if (!GV.isConstant())
    return false;
  if (!GV.getValueType()->isArrayTy())
    return false;
  if (!GV.getValueType()->getArrayElementType()->isIntegerTy(8))
    return false;
  // It's not indexed string at all without print format index GEP.
  if (std::none_of(GV.user_begin(), GV.user_end(),
                   [](const User *Usr) { return isPrintFormatIndexGEP(*Usr); }))
    return false;
  // It's a legal string when all its users are legal print format index GEPs.
  return std::any_of(GV.user_begin(), GV.user_end(), [](const User *Usr) {
    return !isLegalPrintFormatIndexGEP(*Usr);
  });
}

static IndexedStringInfoSeq collectIllegalIndexedStrings(Module &M) {
  IndexedStringInfoSeq Strings;
  std::copy_if(M.global_begin(), M.global_end(), std::back_inserter(Strings),
               [](GlobalVariable &GV) { return isIllegalIndexedString(GV); });
  return Strings;
}

static GlobalVariable &cloneString(GlobalVariable &OrigGV) {
  auto *ClonedGV = new GlobalVariable{*OrigGV.getParent(),
                                      OrigGV.getValueType(),
                                      OrigGV.isConstant(),
                                      OrigGV.getLinkage(),
                                      OrigGV.getInitializer(),
                                      OrigGV.getName() + ".indexed",
                                      nullptr,
                                      OrigGV.getThreadLocalMode(),
                                      OrigGV.getAddressSpace()};
  // Avoiding constants merging.
  OrigGV.setUnnamedAddr(GlobalValue::UnnamedAddr::None);
  ClonedGV->setAlignment(IGCLLVM::getAlign(OrigGV));
  return *ClonedGV;
}

static void handleGEP(const FormatIndexGEPInfo &GEPInfo,
                      GlobalVariable &IndexedString) {
  auto &GEP = *GEPInfo.GEP;
  IGC_ASSERT_MESSAGE(GEP.getNumIndices() == 2 && GEP.hasAllZeroIndices(),
                     "format index GEP should just reach zero string element");
  IGC_ASSERT_MESSAGE(GEP.isInBounds(), "format index GEP should be in bounds");
  IRBuilder<> IRB{GEP.getContext()};
  std::vector<Constant *> Indices = {IRB.getInt32(0), IRB.getInt32(0)};
  auto *NewGEP = ConstantExpr::getInBoundsGetElementPtr(
      IndexedString.getValueType(), &IndexedString, Indices);
  for (CallInst &FormatIndex : GEPInfo.FormatIndices)
    FormatIndex.setArgOperand(0, NewGEP);
}

static void legalizeIndexedString(const IndexedStringInfo &IndexedString) {
  auto &ClonedGV = cloneString(*IndexedString.GV);
  for (auto &GEPInfo : IndexedString.GEPs)
    handleGEP(GEPInfo, ClonedGV);
}

bool GenXPrintfLegalization::runOnModule(Module &M) {
  IndexedStringInfoSeq Worklist = collectIllegalIndexedStrings(M);
  if (Worklist.empty())
    return false;
  for (auto &IndexedString : Worklist)
    legalizeIndexedString(IndexedString);
  return true;
}
