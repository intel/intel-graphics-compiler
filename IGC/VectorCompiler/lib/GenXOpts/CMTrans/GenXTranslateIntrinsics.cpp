/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/InternalIntrinsics/InternalIntrinsics.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>

#define DEBUG_TYPE "genx-translate-intrinsics"

using namespace llvm;

namespace {
class GenXTranslateIntrinsics final
    : public FunctionPass,
      public InstVisitor<GenXTranslateIntrinsics> {
public:
  static char ID;
  GenXTranslateIntrinsics() : FunctionPass(ID) {}
  StringRef getPassName() const override {
    return "GenX intrinsics translator";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override;

  void visitCallInst(CallInst &I) const;

private:
  Value *translateBFloat16Convert(CallInst &I) const;
  Value *translateTFloat32Convert(CallInst &I) const;
};
} // namespace

char GenXTranslateIntrinsics::ID = 0;

INITIALIZE_PASS_BEGIN(GenXTranslateIntrinsics, "GenXTranslateIntrinsics",
                      "GenXTranslateIntrinsics", false, false)
INITIALIZE_PASS_END(GenXTranslateIntrinsics, "GenXTranslateIntrinsics",
                    "GenXTranslateIntrinsics", false, false)

FunctionPass *llvm::createGenXTranslateIntrinsicsPass() {
  initializeGenXTranslateIntrinsicsPass(*PassRegistry::getPassRegistry());
  return new GenXTranslateIntrinsics;
}

bool GenXTranslateIntrinsics::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "GenXTranslateIntrinsics started\n");
  visit(F);
  LLVM_DEBUG(dbgs() << "GenXTranslateIntrinsics finished\n");
  return true;
}

void GenXTranslateIntrinsics::visitCallInst(CallInst &I) const {
  auto IID = GenXIntrinsic::getGenXIntrinsicID(&I);
  Value *NewI = nullptr;

  switch (IID) {
  default:
    return;
  case GenXIntrinsic::genx_bf_cvt:
    NewI = translateBFloat16Convert(I);
    break;
  case GenXIntrinsic::genx_tf32_cvt:
    NewI = translateTFloat32Convert(I);
  }

  if (!NewI)
    return;

  NewI->takeName(&I);
  I.replaceAllUsesWith(NewI);
  I.eraseFromParent();
  return;
}

Value *GenXTranslateIntrinsics::translateBFloat16Convert(CallInst &I) const {
  IGC_ASSERT_EXIT(GenXIntrinsic::getGenXIntrinsicID(&I) ==
                  GenXIntrinsic::genx_bf_cvt);
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *Arg = I.getArgOperand(0);
  auto *ArgTy = Arg->getType();
  auto *ArgETy = ArgTy;
  auto *RetTy = I.getType();

  Type *I16Ty = Builder.getInt16Ty();

  if (auto *ArgVTy = dyn_cast<IGCLLVM::FixedVectorType>(ArgTy)) {
    ArgETy = ArgVTy->getElementType();
    I16Ty = IGCLLVM::FixedVectorType::get(I16Ty, ArgVTy->getNumElements());
  }

  Function *Func = nullptr;
  if (ArgETy->isHalfTy()) {
    Func = vc::InternalIntrinsic::getInternalDeclaration(
        M, vc::InternalIntrinsic::cast_from_bf16, {RetTy, I16Ty});
    Arg = Builder.CreateBitCast(Arg, I16Ty);
  } else {
    Func = vc::InternalIntrinsic::getInternalDeclaration(
        M, vc::InternalIntrinsic::cast_to_bf16, {I16Ty, ArgTy});
  }

  auto *NewI = Builder.CreateCall(Func, {Arg});
  LLVM_DEBUG(dbgs() << "Created: " << *NewI << "\n");

  return Builder.CreateBitCast(NewI, RetTy);
}

Value *GenXTranslateIntrinsics::translateTFloat32Convert(CallInst &I) const {
  IGC_ASSERT_EXIT(GenXIntrinsic::getGenXIntrinsicID(&I) ==
                  GenXIntrinsic::genx_tf32_cvt);
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *Arg = I.getArgOperand(0);
  auto *ArgTy = Arg->getType();
  auto *RetTy = I.getType();

  Function *Func = vc::InternalIntrinsic::getInternalDeclaration(
      M, vc::InternalIntrinsic::round_to_tf32, {RetTy, ArgTy});
  auto *NewI = Builder.CreateCall(Func, {Arg});
  LLVM_DEBUG(dbgs() << "Created: " << *NewI << "\n");

  return NewI;
}
