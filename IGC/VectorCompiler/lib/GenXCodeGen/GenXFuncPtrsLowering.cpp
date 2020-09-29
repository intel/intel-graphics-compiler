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
///
/// GenXFunctionPointersLowering
/// ---------------------
///
/// Function pointers lowering consists of several stages:
/// 1. Collect all instruction that use function pointers and their users that
///    have to be modified
/// 2. Actually modify the instructions collected:
///    - break constant expression in the instructions collected to simplify the
///      analysis
///    - reconstruct existing wrr/rdr instrinsics (remove internal casts, use
///    i64
///      types)
///    - create new wrr/rdrs where necessary, e.g. as a select args for further
///      baling to succeed
///    - reconstruct all funcptrs-related phis
///    - update all users of the instruction modified (may insert additional
///      casts where necessary, e.g. inttoptr for wrr to indirect call)
/// 3. Replace all function pointer PtrToIntInsts to genx.faddr.
///    This is done mainly to prevent them being transformed back into
///    ConstantExprs which prevents bailing for sure, and maybe some
///    other analyzes/transformations as well
/// 4. Sweep all the dead code involved in the function pointers flow,
///    e.g. InsertElementInsts replaced with wrrs
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXModule.h"
#include "GenXRegion.h"
#include "GenXUtil.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include <deque>

using namespace llvm;
using namespace genx;

namespace {

class GenXFunctionPointersLowering : public ModulePass {
  SetVector<Instruction *> InstToProcess;
  std::vector<Instruction *> ToErase;

  const DataLayout *DL = nullptr;
  LLVMContext *Ctx = nullptr;

  void collectFuncUsers(User *U);
  void collectFuncUsers(CallInst *CI);
  void collectFuncUsers(PHINode *Phi);
  void collectFuncUsers(CastInst *Phi);
  void collectFuncUsers(SelectInst *SI);

  void reconstructGenXIntrinsic(CallInst *CI);
  void reconstructPhi(PHINode *Phi);
  void reconstructSelect(SelectInst *SI);
  Value *reconstructValue(Value *V, Instruction *InsPoint);

  void replaceAllUsersCommon(Instruction *Old, Instruction *New);

public:
  static char ID;
  explicit GenXFunctionPointersLowering();
  StringRef getPassName() const override {
    return "GenX function pointers lowering";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;
};

} // namespace

char GenXFunctionPointersLowering::ID = 0;
namespace llvm {
void initializeGenXFunctionPointersLoweringPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXFunctionPointersLowering,
                      "GenXFunctionPointersLowering",
                      "GenXFunctionPointersLowering", false, false)
INITIALIZE_PASS_END(GenXFunctionPointersLowering,
                    "GenXFunctionPointersLowering",
                    "GenXFunctionPointersLowering", false, false)

GenXFunctionPointersLowering::GenXFunctionPointersLowering() : ModulePass(ID) {
  initializeGenXFunctionPointersLoweringPass(*PassRegistry::getPassRegistry());
}

ModulePass *llvm::createGenXFunctionPointersLoweringPass() {
  return new GenXFunctionPointersLowering();
}

void GenXFunctionPointersLowering::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
}

bool GenXFunctionPointersLowering::runOnModule(Module &M) {
  bool Modified = false;

  for (auto &F : M)
    if (F.hasAddressTaken()) {
      F.addFnAttr(genx::FunctionMD::CMStackCall);
      F.addFnAttr(genx::FunctionMD::ReferencedIndirectly);
    }

  for (auto &F : M)
    if (F.hasFnAttribute(genx::FunctionMD::ReferencedIndirectly))
      for (auto *U : F.users())
        collectFuncUsers(U);

  Ctx = &M.getContext();
  DL = &M.getDataLayout();
  for (auto *TI : InstToProcess) {
    if (auto *Phi = dyn_cast<PHINode>(TI))
      reconstructPhi(Phi);
    else if (auto *CI = dyn_cast<CallInst>(TI))
      reconstructGenXIntrinsic(CI);
    else if (auto *SI = dyn_cast<SelectInst>(TI))
      reconstructSelect(SI);
    else
      IGC_ASSERT(0 && "Unsupported instruction to process");
  }

  for (auto &F : M)
    if (F.hasFnAttribute(genx::FunctionMD::ReferencedIndirectly))
      for (auto *U : F.users()) {
        if (auto *UI = dyn_cast<PtrToIntInst>(U)) {
          Function *IntrDecl = GenXIntrinsic::getGenXDeclaration(
              &M, llvm::GenXIntrinsic::genx_faddr, {F.getType()});
          auto *IntrCall = CallInst::Create(IntrDecl, {&F}, "faddr", UI);
          UI->replaceAllUsesWith(IntrCall);
          ToErase.push_back(UI);
        } else if (auto *UCE = dyn_cast<ConstantExpr>(U);
                   !(UCE && UCE->getOpcode() == Instruction::PtrToInt))
          IGC_ASSERT(isa<CallInst>(U) &&
                     "Unsupported first-level user of a function");
      }
  for (auto *I : ToErase)
    I->eraseFromParent();
  return Modified;
}

// TODO: redesign this function
void GenXFunctionPointersLowering::collectFuncUsers(User *U) {
  if (auto *CI = dyn_cast<CallInst>(U))
    collectFuncUsers(CI);
  else if (auto *C = dyn_cast<CastInst>(U))
    collectFuncUsers(C);
  else if (auto *Phi = dyn_cast<PHINode>(U))
    collectFuncUsers(Phi);
  else if (auto *SI = dyn_cast<SelectInst>(U))
    collectFuncUsers(SI);
  else if (auto *ICmp = dyn_cast<ICmpInst>(U)) {
    // skip
  } else if (auto *EE = dyn_cast<ExtractElementInst>(U))
    collectFuncUsers(EE);
  else if (isa<Constant>(U))
    for (auto *UU : U->users())
      collectFuncUsers(UU);
  else
    IGC_ASSERT(0 && "unsupported funcptr user");
}

void GenXFunctionPointersLowering::collectFuncUsers(CallInst *CI) {
  if (!IGCLLVM::isIndirectCall(*CI)) {
    InstToProcess.insert(CI);

    for (auto *U : CI->users())
      collectFuncUsers(U);
  }
}

// do not process bitcast itself, after our transformations
// it should become dead and will be swept
void GenXFunctionPointersLowering::collectFuncUsers(CastInst *BC) {
  for (auto *U : BC->users())
    collectFuncUsers(U);
}

void GenXFunctionPointersLowering::collectFuncUsers(PHINode *Phi) {
  InstToProcess.insert(Phi);

  for (auto *U : Phi->users())
    collectFuncUsers(U);
}

void GenXFunctionPointersLowering::collectFuncUsers(SelectInst *SI) {
  InstToProcess.insert(SI);

  if (!SI->getType()->getScalarType()->isIntegerTy(64))
    for (auto *U : SI->users())
      collectFuncUsers(U);
}

void GenXFunctionPointersLowering::replaceAllUsersCommon(Instruction *Old,
                                                         Instruction *New) {
  while (!Old->use_empty()) {
    auto *U = Old->user_back();
    if (auto *CIU = dyn_cast<CallInst>(U)) {
      if (IGCLLVM::getCalledValue(CIU) == Old) {
        if (New->getType()->isVectorTy()) {
          IGC_ASSERT(New->getType()->getVectorNumElements() == 1);
          New = CastInst::CreateBitOrPointerCast(
              New, New->getType()->getVectorElementType(), "", CIU);
          New->setDebugLoc(CIU->getDebugLoc());
        }
        auto *IntToPtr = CastInst::CreateBitOrPointerCast(
            New, IGCLLVM::getCalledValue(CIU)->getType(), "", CIU);
        IntToPtr->setDebugLoc(Old->getDebugLoc());
        CIU->replaceUsesOfWith(Old, IntToPtr);
      } else if (GenXIntrinsic::getGenXIntrinsicID(CIU->getCalledFunction()) ==
                     GenXIntrinsic::genx_rdregioni ||
                 GenXIntrinsic::getGenXIntrinsicID(CIU->getCalledFunction()) ==
                     GenXIntrinsic::genx_wrregioni ||
                 IGCLLVM::getCalledValue(CIU) != Old) {
        CIU->replaceUsesOfWith(Old, New);
      } else
        IGC_ASSERT(0 && "unsupported call of a function pointer");
    } else if (isa<IntToPtrInst>(U) || isa<ICmpInst>(U) ||
               isa<BitCastInst>(U) || isa<PHINode>(U)) {
      U->replaceUsesOfWith(Old, New);
    } else {
      IGC_ASSERT(0 && "Unsupported function pointer user\n");
    }
  }
  Old->eraseFromParent();
}

void GenXFunctionPointersLowering::reconstructGenXIntrinsic(CallInst *CI) {
  IGC_ASSERT_MESSAGE(GenXIntrinsic::isGenXIntrinsic(CI->getCalledFunction()),
                     "Unsupported call to process");
  genx::breakConstantExprs(CI);
  unsigned OpIdx = 0;
  switch (GenXIntrinsic::getGenXIntrinsicID(CI->getCalledFunction())) {
  case GenXIntrinsic::genx_rdregioni:
    OpIdx = GenXIntrinsic::GenXRegion::OldValueOperandNum;
    break;
  case GenXIntrinsic::genx_wrregioni:
    OpIdx = GenXIntrinsic::GenXRegion::NewValueOperandNum;
    break;
  case GenXIntrinsic::genx_scatter_scaled:
    OpIdx = 6;
    break;
  case GenXIntrinsic::genx_scatter_private:
    OpIdx = 3;
    break;
  default:
    IGC_ASSERT(0 && "Unsupported genx intrinsic");
  }
  reconstructValue(CI->getOperand(OpIdx), CI);
}

void GenXFunctionPointersLowering::reconstructSelect(SelectInst *SI) {
  genx::breakConstantExprs(SI);
  auto *OrigTy = SI->getType();
  auto *TV = reconstructValue(SI->getTrueValue(), SI);
  auto *FV = reconstructValue(SI->getFalseValue(), SI);
  IGC_ASSERT(TV && FV);
  if (TV->getType() != OrigTy) {
    IGC_ASSERT(
        OrigTy->getScalarType()->isPointerTy() &&
        OrigTy->getScalarType()->getPointerElementType()->isFunctionTy());
    Instruction *NewSel =
        SelectInst::Create(SI->getCondition(), TV, FV, SI->getName(), SI);
    replaceAllUsersCommon(SI, NewSel);
  }
}

void GenXFunctionPointersLowering::reconstructPhi(PHINode *Phi) {
  genx::breakConstantExprs(Phi);
  for (unsigned i = 0; i < Phi->getNumIncomingValues(); i++) {
    auto *Op = Phi->getIncomingValue(i);
    auto *OpTr = reconstructValue(Op, &Phi->getIncomingBlock(i)->back());
    Phi->replaceUsesOfWith(Op, OpTr);
  }
}

Value *GenXFunctionPointersLowering::reconstructValue(Value *V,
                                                      Instruction *InsPoint) {
  if (isa<Function>(V)) {
    Value *Result = CastInst::CreateBitOrPointerCast(V, Type::getInt64Ty(*Ctx),
                                                     "", InsPoint);
    cast<Instruction>(Result)->setDebugLoc(InsPoint->getDebugLoc());
    auto *VecTy = VectorType::get(Result->getType(), 1);
    Region R(VecTy);
    Result = R.createWrRegion(UndefValue::get(VecTy), Result, "", InsPoint,
                              InsPoint->getDebugLoc());
    return Result;
  }
  if (auto *C = dyn_cast<Constant>(V); C && C->isZeroValue())
    return V;
  auto *OrigV = V;

  Instruction *Inst = cast<Instruction>(OrigV);
  bool hasShuffle = false;
  while (isa<BitCastInst>(Inst) || isa<ShuffleVectorInst>(Inst)) {
    hasShuffle |= isa<ShuffleVectorInst>(Inst);
    Inst = cast<Instruction>(Inst->getOperand(0));
  }
  InsPoint = Inst;
  if (!isa<InsertElementInst>(Inst))
    return V;
  std::deque<Instruction *> Worklist;
  std::vector<ExtractElementInst *> Extra;
  unsigned Scale = 0;
  while (Inst) {
    if (auto *EEInst = dyn_cast<ExtractElementInst>(Inst->getOperand(1))) {
      Extra.push_back(EEInst);
      ToErase.push_back(Inst);
    } else
      Worklist.push_front(Inst);
    Inst = dyn_cast<InsertElementInst>(Inst->getOperand(0));
  }
  // TODO: replace extractelems from the same function/splat vector
  // with a shufflevector
  for (auto *EEInst : llvm::reverse(Extra)) {
    Scale = genx::QWordBytes /
            (DL->getTypeSizeInBits(EEInst->getType()) / genx::ByteBits);
    IGC_ASSERT(Scale);
    if (auto *Idx = dyn_cast<Constant>(EEInst->getIndexOperand());
        Idx && Idx->getUniqueInteger().getZExtValue() % Scale == 0) {
      auto *VecOper =
          (Worklist.empty()
               ? UndefValue::get(VectorType::get(
                     IntegerType::get(*Ctx,
                                      DL->getTypeSizeInBits(EEInst->getType()) *
                                          Scale),
                     OrigV->getType()->getVectorNumElements() / Scale))
               : cast<Value>(Worklist.back()));
      auto *FptrF = getFunctionPointerFunc(EEInst->getVectorOperand());
      IGC_ASSERT(FptrF);
      auto *PTI = CastInst::CreateBitOrPointerCast(FptrF,
            Type::getInt64Ty(*Ctx), "", EEInst);
      IGC_ASSERT(PTI && "CreateBitOrPointerCast failed!");
      PTI->setDebugLoc(EEInst->getDebugLoc());
      Instruction *NewInsElem = InsertElementInst::Create(
          VecOper, PTI,
          ConstantExpr::getURem(
              ConstantExpr::getUDiv(Idx,
                                    ConstantInt::get(Idx->getType(), Scale)),
              ConstantInt::get(Idx->getType(),
                               VecOper->getType()->getVectorNumElements())),
          "", EEInst);
      Worklist.push_back(NewInsElem);
      ToErase.push_back(EEInst);
    } else
      ToErase.push_back(EEInst);
  }
  if (Scale > 1) {
    IGC_ASSERT(!Worklist.empty());
    auto *NewInsElem = CastInst::CreateBitOrPointerCast(
        Worklist.back(), OrigV->getType(), "recast", InsPoint);
    NewInsElem->setDebugLoc(InsPoint->getDebugLoc());
    InsPoint = NewInsElem;
    OrigV->replaceAllUsesWith(NewInsElem);
  }
  Instruction *Result = nullptr;
  for (auto *I : Worklist) {
    // ?? to remove
    if (isa<BitCastInst>(I))
      I = cast<Instruction>(I->getOperand(0));
    Region R(I->getOperand(1));
    R.Indirect = BinaryOperator::CreateMul(
        I->getOperand(2),
        ConstantInt::get(I->getOperand(2)->getType(), genx::QWordBytes), "", I);
    if (!R.Indirect->getType()->isIntegerTy(16)) {
      R.Indirect = CastInst::CreateTruncOrBitCast(
          R.Indirect, Type::getInt16Ty(*Ctx), "", I);
      cast<Instruction>(R.Indirect)->setDebugLoc(I->getDebugLoc());
    }
    Result = cast<Instruction>(R.createWrRegion(
        I->getOperand(0), I->getOperand(1), I->getName() + ".fptr_mem",
        InsPoint, I->getDebugLoc()));
    I->replaceAllUsesWith(Result);
    I->eraseFromParent();
  }
  if (hasShuffle) {
    IGC_ASSERT(Result && Result->hasOneUse());
    auto *Shuffle = dyn_cast<ShuffleVectorInst>(Result->user_back());
    // we support only splat values for now
    IGC_ASSERT(Shuffle && Shuffle->getMask()->isZeroValue());

    Region R(Result);
    R.Stride = 0;
    R.Indirect = 0;
    auto *I64Ty = Type::getInt64Ty(*Ctx);
    auto V1_64Ty = VectorType::get(I64Ty, 1);
    auto NewR = Region(V1_64Ty);
    auto *NewWRR =
        NewR.createWrRegion(UndefValue::get(V1_64Ty), Result->getOperand(1),
                            Result->getName(), Shuffle, Result->getDebugLoc());
    ToErase.push_back(Result);
    Result = R.createRdRegion(NewWRR, Shuffle->getName() + ".fptr_shuf",
                              Shuffle, Shuffle->getDebugLoc());
    Shuffle->replaceAllUsesWith(Result);
    Shuffle->eraseFromParent();
  }
  IGC_ASSERT(Result);
  return Result;
}
