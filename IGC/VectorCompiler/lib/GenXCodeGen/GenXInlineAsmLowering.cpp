/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXInlineAsmLowering
/// ------------
/// This pass recreates VISA inline assembly with new types
/// if 'cr' constraint is used. Also pass  inserts constraints
/// information as metadata in order not to parse constraints
/// string  every time in each pass where this information is needed.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXGotoJoin.h"
#include "GenXIntrinsics.h"
#include "GenXModule.h"
#include "GenXSubtarget.h"
#include "GenXUtil.h"
#include "GenXVisa.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "visa_igc_common_header.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;

namespace {

class GenXInlineAsmLowering : public FunctionPass {
  using ConstraintInfoVector = InlineAsm::ConstraintInfoVector;
  using ConstraintInfo = InlineAsm::ConstraintInfo;
  using GenXConstraintInfoVector = std::vector<GenXInlineAsmInfo>;

private:
  LLVMContext *Context = nullptr;
  SmallVector<Instruction *, 8> ToErase;

  MDNode *createInlineAsmMetadata(
      CallInst *CI,
      const InlineAsm::ConstraintInfoVector &ConstraintsInfo) const;

  Type *rewriteTypeForConstraintIfNeeded(
      Type *Ty, const GenXInlineAsmInfo &ConstraintInfo) const;
  Type *rewriteTypeForCR(Type *CRType) const;

  FunctionType *rewriteFunctionTypeForInlineAsmIfNeeded(
      CallInst *CI, const GenXConstraintInfoVector &ConstraintsInfo) const;

  void replaceInlineAsmUses(CallInst *Of, CallInst *With,
                            const GenXConstraintInfoVector &ConstraintsInfo);

  CallInst *
  recreateInlineAsmWithCR(CallInst *CI,
                          const GenXConstraintInfoVector &ConstraintsInfo);

public:
  static char ID;
  explicit GenXInlineAsmLowering() : FunctionPass(ID) {}
  StringRef getPassName() const override {
    return "GenX VISA inline asm lowering";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
};

} // end namespace

char GenXInlineAsmLowering::ID = 0;
namespace llvm {
void initializeGenXInlineAsmLoweringPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXInlineAsmLowering, "GenXInlineAsmLowering",
                      "GenXInlineAsmLowering", false, false)
INITIALIZE_PASS_END(GenXInlineAsmLowering, "GenXInlineAsmLowering",
                    "GenXInlineAsmLowering", false, false)

FunctionPass *llvm::createGenXInlineAsmLoweringPass() {
  initializeGenXInlineAsmLoweringPass(*PassRegistry::getPassRegistry());
  return new GenXInlineAsmLowering;
}

void GenXInlineAsmLowering::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addPreserved<GenXModule>();
}

bool GenXInlineAsmLowering::runOnFunction(Function &F) {
  auto GM = getAnalysisIfAvailable<GenXModule>();
  if (GM && !GM->HasInlineAsm())
    return false;

  // Collect inline asm worklist
  auto InlineAsmsToProcess =
      make_filter_range(instructions(&F), [&](Instruction &I) {
        auto *CI = dyn_cast<CallInst>(&I);
        // No need to process inline asm with empty constraint string
        return CI && CI->isInlineAsm() &&
               !cast<InlineAsm>(IGCLLVM::getCalledValue(CI))
                    ->getConstraintString()
                    .empty();
      });

  SmallVector<CallInst *, 8> InlineAsms;
  llvm::transform(InlineAsmsToProcess, std::back_inserter(InlineAsms),
                  [&](Instruction &I) { return cast<CallInst>(&I); });

  if (InlineAsms.empty())
    return false;

  Context = &InlineAsms[0]->getContext();
  for (auto *CI : InlineAsms) {
    auto *IA = cast<InlineAsm>(IGCLLVM::getCalledValue(CI));
    InlineAsm::ConstraintInfoVector ConstraintsInfo = IA->ParseConstraints();
    MDNode *ConstraintsMD = createInlineAsmMetadata(CI, ConstraintsInfo);
    GenXConstraintInfoVector GenXConstraintsInfo =
        genx::getGenXInlineAsmInfo(ConstraintsMD);

    // No need to recreate asm expression if
    // there is no 'cr' constraint. Set created metadata and return.
    if (!genx::hasConstraintOfType(GenXConstraintsInfo,
                                   ConstraintType::Constraint_cr)) {
      CI->setMetadata(genx::MD_genx_inline_asm_info, ConstraintsMD);
      continue;
    }

    // Create new inline asm and don't forget to set
    // earlier created metadata.
    CallInst *NewCI = recreateInlineAsmWithCR(CI, GenXConstraintsInfo);
    NewCI->setMetadata(genx::MD_genx_inline_asm_info, ConstraintsMD);
  }

  for (auto *I : ToErase)
    I->eraseFromParent();
  ToErase.clear();

  return true;
}

// Creating metadata for inline asm constraints
// in order not to parse constraints string  every time in each pass
// where this information is needed.
MDNode *GenXInlineAsmLowering::createInlineAsmMetadata(
    CallInst *CI, const ConstraintInfoVector &ConstraintsInfo) const {
  IGC_ASSERT_MESSAGE(!ConstraintsInfo.empty(), "Non empty constraints expected");
  IGC_ASSERT_MESSAGE(CI->isInlineAsm(), "Inline asm expected");

  Type *Int1Ty = Type::getInt1Ty(*Context);
  Type *Int32Ty = Type::getInt32Ty(*Context);
  std::vector<Metadata *> Entries;
  for (auto &&Info : ConstraintsInfo) {
    std::string Codes;
    if (genx::isInlineAsmMatchingInputConstraint(Info))
      Codes = genx::getInlineAsmCodes(
          ConstraintsInfo[genx::getInlineAsmMatchedOperand(Info)]);
    else
      Codes = genx::getInlineAsmCodes(Info);

    genx::ConstraintType CTy = genx::getInlineAsmConstraintType(Codes);
    if (CTy == ConstraintType::Constraint_unknown)
      Context->emitError(CI, "Unsupported constraint '" + Codes +
                                 "' in inline assembly");

    Metadata *EntryMD[3] = {
        ConstantAsMetadata::get(
            ConstantInt::get(Int32Ty, static_cast<uint32_t>(CTy))),
        ConstantAsMetadata::get(ConstantInt::get(Int32Ty, Info.MatchingInput)),
        ConstantAsMetadata::get(ConstantInt::get(
            Int1Ty, (Info.Type == InlineAsm::ConstraintPrefix::isOutput)))};
    Entries.push_back(MDNode::get(*Context, EntryMD));
  }
  return MDTuple::get(*Context, Entries);
}

Type *GenXInlineAsmLowering::rewriteTypeForConstraintIfNeeded(
    Type *Ty, const GenXInlineAsmInfo &ConstraintInfo) const {
  switch (ConstraintInfo.getConstraintType()) {
  default:
    return Ty;
  case ConstraintType::Constraint_cr:
    return rewriteTypeForCR(Ty);
  }
}

Type *GenXInlineAsmLowering::rewriteTypeForCR(Type *CRType) const {
  IGC_ASSERT_MESSAGE(CRType->isIntOrIntVectorTy(),
    "Expected integer inputs for 'cr' constraint");
  Type *Int1Ty = Type::getInt1Ty(*Context);
  return CRType->isVectorTy()
             ? IGCLLVM::FixedVectorType::get(
                   Int1Ty,
                   cast<IGCLLVM::FixedVectorType>(CRType)->getNumElements())
             : Int1Ty;
}

// If there exist 'cr' for now output a new result type must be constructed
FunctionType *GenXInlineAsmLowering::rewriteFunctionTypeForInlineAsmIfNeeded(
    CallInst *CI, const GenXConstraintInfoVector &ConstraintsInfo) const {
  // Rewriting return type
  unsigned NumOutputs = genx::getInlineAsmNumOutputs(CI);
  std::vector<Type *> NewResultsTypes;
  if (NumOutputs == 1) {
    NewResultsTypes.push_back(
        rewriteTypeForConstraintIfNeeded(CI->getType(), ConstraintsInfo[0]));
  } else if (NumOutputs > 1) {
    auto *ST = cast<StructType>(CI->getType());
    std::transform(ST->element_begin(), ST->element_end(),
                   ConstraintsInfo.begin(), std::back_inserter(NewResultsTypes),
                   [&](Type *Ty, const GenXInlineAsmInfo &Info) {
                     return rewriteTypeForConstraintIfNeeded(Ty, Info);
                   });
  }

  // New return type: struct for multiple outputs,
  // void for no outputs, and one exact type for single output
  Type *NewRetType;
  if (NewResultsTypes.empty())
    NewRetType = Type::getVoidTy(*Context);
  else if (NewResultsTypes.size() == 1)
    NewRetType = NewResultsTypes[0];
  else
    NewRetType = StructType::get(*Context, NewResultsTypes);

  // Rewritng params types
  std::vector<Type *> NewParamsTypes;
  std::transform(CI->arg_begin(), CI->arg_end(),
                 ConstraintsInfo.begin() + NumOutputs,
                 std::back_inserter(NewParamsTypes),
                 [&](Value *V, const GenXInlineAsmInfo &Info) {
                   return rewriteTypeForConstraintIfNeeded(V->getType(), Info);
                 });
  return FunctionType::get(NewRetType, NewParamsTypes, false);
}

// If result type differs than iterate over all
// users of original call and replace it's
// uses with new outputs. Thus new extractelements and
// zero exstensions might be created. Existing extracts should be eliminated.
void GenXInlineAsmLowering::replaceInlineAsmUses(
    CallInst *Of, CallInst *With,
    const GenXConstraintInfoVector &ConstraintsInfo) {
  if (Of->getType() == With->getType()) {
    Of->replaceAllUsesWith(With);
    return;
  }
  IRBuilder<> Builder(*Context);
  Builder.SetInsertPoint(With->getNextNode());
  unsigned NumOutputs = genx::getInlineAsmNumOutputs(Of);
  if (NumOutputs == 1) {
    Value *NewResZExt = Builder.CreateZExt(With, Of->getType(), ".asm.zext.cr");
    Of->replaceAllUsesWith(NewResZExt);
    return;
  }

  // Create new extractvalues and replace all uses
  for (auto *U : Of->users()) {
    Value *ToZext = With;
    auto *EV = cast<ExtractValueInst>(U);
    ToErase.push_back(EV);
    unsigned OutputConstraintIdx = EV->getIndices()[0];
    ToZext =
        Builder.CreateExtractValue(ToZext, OutputConstraintIdx, "asmresult.cr");
    // Zero extension needed only for 'cr' output
    if (ConstraintsInfo[OutputConstraintIdx].getConstraintType() ==
        genx::ConstraintType::Constraint_cr)
      ToZext = Builder.CreateZExt(ToZext, U->getType(), ".asmresult.zext.cr");
    U->replaceAllUsesWith(ToZext);
  }
}

// If inline assembly uses 'cr' constraints (for now)
// all types should be converted to i1. So inserting
// truncations for inputs and zero extensions for outputs.
CallInst *GenXInlineAsmLowering::recreateInlineAsmWithCR(
    CallInst *CI, const GenXConstraintInfoVector &ConstraintsInfo) {
  IGC_ASSERT_MESSAGE(!ConstraintsInfo.empty(), "Non empty constraints expected");
  IGC_ASSERT_MESSAGE(CI->isInlineAsm(), "Inline asm expected");

  // If there exist 'cr' output a new result type must be constructed
  FunctionType *NewFTy =
      rewriteFunctionTypeForInlineAsmIfNeeded(CI, ConstraintsInfo);

  // New function types for 'cr':
  // any_int       -> i1
  // <any_int x N> -> <i1 x N>
  //
  // Create truncation for input args if needed
  IRBuilder<> Builder(CI);
  std::vector<Value *> NewArgs;
  std::transform(CI->arg_begin(), CI->arg_end(), NewFTy->param_begin(),
                 std::back_inserter(NewArgs), [&](Value *Arg, Type *NewArgTy) {
                   if (Arg->getType() != NewArgTy)
                     Arg = Builder.CreateTrunc(Arg, NewArgTy, ".trunc.cr");
                   return Arg;
                 });

  // Create exactly the same inline assembly but with new function type
  auto *IA = cast<InlineAsm>(IGCLLVM::getCalledValue(CI));
  InlineAsm *NewIA = InlineAsm::get(
      NewFTy, IA->getAsmString(), IA->getConstraintString(),
      IA->hasSideEffects(), IA->isAlignStack(), IA->getDialect());
  CallInst *NewCI = Builder.CreateCall(NewIA, NewArgs, ".asm.cr");
  NewCI->setAttributes(CI->getAttributes());
  NewCI->setDebugLoc(CI->getDebugLoc());

  replaceInlineAsmUses(CI, NewCI, ConstraintsInfo);
  ToErase.push_back(CI);

  return NewCI;
}
