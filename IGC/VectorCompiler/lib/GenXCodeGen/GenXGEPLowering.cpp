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
/// GenXGEPLowering
/// ---------------
///
/// GenXGEPLowering is a function pass that lowers GEP instructions into
/// primitive ones that the rest of the GenX backend can deal with.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXModule.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"

using namespace llvm;
using namespace genx;

namespace {
class GenXGEPLowering : public FunctionPass {
  const DataLayout *DL = nullptr;
  LoopInfo *LI = nullptr;
  IRBuilder<> *Builder = nullptr;

public:
  static char ID;

  GenXGEPLowering() : FunctionPass(ID) {}

  virtual StringRef getPassName() const override { return "GenX GEP Lowering"; }

  virtual bool runOnFunction(Function &F) override;

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
    AU.setPreservesCFG();
    AU.addPreserved<GenXModule>();
    AU.addPreserved<LoopInfoWrapperPass>();
  }

private:
  bool lowerGetElementPtrInst(GetElementPtrInst *GEP,
                              BasicBlock::iterator &BBI) const;
  Value *truncExpr(Value *Val, Type *NewTy) const;
  Value *getSExtOrTrunc(Value *, Type *) const;
};

} // namespace

char GenXGEPLowering::ID = 0;
namespace llvm { void initializeGenXGEPLoweringPass(PassRegistry &); }
INITIALIZE_PASS_BEGIN(GenXGEPLowering, "GenXGEPLowering", "GenXGEPLowering", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(GenXGEPLowering, "GenXGEPLowering", "GenXGEPLowering", false, false)

FunctionPass *llvm::createGenXGEPLoweringPass() {
  initializeGenXGEPLoweringPass(*PassRegistry::getPassRegistry());
  return new GenXGEPLowering;
}

bool GenXGEPLowering::runOnFunction(Function &F) {
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DL = &F.getParent()->getDataLayout();

  const TargetTransformInfo &TTI =
    getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
  auto FlatAddrSpace = TTI.getFlatAddressSpace();

  assert(DL && "null datalayout");
#if 0
  // a good place to fix block layout
  if (LI->empty())
    LayoutBlocks(F);
  else
    LayoutBlocks(F, *LI);
#endif
  IRBuilder<> TheBuilder(F.getContext());
  Builder = &TheBuilder;

  bool Changed = false;
  for (auto &BB : F) {
    for (auto BI = BB.begin(), BE = BB.end(); BI != BE;) {
      Instruction *Inst = &(*BI++);
      Builder->SetInsertPoint(Inst);

      switch (Inst->getOpcode()) {
      default: // By default, DO NOTHING
        break;
      case Instruction::GetElementPtr:
        Changed |= lowerGetElementPtrInst(cast<GetElementPtrInst>(Inst), BI);
        break;
      case Instruction::PtrToInt:
        auto PtrV = cast<PtrToIntInst>(Inst)->getPointerOperand();
        auto AddrSpace = cast<PtrToIntInst>(Inst)->getPointerAddressSpace();
        if (AddrSpace == FlatAddrSpace) {
          if (auto PtrCast = dyn_cast<AddrSpaceCastInst>(PtrV)) {
            // this is no-op AddrSpaceCast, should be removed
            // create a new PtrToInt from the original pointer
            // bypass the AddrSpaceCast and PtrToInt
            auto P2I = Builder->CreatePtrToInt(PtrCast->getOperand(0), Inst->getType());
            Inst->replaceAllUsesWith(P2I);
            Inst->eraseFromParent();
            if (PtrCast->use_empty()) {
              PtrCast->eraseFromParent();
            }
          }
        }
        break;
      }
    }
  }
  Builder = nullptr;

  return Changed;
}

bool GenXGEPLowering::lowerGetElementPtrInst(GetElementPtrInst *GEP,
                                             BasicBlock::iterator &BBI) const {
  assert(Builder);
  Value *PtrOp = GEP->getPointerOperand();
  PointerType *PtrTy = dyn_cast<PointerType>(PtrOp->getType());
  assert(PtrTy && "Only accept scalar pointer!");

  unsigned PtrSizeInBits = DL->getPointerSizeInBits(PtrTy->getAddressSpace());
  unsigned PtrMathSizeInBits = PtrSizeInBits;
  auto IntPtrTy = IntegerType::get(Builder->getContext(), PtrSizeInBits);
  auto PtrMathTy = IntegerType::get(Builder->getContext(), PtrMathSizeInBits);

  // Check if the pointer itself is created from IntToPtr. If it is, and if
  // the int is the same size, we can use the int directly. Otherwise, we
  // need to add PtrToInt.
  Value *BasePointer = nullptr;
  if (IntToPtrInst *I2PI = dyn_cast<IntToPtrInst>(PtrOp)) {
    Value *IntOp = I2PI->getOperand(0);
    if (IntOp->getType() == IntPtrTy)
      BasePointer = IntOp;
  }
  if (!BasePointer)
    BasePointer = Builder->CreatePtrToInt(PtrOp, IntPtrTy);

  // This is the value of the pointer, which will ultimately replace gep.
  Value *PointerValue = BasePointer;

  Type *Ty = PtrTy;
  gep_type_iterator GTI = gep_type_begin(GEP);
  for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI, ++GTI) {
    Value *Idx = *OI;
    if (StructType *StTy = GTI.getStructTypeOrNull()) {
      unsigned Field = unsigned(cast<ConstantInt>(Idx)->getZExtValue());
      if (Field) {
        uint64_t Offset = DL->getStructLayout(StTy)->getElementOffset(Field);
        Value *OffsetVal = Builder->getInt(APInt(PtrMathSizeInBits, Offset));
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
      } else {
        Value *NewIdx = getSExtOrTrunc(Idx, PtrMathTy);
        APInt ElementSize = APInt(PtrMathSizeInBits, DL->getTypeAllocSize(Ty));

        if (BinaryOperator *BO = dyn_cast<BinaryOperator>(NewIdx)) {
          // Detect the pattern GEP base, a + b where base and a are both loop
          // invariant (but not b), so we could rearrange the lowered code into
          // (base + (a << shftAmt)) + (b << shftAmt).
          Loop *L = LI ? LI->getLoopFor(BO->getParent()) : nullptr;
          if (L && L->isLoopInvariant(PtrOp) &&
              BO->getOpcode() == Instruction::Add) {

            auto reassociate = [&](Value *A, Value *B) {
              Value *InvVal = nullptr;
              if (ElementSize == 1)
                InvVal = A;
              else if (ElementSize.isPowerOf2())
                InvVal = Builder->CreateShl(
                    A, APInt(PtrMathSizeInBits, ElementSize.logBase2()));
              else
                InvVal = Builder->CreateMul(A, Builder->getInt(ElementSize));
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
          NewIdx = Builder->CreateShl(NewIdx, ShiftAmount);
        } else
          NewIdx = Builder->CreateMul(NewIdx, Builder->getInt(ElementSize));

        PointerValue = Builder->CreateAdd(PointerValue, NewIdx);
      }
    }
  }

  PointerValue = Builder->CreateIntToPtr(PointerValue, GEP->getType());
  GEP->replaceAllUsesWith(PointerValue);
  GEP->eraseFromParent();
  if (Instruction *I = dyn_cast<Instruction>(PointerValue)) {
    BBI = BasicBlock::iterator(I);
    ++BBI;
  }

  return true;
}

Value *GenXGEPLowering::getSExtOrTrunc(Value *Val, Type *NewTy) const {
  assert(Builder);
  Type *OldTy = Val->getType();
  unsigned OldWidth = OldTy->getIntegerBitWidth();
  unsigned NewWidth = NewTy->getIntegerBitWidth();

  if (OldWidth < NewWidth) // SExt
    return Builder->CreateSExt(Val, NewTy);
  if (OldWidth > NewWidth) // Trunc
    return truncExpr(Val, NewTy);
  return Val;
}

Value *GenXGEPLowering::truncExpr(Value *Val, Type *NewTy) const {
  assert(Builder);
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
