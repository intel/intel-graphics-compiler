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
//
/// This pass lowers all function pointers related instructions
//
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXModule.h"
#include "GenXRegion.h"
#include "GenXUtil.h"
#include "llvmWrapper/IR/InstrTypes.h"
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

using namespace llvm;
using namespace genx;

namespace {

// Function pointers lowering consists of two stages:
// 1. Collect all instruction that use function pointers and their users that
// have to be modified
// 2. Actually modify the instructions collected:
//    - reconstruct existing wrr/rdr instrinsics (remove internal casts, use i64
//    types)
//    - create new wrr/rdrs where necessary, e.g. as a select args for further
//    baling to succeed
//    - reconstruct all funcptrs-related phis
//    - update all users of the instruction modified (may insert additional
//    casts where necessary,
//                                                    e.g. ptrtoint for wrr to
//                                                    indirect call)

class GenXFunctionPointersLowering : public ModulePass {
  SetVector<Instruction *> InstToProcess;
  std::map<PHINode *, unsigned> PhisIndex;

  const DataLayout *DL = nullptr;
  LLVMContext *Ctx = nullptr;

  bool IsFuncPointerVec(Value *V, SetVector<Function *> *Funcs = nullptr);

  void collectFuncUsers(User *U);
  void collectFuncUsers(IGCLLVM::CallInst *CI);
  void collectFuncUsers(PHINode *Phi);
  void collectFuncUsers(CastInst *Phi);
  void collectFuncUsers(SelectInst *SI);

  void reconstructCall(CallInst *CI);
  void reconstructPhi(PHINode *Phi);
  void reconstructSelect(SelectInst *SI);

  void replaceAllUsersCommon(Instruction *Old, Instruction *New);

  Value *transformFuncPtrVec(Value *V);

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
    if (F.hasFnAttribute("referenced-indirectly"))
      for (auto *U : F.users())
        collectFuncUsers(U);

  Ctx = &M.getContext();
  DL = &M.getDataLayout();
  for (auto *TI : InstToProcess) {
    if (auto *Phi = dyn_cast<PHINode>(TI))
      reconstructPhi(Phi);
    else if (auto *CI = dyn_cast<CallInst>(TI))
      reconstructCall(CI);
    else if (auto *SI = dyn_cast<SelectInst>(TI))
      reconstructSelect(SI);
    else
      assert(0 && "Unsupported instruction to process");
  }
  return Modified;
}

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
  } else if (auto *EE = dyn_cast<ExtractElementInst>(U)) {
    collectFuncUsers(EE);
  } else if (isa<Constant>(U))
    for (auto *UU : U->users())
      collectFuncUsers(UU);
  else {
    assert(0 && "unsupported funcptr user");
  }
}

void GenXFunctionPointersLowering::collectFuncUsers(IGCLLVM::CallInst *CI) {
  if (!CI->isIndirectCall() &&
      (GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
           GenXIntrinsic::genx_rdregioni ||
       GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
           GenXIntrinsic::genx_wrregioni)) {
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
    if (auto *CIU = dyn_cast<IGCLLVM::CallInst>(U)) {
      if (CIU->getCalledOperand() == Old) {
        auto *IntToPtr = CastInst::CreateBitOrPointerCast(
            New, CIU->getCalledOperand()->getType(), "", CIU);
        CIU->replaceUsesOfWith(Old, IntToPtr);
      } else if (GenXIntrinsic::getAnyIntrinsicID(CIU->getCalledFunction()) ==
                     GenXIntrinsic::genx_rdregioni ||
                 GenXIntrinsic::getAnyIntrinsicID(CIU->getCalledFunction()) ==
                     GenXIntrinsic::genx_wrregioni ||
                 CIU->getCalledOperand() != Old) {
        CIU->replaceUsesOfWith(Old, New);
      } else
        assert(0 && "unsupported call of a function pointer");
    } else if (isa<IntToPtrInst>(U) || isa<ICmpInst>(U)) {
      U->replaceUsesOfWith(Old, New);
    } else if (auto *Phi = dyn_cast<PHINode>(U)) {
      Phi->replaceUsesOfWith(Old, New);
      PhisIndex[Phi]++;
    } else {
      assert(0 && "Unsupported function pointer user\n");
    }
  }
  Old->eraseFromParent();
}

void GenXFunctionPointersLowering::reconstructCall(CallInst *CI) {
  assert(GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
             GenXIntrinsic::genx_rdregioni ||
         GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
             GenXIntrinsic::genx_wrregioni);
  Region R(Type::getInt64Ty(*Ctx));
  unsigned OffIdx = GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
                            GenXIntrinsic::genx_rdregioni
                        ? 4
                        : 5;
  if (!isa<Constant>(CI->getOperand(OffIdx)))
    R.Indirect = CI->getOperand(OffIdx);
  else
    R.Offset = cast<ConstantInt>(CI->getOperand(OffIdx))->getZExtValue();
  Instruction *Result = nullptr;
  if (GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
      GenXIntrinsic::genx_rdregioni) {
    Result = cast<Instruction>(
        R.createRdRegion(transformFuncPtrVec(CI->getOperand(0)), CI->getName(),
                         CI, CI->getDebugLoc(), true));
  } else if (GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
             GenXIntrinsic::genx_wrregioni)
    Result = cast<Instruction>(
        R.createWrRegion(transformFuncPtrVec(CI->getOperand(0)),
                         transformFuncPtrVec(CI->getOperand(1)), CI->getName(),
                         CI, CI->getDebugLoc()));
  if (Result->getType() == CI->getType())
    return;
  replaceAllUsersCommon(CI, Result);
}

void GenXFunctionPointersLowering::reconstructPhi(PHINode *Phi) {
  for (auto *Op : Phi->operand_values()) {
    auto *OpTr = transformFuncPtrVec(Op);
    Phi->replaceUsesOfWith(Op, OpTr);
    if (OpTr != Op)
      PhisIndex[Phi]++;
  }
  assert(Phi->getNumOperands() > 0 && Phi->getNumOperands() == PhisIndex[Phi]);
  Type *NewTy = Phi->value_op_begin()->getType();
  assert(std::all_of(Phi->value_op_begin(), Phi->value_op_end(),
                     [&NewTy](Value *V) { return V->getType() == NewTy; }));
  auto *NewPhi = PHINode::Create(NewTy, 0, Phi->getName(), Phi);
  for (unsigned i = 0; i < Phi->getNumIncomingValues(); ++i)
    NewPhi->addIncoming(Phi->getIncomingValue(i), Phi->getIncomingBlock(i));
  while (!Phi->user_empty()) {
    // already checked that this is only wrr/rdr
    auto *U = Phi->user_back();
    U->replaceUsesOfWith(Phi, NewPhi);
  }
  Phi->eraseFromParent();
}

void GenXFunctionPointersLowering::reconstructSelect(SelectInst *SI) {
  Value *TVal = nullptr, *FVal = nullptr;
  Region R1(SI->getTrueValue()->getType(), DL),
      R2(SI->getFalseValue()->getType(), DL);
  auto *BCT = BitCastInst::CreateBitOrPointerCast(
      transformFuncPtrVec(SI->getTrueValue()), Type::getInt64Ty(*Ctx), "", SI);
  BCT = BitCastInst::CreateBitOrPointerCast(
      BCT, VectorType::get(Type::getInt64Ty(*Ctx), 1), "", SI);
  auto *BCF = BitCastInst::CreateBitOrPointerCast(
      transformFuncPtrVec(SI->getFalseValue()), Type::getInt64Ty(*Ctx), "", SI);
  BCF = BitCastInst::CreateBitOrPointerCast(
      BCF, VectorType::get(Type::getInt64Ty(*Ctx), 1), "", SI);
  R1.Width = (SI->getTrueValue()->getType()->isVectorTy())
                 ? SI->getTrueValue()->getType()->getVectorNumElements()
                 : 1;
  R1.Width = (SI->getFalseValue()->getType()->isVectorTy())
                 ? SI->getFalseValue()->getType()->getVectorNumElements()
                 : 1;
  R1.Stride = 0, R1.VStride = 0;
  R2.Stride = 0, R2.VStride = 0;
  TVal = R1.createRdRegion(BCT, SI->getName(), SI, SI->getDebugLoc(), true);
  FVal = R2.createRdRegion(BCF, SI->getName(), SI, SI->getDebugLoc(), true);
  auto *NewSI = SelectInst::Create(SI->getCondition(), TVal, FVal, "", SI);
  if (SI->getType() == NewSI->getType())
    SI->replaceAllUsesWith(NewSI);
  else
    replaceAllUsersCommon(SI, NewSI);
}

Value *GenXFunctionPointersLowering::transformFuncPtrVec(Value *V) {
  // quite often wrr/rdr get bitcast of funcptrs to <N * i8> as input,
  // here we simply don't need them and DCE will sweep them later
  auto Int64Ty = Type::getInt64Ty(*Ctx);
  if (isa<UndefValue>(V)) {
    assert(V->getType()->isSingleValueType());
    if (V->getType()->getScalarType()->isIntegerTy(64))
      return V;
    else if (V->getType()->isVectorTy())
      return UndefValue::get(
          VectorType::get(Int64Ty, V->getType()->getVectorNumElements()));
    else
      return UndefValue::get(Int64Ty);
  }
  if (isa<ConstantExpr>(V) &&
      cast<ConstantExpr>(V)->getOpcode() == Instruction::BitCast)
    V = cast<ConstantExpr>(V)->getOperand(0);
  else if (auto *BC = dyn_cast<BitCastInst>(V))
    if (!(BC->getType()->isVectorTy() &&
          BC->getType()->getScalarType() == BC->getOperand(0)->getType()))
      V = BC->getOperand(0);
  SetVector<Function *> Funcs;
  if (!isFuncPointerVec(V, &Funcs))
    return V;
  assert(Funcs.size() > 0);

  assert(V->getType()->isVectorTy());
  std::vector<Constant *> CF;
  for (auto &Val : Funcs)
    CF.push_back(ConstantExpr::getPtrToInt(cast<Constant>(Val), Int64Ty));
  Value *NewVal = nullptr;
  // generate i64 instead of <1 x i64>
  if (CF.size() > 1)
    NewVal = ConstantVector::get(CF);
  else if (CF.size() == 1)
    NewVal = CF.front();
  return NewVal;
}
