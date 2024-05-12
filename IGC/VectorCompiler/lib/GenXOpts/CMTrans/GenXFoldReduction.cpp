/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Utils/GenX/Region.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>

#define DEBUG_TYPE "genx-fold-reduction"

using namespace llvm;

static cl::opt<bool>
    MatchReduction("vc-match-reduction", cl::init(true), cl::Hidden,
                   cl::desc("Match vector reduction instruction sequences"));

namespace {
class GenXFoldReduction final : public FunctionPass,
                                public InstVisitor<GenXFoldReduction> {
public:
  static char ID;
  explicit GenXFoldReduction() : FunctionPass(ID) {}
  StringRef getPassName() const override { return "GenX fold reduction"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override;

  void visitBinaryOperator(BinaryOperator &BO);
  void visitIntrinsicInst(IntrinsicInst &II);

private:
  bool tryFoldReduction(Instruction &I, Intrinsic::ID IID,
                        Value *Start = nullptr);

  std::pair<Value *, Instruction *>
  findReductionSourceAndResult(Instruction &I);

  const DataLayout *DL = nullptr;
  bool Changed = false;
};
} // namespace

char GenXFoldReduction::ID = 0;

INITIALIZE_PASS_BEGIN(GenXFoldReduction, "GenXFoldReduction",
                      "GenXFoldReduction", false, false)
INITIALIZE_PASS_END(GenXFoldReduction, "GenXFoldReduction", "GenXFoldReduction",
                    false, false)

FunctionPass *llvm::createGenXFoldReductionPass() {
  initializeGenXFoldReductionPass(*PassRegistry::getPassRegistry());
  return new GenXFoldReduction();
}

bool GenXFoldReduction::runOnFunction(Function &F) {
  if (!MatchReduction)
    return false;

  LLVM_DEBUG(dbgs() << ">> GenXFoldReduction <<\n");
  Changed = false;
  DL = &F.getParent()->getDataLayout();

  visit(F);

  return Changed;
}

void GenXFoldReduction::visitBinaryOperator(BinaryOperator &BO) {
  Value *Start = nullptr;
  auto IID = Intrinsic::not_intrinsic;

  auto *Ty = BO.getType()->getScalarType();

  switch (BO.getOpcode()) {
  default:
    return;
  case Instruction::Add:
#if LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::vector_reduce_add;
#else  // LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::experimental_vector_reduce_add;
#endif // LLVM_VERSION_MAJOR >= 12
    break;
  case Instruction::Mul:
#if LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::vector_reduce_mul;
#else  // LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::experimental_vector_reduce_mul;
#endif // LLVM_VERSION_MAJOR >= 12
    break;
  case Instruction::FAdd:
#if LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::vector_reduce_fadd;
#else  // LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::experimental_vector_reduce_v2_fadd;
#endif // LLVM_VERSION_MAJOR >= 12
    Start = ConstantFP::getNegativeZero(Ty);
    break;
  case Instruction::FMul:
#if LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::vector_reduce_fmul;
#else  // LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::experimental_vector_reduce_v2_fmul;
#endif // LLVM_VERSION_MAJOR >= 12
    Start = ConstantFP::get(Ty, 1.0);
    break;
  }

  Changed |= tryFoldReduction(BO, IID, Start);
}

void GenXFoldReduction::visitIntrinsicInst(IntrinsicInst &II) {
  auto IID = Intrinsic::not_intrinsic;

  switch (II.getIntrinsicID()) {
  default:
    return;
  case Intrinsic::maxnum:
#if LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::vector_reduce_fmax;
#else  // LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::experimental_vector_reduce_fmax;
#endif // LLVM_VERSION_MAJOR >= 12
    break;
  case Intrinsic::minnum:
#if LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::vector_reduce_fmin;
#else  // LLVM_VERSION_MAJOR >= 12
    IID = Intrinsic::experimental_vector_reduce_fmin;
#endif // LLVM_VERSION_MAJOR >= 12
    break;
  }

  Changed |= tryFoldReduction(II, IID);
}

namespace {
struct ReductionSourceDescriptor {
  Value *Src = nullptr;
  uint64_t Idx = 0;
  uint64_t NumElts = 0;
};

bool areConsecutive(const ReductionSourceDescriptor &Desc0,
                    const ReductionSourceDescriptor &Desc1) {
  return Desc0.Src == Desc1.Src && Desc0.Idx == 0 && Desc0.NumElts == Desc1.Idx;
}

ReductionSourceDescriptor lookupThruReadRegion(ReductionSourceDescriptor Desc) {
  using namespace GenXIntrinsic;

  while (isRdRegion(Desc.Src)) {
    auto *RdRegion = cast<CallInst>(Desc.Src);
    vc::CMRegion R(RdRegion);

    if (R.Indirect || !R.isContiguous())
      break;

    Desc.Src = RdRegion->getOperand(GenXRegion::OldValueOperandNum);
    Desc.Idx += R.getOffsetInElements();
  }

  return Desc;
}

ReductionSourceDescriptor getReductionSource(Value *V) {
  using namespace GenXIntrinsic;
  if (!V->hasOneUse())
    return {};

  if (auto *Extract = dyn_cast<ExtractElementInst>(V)) {
    auto *Src = Extract->getVectorOperand();
    auto *Idx = dyn_cast<ConstantInt>(Extract->getIndexOperand());
    if (!Idx)
      return {};
    return {Src, Idx->getZExtValue(), 1};
  }

  if (!isRdRegion(V))
    return {};

  auto *RdRegion = cast<CallInst>(V);
  vc::CMRegion R(RdRegion);

  auto *Src = RdRegion->getOperand(GenXRegion::OldValueOperandNum);
  if (R.Indirect || !R.isContiguous())
    return {};

  auto *VTy = cast<IGCLLVM::FixedVectorType>(V->getType());
  auto NumElts = VTy->getNumElements();

  return {Src, R.getOffsetInElements(), NumElts};
}

bool areSameOperations(Instruction *I1, Instruction *I2) {
  auto *II1 = dyn_cast<IntrinsicInst>(I1);
  auto *II2 = dyn_cast<IntrinsicInst>(I2);
  if (II1 && II2)
    return II1->getIntrinsicID() == II2->getIntrinsicID();

  auto *BO1 = dyn_cast<BinaryOperator>(I1);
  auto *BO2 = dyn_cast<BinaryOperator>(I2);
  if (!BO1 || !BO2)
    return false;

  return BO1->getOpcode() == BO2->getOpcode();
}
} // namespace

bool GenXFoldReduction::tryFoldReduction(Instruction &I, Intrinsic::ID IID,
                                         Value *Start) {
  auto [Src, Result] = findReductionSourceAndResult(I);
  if (!Src || !Result)
    return false;

  LLVM_DEBUG(dbgs() << "Reduction sequence found.\n  Source: " << *Src
                    << "\n  Result: " << *Result << "\n");

  IRBuilder<> Builder(Result);

  auto *ReductionIntrinsic =
      Intrinsic::getDeclaration(I.getModule(), IID, {Src->getType()});

  SmallVector<Value *, 2> Args;
  if (Start)
    Args.push_back(Start);
  Args.push_back(Src);

  auto *NewResult = Builder.CreateCall(ReductionIntrinsic, Args);
  if (Result->getType()->isFloatingPointTy())
    NewResult->setHasAllowReassoc(true);

  NewResult->takeName(Result);
  Result->replaceAllUsesWith(NewResult);
  Result->eraseFromParent();

  LLVM_DEBUG(dbgs() << "New reduction intrinsic created: " << *NewResult
                    << "\n");

  return true;
}

namespace {
// Try to match the non-power-of-two reduction step as follows:
// 1. Current instruction has one user.
// 2. The user is the same instruction as the current one.
// 3. The second operand of the user is an rdregion or extractelement from
// the initial reduction source.
Instruction *matchNextNonPowerOfTwoStep(Instruction *CurrInst,
                                        ReductionSourceDescriptor &SrcDesc) {
  if (!CurrInst->hasOneUse())
    return nullptr;

  auto *NextInst = dyn_cast<Instruction>(CurrInst->user_back());
  if (!NextInst || !areSameOperations(CurrInst, NextInst))
    return nullptr;

  auto NextDesc = getReductionSource(NextInst->getOperand(1));
  if (!NextDesc.Src)
    return nullptr;

  NextDesc = lookupThruReadRegion(NextDesc);
  if (NextDesc.Src != SrcDesc.Src ||
      NextDesc.Idx != SrcDesc.Idx + SrcDesc.NumElts)
    return nullptr;

  SrcDesc.NumElts += NextDesc.NumElts;
  return NextInst;
}

// Try to match the next reduction step as follows:
// 1. Current instruction has two users.
// 2. Both users are rdregion or extractelement instructions, splitting
// current instruction value into two halves.
// 3. Both users are consumed by the instruction that has the same opcode as
// the current one.
//
// Example:
//   %curr = add <16 x i32> %x.0, %x.16
//   %curr.0 = <8 x i32> call @llvm.genx.rdregioni.v8i32.v16i32.i16(
//       <16 x i32> %curr, i32 1, i32, 1, i32 0, i16 0, i32 0)
//   %curr.8 = <8 x i32> call @llvm.genx.rdregioni.v8i32.v16i32.i16(
//       <16 x i32> %curr, i32 1, i32, 1, i32 0, i16 32, i32 0)
//   %next = add <8 x i32> %curr.0, %curr.8
Instruction *matchNextStep(Instruction *CurrInst) {
  if (!CurrInst->hasNUses(2))
    return nullptr;

  auto *User = dyn_cast<Instruction>(CurrInst->user_back());
  if (!User || !User->hasOneUse())
    return nullptr;

  auto *NextInst = dyn_cast<Instruction>(User->user_back());
  if (!NextInst || !areSameOperations(CurrInst, NextInst))
    return nullptr;

  auto NextDesc0 = getReductionSource(NextInst->getOperand(0));
  auto NextDesc1 = getReductionSource(NextInst->getOperand(1));

  if (CurrInst != NextDesc0.Src || !areConsecutive(NextDesc0, NextDesc1))
    return nullptr;

  return NextInst;
}
} // namespace

std::pair<Value *, Instruction *>
GenXFoldReduction::findReductionSourceAndResult(Instruction &I) {
  if (I.getNumUses() == 0)
    return {nullptr, nullptr};

  auto *Ty = dyn_cast<IGCLLVM::FixedVectorType>(I.getType());
  if (!Ty)
    return {nullptr, nullptr};

  auto Desc0 = getReductionSource(I.getOperand(0));
  auto Desc1 = getReductionSource(I.getOperand(1));

  if (!Desc0.Src || !areConsecutive(Desc0, Desc1))
    return {nullptr, nullptr};

  auto *Src = Desc0.Src;
  auto *SrcTy = cast<IGCLLVM::FixedVectorType>(Src->getType());
  auto Width = SrcTy->getNumElements();
  auto SrcDesc = lookupThruReadRegion({Src, 0, Width});

  auto *CurrInst = &I;
  // Reduction result is a scalar value
  while (CurrInst && CurrInst->getType()->isVectorTy()) {
    if (CurrInst->hasOneUse()) {
      // Try to match the non-power-of-two reduction step.
      CurrInst = matchNextNonPowerOfTwoStep(CurrInst, SrcDesc);
    } else {
      // Try to match the next reduction step.
      CurrInst = matchNextStep(CurrInst);
    }
  }

  if (!CurrInst)
    return {nullptr, nullptr};

  // There might be the last reduction step on scalar values, when the input
  // vector has odd number of elements
  if (auto *NextInst = matchNextNonPowerOfTwoStep(CurrInst, SrcDesc))
    CurrInst = NextInst;

  IGC_ASSERT(areSameOperations(CurrInst, &I));

  auto *VTy = cast<IGCLLVM::FixedVectorType>(SrcDesc.Src->getType());
  auto SrcWidth = VTy->getNumElements();

  if (SrcDesc.NumElts != SrcWidth) {
    // This is a reduction of a vector part, create a new rdregion to extract
    // the source.
    auto *ReduceTy =
        IGCLLVM::FixedVectorType::get(VTy->getElementType(), SrcDesc.NumElts);

    vc::CMRegion R(ReduceTy, DL);
    R.Offset = SrcDesc.Idx * R.ElementBytes;

    auto *RdRegion =
        R.createRdRegion(SrcDesc.Src, "", CurrInst, CurrInst->getDebugLoc());
    return {RdRegion, CurrInst};
  }

  return {SrcDesc.Src, CurrInst};
}
