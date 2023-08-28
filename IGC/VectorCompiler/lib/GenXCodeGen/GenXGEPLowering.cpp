/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/// GenXGEPLowering
/// ---------------
///
/// GenXGEPLowering is a function pass that lowers GEP instructions into
/// primitive ones that the rest of the GenX backend can deal with.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXModule.h"
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/TypeSize.h"

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>

#define DEBUG_TYPE "GENX_GEPLowering"

using namespace llvm;
using namespace genx;

namespace {
class GenXGEPLowering : public FunctionPass,
                        public InstVisitor<GenXGEPLowering, Value *> {
  const DataLayout *DL = nullptr;
  const TargetTransformInfo *TTI = nullptr;

  LoopInfo *LI = nullptr;
  IRBuilder<> *Builder = nullptr;
  Module *M = nullptr;

public:
  static char ID;

  GenXGEPLowering() : FunctionPass(ID) {}

  StringRef getPassName() const override { return "GenX GEP Lowering"; }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
    AU.setPreservesCFG();
    AU.addPreserved<GenXModule>();
    AU.addPreserved<LoopInfoWrapperPass>();
  }

  Value *visitGetElementPtrInst(GetElementPtrInst &GEP);
  Value *visitPtrToIntInst(PtrToIntInst &PTI);
  Value *visitInstruction(Instruction &) { return nullptr; }

private:
  Value *truncExpr(Value *Val, Type *NewTy) const;
  Value *getSExtOrTrunc(Value *, Type *) const;
};

} // namespace

char GenXGEPLowering::ID = 0;
namespace llvm {
void initializeGenXGEPLoweringPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXGEPLowering, "GenXGEPLowering", "GenXGEPLowering",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(GenXGEPLowering, "GenXGEPLowering", "GenXGEPLowering",
                    false, false)

FunctionPass *llvm::createGenXGEPLoweringPass() {
  initializeGenXGEPLoweringPass(*PassRegistry::getPassRegistry());
  return new GenXGEPLowering;
}

bool GenXGEPLowering::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "Before GEPLowering\n");
  LLVM_DEBUG(F.dump());
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  M = F.getParent();
  DL = &M->getDataLayout();

  TTI = &getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);

  IRBuilder<> TheBuilder(F.getContext());
  Builder = &TheBuilder;

  bool Modified = false;

  for (auto &BB : F)
    for (auto BI = BB.begin(); BI != BB.end();) {
      auto &Inst = *BI++;
      Builder->SetInsertPoint(&Inst);
      Modified |= visit(Inst) != nullptr;
    }

  LLVM_DEBUG(dbgs() << "After GEPLowering\n");
  LLVM_DEBUG(F.dump());

  Builder = nullptr;

  return Modified;
}

Value *GenXGEPLowering::visitPtrToIntInst(PtrToIntInst &PTI) {
  IGC_ASSERT(Builder);
  IGC_ASSERT(TTI);
  auto *Src = PTI.getPointerOperand();

  for (auto *Cast = dyn_cast<CastInst>(Src); Cast != nullptr;
       Cast = dyn_cast<CastInst>(Src)) {
    if (isa<AddrSpaceCastInst>(Cast)) {
      auto *PtrTy = cast<PointerType>(Cast->getType());
      auto AddrSpace = PtrTy->getAddressSpace();
      if (AddrSpace != TTI->getFlatAddressSpace())
        break;
      // The `addrspacecast` is just no-op so it can be eliminated
      Src = Cast->getOperand(0);
    } else if (isa<BitCastInst>(Cast)) {
      auto *Arg = Cast->getOperand(0);
      auto *ArgTy = Arg->getType();
      auto *CastTy = Cast->getType();
      if (ArgTy->isVectorTy() != CastTy->isVectorTy())
        break;
      // The `bitcast` is just no-op so it can be eliminated
      Src = Arg;
    } else {
      // We found `inttoptr`, getting rid of `ptrtoint`
      if (isa<IntToPtrInst>(Cast))
        Src = Cast->getOperand(0);
      break;
    }
  }

  IGC_ASSERT(Src);
  if (Src == PTI.getPointerOperand())
    return nullptr; // Nothing to do with `ptrtoint`

  auto *SrcTy = Src->getType();
  auto *Ty = PTI.getType();
  Value *NewI = nullptr;

  if (SrcTy->isPointerTy()) {
    NewI = Builder->CreatePtrToInt(Src, Ty);
    NewI->takeName(&PTI);
  } else {
    IGC_ASSERT(SrcTy->isIntegerTy());
    NewI = Builder->CreateIntCast(Src, Ty, false);
  }

  PTI.replaceAllUsesWith(NewI);
  PTI.eraseFromParent();
  return NewI;
}

Value *GenXGEPLowering::visitGetElementPtrInst(GetElementPtrInst &GEP) {
  IGC_ASSERT(Builder);
  Value *PtrOp = GEP.getPointerOperand();

  Value *PointerValue = nullptr;
  PointerType *PtrTy = dyn_cast<PointerType>(PtrOp->getType());
  Type *IntPtrTy = nullptr;
  if (PtrTy) {
    IntPtrTy =
        DL->getIntPtrType(Builder->getContext(), PtrTy->getAddressSpace());
  } else {
    IGC_ASSERT(PtrOp->getType()->isVectorTy());
    auto PtrOpVTy = cast<IGCLLVM::FixedVectorType>(PtrOp->getType());
    PtrTy = cast<PointerType>(PtrOpVTy->getElementType());
    IntPtrTy =
        DL->getIntPtrType(Builder->getContext(), PtrTy->getAddressSpace());
    IntPtrTy =
        IGCLLVM::FixedVectorType::get(IntPtrTy, PtrOpVTy->getNumElements());
  }

  PointerValue = Builder->CreatePtrToInt(PtrOp, IntPtrTy);
  if (auto *PTI = dyn_cast<PtrToIntInst>(PointerValue))
    if (auto *NewPTI = visitPtrToIntInst(*PTI))
      PointerValue = NewPTI;

  unsigned PtrMathSizeInBits =
      DL->getPointerSizeInBits(PtrTy->getAddressSpace());
  auto *PtrMathTy = IntegerType::get(Builder->getContext(), PtrMathSizeInBits);

  auto *GEPVecTy = dyn_cast<IGCLLVM::FixedVectorType>(GEP.getType());
  if (GEPVecTy && isa<PointerType>(PtrOp->getType())) {
    PointerValue = Builder->CreateVectorSplat(
        GEPVecTy->getNumElements(), PointerValue, PtrOp->getName() + ".splat");
  }

  Type *Ty = PtrOp->getType();
  gep_type_iterator GTI = gep_type_begin(GEP);
  for (auto OI = GEP.op_begin() + 1, E = GEP.op_end(); OI != E; ++OI, ++GTI) {
    Value *Idx = *OI;
    if (StructType *StTy = GTI.getStructTypeOrNull()) {
      int64_t Field = 0;
      Constant *CE = cast<Constant>(Idx);
      if (!CE->isNullValue()) {
        Field = CE->getUniqueInteger().getSExtValue();
        uint64_t Offset = DL->getStructLayout(StTy)->getElementOffset(Field);
        Value *OffsetVal = Constant::getIntegerValue(
          PointerValue->getType(), APInt(PtrMathSizeInBits, Offset));
        PointerValue = Builder->CreateAdd(PointerValue, OffsetVal);
      }

      Ty = StTy->getElementType(Field);
    } else {
      Ty = GTI.getIndexedType();
      if (const ConstantInt *CI = dyn_cast<ConstantInt>(Idx)) {
        if (!CI->isZero()) {
          uint64_t Offset = DL->getTypeAllocSize(Ty) * CI->getSExtValue();
          Value *OffsetVal = Builder->getInt(APInt(PtrMathSizeInBits, Offset));
          PointerValue = Builder->CreateAdd(PointerValue, OffsetVal);
        }
      } else if (!isa<ConstantAggregateZero>(Idx)) {
        Value *NewIdx = getSExtOrTrunc(Idx, PtrMathTy);
        APInt ElementSize = APInt(PtrMathSizeInBits, DL->getTypeAllocSize(Ty));

        if (BinaryOperator *BO = dyn_cast<BinaryOperator>(NewIdx)) {
          // Detect the pattern GEP base, a + b where base and a are both loop
          // invariant (but not b), so we could rearrange the lowered code into
          // (base + (a << shftAmt)) + (b << shftAmt).
          Loop *L = LI ? LI->getLoopFor(BO->getParent()) : nullptr;
          if (L && L->isLoopInvariant(GEP.getPointerOperand()) &&
              BO->getOpcode() == Instruction::Add) {

            auto reassociate = [&](Value *A, Value *B) {
              Value *InvVal = nullptr;
              if (ElementSize == 1)
                InvVal = A;
              else if (ElementSize.isPowerOf2())
                InvVal = Builder->CreateShl(
                    A, Constant::getIntegerValue(
                           A->getType(),
                           APInt(PtrMathSizeInBits, ElementSize.logBase2())));
              else
                InvVal = Builder->CreateMul(
                    A, Constant::getIntegerValue(A->getType(), ElementSize));
              PointerValue = Builder->CreateAdd(PointerValue, InvVal);
              NewIdx = B;
            };

            Value *LHS = BO->getOperand(0);
            Value *RHS = BO->getOperand(1);
            bool isLHSLI = L->isLoopInvariant(LHS);
            bool isRHSLI = L->isLoopInvariant(RHS);
            if (isLHSLI && !isRHSLI)
              reassociate(LHS, RHS);
            else if (!isLHSLI && isRHSLI)
              reassociate(RHS, LHS);
          }
        }
        if (ElementSize == 1) {
          // DO NOTHING.
        } else if (ElementSize.isPowerOf2()) {
          APInt ShiftAmount = APInt(PtrMathSizeInBits, ElementSize.logBase2());
          NewIdx = Builder->CreateShl(
              NewIdx,
              Constant::getIntegerValue(NewIdx->getType(), ShiftAmount));
        } else
          NewIdx = Builder->CreateMul(
              NewIdx,
              Constant::getIntegerValue(NewIdx->getType(), ElementSize));

        PointerValue = Builder->CreateAdd(PointerValue, NewIdx);
      }
    }
  }

  PointerValue = Builder->CreateIntToPtr(PointerValue, GEP.getType());
  PointerValue->takeName(&GEP);
  GEP.replaceAllUsesWith(PointerValue);
  GEP.eraseFromParent();
  return PointerValue;
}

Value *GenXGEPLowering::getSExtOrTrunc(Value *Val, Type *NewTy) const {
  IGC_ASSERT(Builder);
  unsigned NewWidth = NewTy->getIntegerBitWidth();
  Type *OldTy = Val->getType();
  if (auto *OldVecTy = dyn_cast<IGCLLVM::FixedVectorType>(OldTy)) {
    NewTy = IGCLLVM::FixedVectorType::get(NewTy, OldVecTy->getNumElements());
    OldTy = OldVecTy->getElementType();
  }
  unsigned OldWidth = OldTy->getIntegerBitWidth();

  if (OldWidth < NewWidth) // SExt
    return Builder->CreateSExt(Val, NewTy);
  if (OldWidth > NewWidth) // Trunc
    return truncExpr(Val, NewTy);
  return Val;
}

Value *GenXGEPLowering::truncExpr(Value *Val, Type *NewTy) const {
  IGC_ASSERT(Builder);
  // Truncation on Gen could be as cheap as NOP by creating proper regions.
  // Instead of truncating the value itself, truncate how it's calculated.
  if (Constant *C = dyn_cast<Constant>(Val))
    return Builder->CreateIntCast(C, NewTy, false);

  if (!isa<Instruction>(Val))
    return Builder->CreateTrunc(Val, NewTy);

  Instruction *I = cast<Instruction>(Val);
  unsigned Opc = I->getOpcode();
  switch (Opc) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor: {
    BinaryOperator *BO = cast<BinaryOperator>(I);
    Value *LHS = truncExpr(BO->getOperand(0), NewTy);
    Value *RHS = truncExpr(BO->getOperand(1), NewTy);
    return Builder->CreateBinOp(BO->getOpcode(), LHS, RHS);
  }
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt: {
    Value *Opnd = I->getOperand(0);
    if (Opnd->getType() == NewTy)
      return Opnd;
    return Builder->CreateIntCast(Opnd, NewTy, Opc == Instruction::SExt);
  }
  case Instruction::Select: {
    Value *TVal = truncExpr(I->getOperand(1), NewTy);
    Value *FVal = truncExpr(I->getOperand(2), NewTy);
    return Builder->CreateSelect(I->getOperand(0), TVal, FVal);
  }
#if 0
  // TODO: Rewrite truncExpr into iterative one instead of recursive one to
  // easily found the loop due to phi-node.
  case Instruction::PHI: {
    PHINode *PN = cast<PHINode>(I);
    PHINode *Res = PHINode::Create(NewTy, PN->getNumIncomingValues());
    for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
      Value *V = truncExpr(PN->getIncomingValue(i), NewTy);
      Res->addIncoming(V, PN->getIncomingBlock(i));
    }
    return Res;
  }
#endif
  default:
    // Don't know truncate its calculation safely, fall back to the regular way.
    break;
  }

  return Builder->CreateTrunc(Val, NewTy);
}
