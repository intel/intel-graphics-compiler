/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPrintfLegalization
/// ----------------------
/// For indexed string - string that is used in `@print.format.index` and thus
/// translated into index - only GV to GEP to `@print.format.index` value chains
/// are possible.
///
/// As first step this pass finds all places where format index doesn't take
/// pointer directly from GEP that gets pointer to the first character of a
/// constant string and changes the order of instructions:
///   GV -> GEP -> inst1 -> inst2 -> format.index
/// becomes:
///   GV -> GEP -> format.index -> inst1' -> inst2'
/// inst1, inst2 operated on pointers, inst1', inst2' operate on indices.
///
/// The 2nd step:
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
#include "vc/Utils/GenX/Printf.h"
#include "vc/Utils/General/InstRebuilder.h"
#include "vc/Utils/General/IRBuilder.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/Support/Alignment.h"

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Value.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/Error.h>

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <unordered_set>
#include <vector>

using namespace llvm;
using namespace vc;
using CallInstRef = std::reference_wrapper<CallInst>;
using InstructionRef = std::reference_wrapper<Instruction>;

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

  // Handle attributes.
  IGC_ASSERT_MESSAGE(
      OrigGV.hasAttribute(PrintfStringVariable),
      "original GV must already have the attribute before the cloning");
  auto OrigAttrs = OrigGV.getAttributes();
  OrigAttrs =
      OrigAttrs.removeAttribute(OrigGV.getContext(), PrintfStringVariable);
  OrigGV.setAttributes(OrigAttrs);
  ClonedGV->addAttribute(PrintfStringVariable);
  return *ClonedGV;
}

static void handleGEP(const FormatIndexGEPInfo &GEPInfo,
                      GlobalVariable &IndexedString) {
  auto &GEP = *GEPInfo.GEP;
  IGC_ASSERT_MESSAGE(GEP.getNumIndices() == 2 && GEP.hasAllZeroIndices(),
                     "format index GEP should just reach zero string element");
  IGC_ASSERT_MESSAGE(GEP.isInBounds(), "format index GEP should be in bounds");
  auto *NewGEP = &castArrayToFirstElemPtr(IndexedString);
  for (CallInst &FormatIndex : GEPInfo.FormatIndices)
    FormatIndex.setArgOperand(0, NewGEP);
}

static void legalizeIndexedString(const IndexedStringInfo &IndexedString) {
  auto &ClonedGV = cloneString(*IndexedString.GV);
  for (auto &GEPInfo : IndexedString.GEPs)
    handleGEP(GEPInfo, ClonedGV);
}

static bool legalizeIndexedStrings(Module &M) {
  IndexedStringInfoSeq Worklist = collectIllegalIndexedStrings(M);
  if (Worklist.empty())
    return false;
  for (auto &IndexedString : Worklist)
    legalizeIndexedString(IndexedString);
  return true;
}

// Format index is legal when it takes GEP that takes pointer to the first
// character of a constant string as operand (GV -> GEP -> format.index).
// This function returns whether the provided \p Inst is a format index but
// not a legal one.
static bool isIllegalPrintFormatIndex(const Instruction &Inst) {
  // Not print format index at all.
  if (!isPrintFormatIndex(Inst))
    return false;
  return !isConstantStringFirstElementGEP(*Inst.getOperand(0));
}

namespace {
// Describes what to do with an operand in terms of RebuildInfoBuilder
// methods: whether addEntry or addNodeIfRequired should be called and with
// which operands.
struct OperandTreatment {
  bool IsEntry;
  unsigned OperandNo;
  Optional<Value *> NewOperand;
};
} // anonymous namespace

using VisitedSet = std::unordered_set<Value *>;
template <typename RebuildInfoBuilderT>
void recursivelyTraverseFormatIndexPreds(Value &Pred,
                                         RebuildInfoBuilderT &ToRebuild);

// Continues traversing through select operand unless proper GEP is reached.
// In the latter case format index for GEP is produced (a legal format index).
// Returns how to treat the provided select operand.
template <typename RebuildInfoBuilderT>
OperandTreatment traverseSelectOperand(SelectInst &Sel, Use &Op,
                                       RebuildInfoBuilderT &ToRebuild,
                                       VisitedSet &Visited) {
  if (isConstantStringFirstElementGEP(*Op.get())) {
    CallInst &FormatIndex = createPrintFormatIndex(*Op.get(), Sel);
    return {/* IsEntry */ true, Op.getOperandNo(), &FormatIndex};
  }
  // go deeper
  recursivelyTraverseFormatIndexPreds(*Op.get(), ToRebuild, Visited);
  return {/* IsEntry */ false, Op.getOperandNo(), {}};
}

// Recurring function - implementation for traverseFormatIndexPreds.
template <typename RebuildInfoBuilderT>
void recursivelyTraverseFormatIndexPreds(Value &Pred,
                                         RebuildInfoBuilderT &ToRebuild,
                                         VisitedSet &Visited) {
  if (Visited.count(&Pred))
    return;
  Visited.insert(&Pred);
  if (!isa<SelectInst>(Pred))
    report_fatal_error(PrintfStringAccessError);
  auto &Sel = cast<SelectInst>(Pred);
  std::array<OperandTreatment, 2> OperandInfos = {
      traverseSelectOperand(Sel, Sel.getOperandUse(1), ToRebuild, Visited),
      traverseSelectOperand(Sel, Sel.getOperandUse(2), ToRebuild, Visited)};
  // Can add new nodes only when both operands are traversed.
  for (const OperandTreatment &OperandInfo : OperandInfos) {
    if (OperandInfo.IsEntry) {
      ToRebuild.addEntry(Sel, OperandInfo.OperandNo,
                         *OperandInfo.NewOperand.getValue());
      continue;
    }
    IGC_ASSERT_MESSAGE(!OperandInfo.NewOperand.hasValue(),
                       "shouldn't have new operand for not entry");
    ToRebuild.addNodeIfRequired(Sel, OperandInfo.OperandNo);
  }
}

// Recursively traverse format index operands, format index operand operands
// and so on. Fills rebuild info. Already visited instructions are inserted
// into \p Visited set, and this function correspondingly updates this set.
template <typename RebuildInfoBuilderT>
void traverseFormatIndexPreds(Instruction &FormatIndex,
                              RebuildInfoBuilderT &ToRebuild,
                              VisitedSet &Visited) {
  Visited.insert(&FormatIndex);
  recursivelyTraverseFormatIndexPreds(*FormatIndex.getOperand(0), ToRebuild,
                                      Visited);
  ToRebuild.addNodeIfRequired(FormatIndex, 0);
}

// When all format index predecessors are replaced, replacements already return
// indices. Thus no additional instruction is needed, replacement for format
// index is its updated operand.
static Value *rebuildPrintFormatIndex(const InstToRebuild &InstInfo) {
  IGC_ASSERT_MESSAGE(InstInfo.validate(),
                     "wrong argument: invalid rebuild info");
  IGC_ASSERT_MESSAGE(isPrintFormatIndex(*InstInfo.User),
                     "wrong argument: format index replacement is expected");
  IGC_ASSERT_MESSAGE(
      InstInfo.OperandNos.size() == 1 && InstInfo.OperandNos[0] == 0,
      "format index has only one operand and it should be updated");
  IGC_ASSERT_MESSAGE(InstInfo.NewOperands.size() == 1 &&
                         InstInfo.NewOperands[0],
                     "should have a new operand to update");
  IGC_ASSERT_MESSAGE(InstInfo.IsTerminal,
                     "format index should be a terminal instruction");
  // When we get to the final format index, indices are already propogated
  // through the chain of instructions. Thus new format index operand is
  // already an index.
  return InstInfo.NewOperands[0];
}

// Transforms such instruction chain:
//   GV -> GEP -> inst1 -> inst2 -> format.index
// into this instruction chain:
//   GV -> GEP -> format.index -> inst1' -> inst2'
// InstRebuilder is used to transform instructions that operate over pointers
// into instructions that operate over indices (inst1 -> inst1').
// RebuildInfo consists of these instructions between GEP and format index and
// format index itself. Format index is terminal instruction.
static bool legalizeFormatIndices(Function &F) {
  std::vector<InstructionRef> IllegalFormatIndices;
  copy_if(instructions(F), std::back_inserter(IllegalFormatIndices),
          [](Instruction &Inst) { return isIllegalPrintFormatIndex(Inst); });
  if (IllegalFormatIndices.empty())
    return false;

  auto ToRebuild = MakeRebuildInfoBuilder(
      [](const Instruction &Inst) { return isPrintFormatIndex(Inst); });
  VisitedSet Visited;
  // Collecting instructions to rebuild.
  for (Instruction &FormatIndex : IllegalFormatIndices)
    traverseFormatIndexPreds(FormatIndex, ToRebuild, Visited);
  // Rebuilding the collected instructions.
  MakeInstructionRebuilder(
      std::move(ToRebuild).emit(),
      [](const InstToRebuild &InstInfo) {
        IGC_ASSERT_MESSAGE(InstInfo.validate(),
                           "wrong argument: invalid rebuild info");
        return isPrintFormatIndex(*InstInfo.User);
      },
      [](const InstToRebuild &InstInfo) {
        return rebuildPrintFormatIndex(InstInfo);
      })
      .rebuild();
  return true;
}

bool GenXPrintfLegalization::runOnModule(Module &M) {
  bool Modified = false;
  for (Function &F : M)
    Modified |= legalizeFormatIndices(F);
  Modified |= legalizeIndexedStrings(M);
  return Modified;
}
