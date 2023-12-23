/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXBFloatLowering
/// ------------
///
/// This pass lowers bfloat instructions into float ones, because bfloat
/// arithmetic operation aren't supported by the hardware.
///
/// It transforms code, like:
///     %fdiv_res = fdiv bfloat %a, %b
/// Into the following code:
///     %a.ext = fpext bfloat %a to float
///     %b.ext = fpext bfloat %b to float
///     %fdiv = fdiv float %a.ext, %b.ext
///     %trunc = fptrunc float %fdiv to bfloat
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include <llvm/IR/InstVisitor.h>
#include <llvm/CodeGen/TargetPassConfig.h>

#define DEBUG_TYPE "genx-bfloat-lowering"

using namespace llvm;
using namespace genx;

static cl::opt<bool>
    EnableGenXBFloatLowering("enable-genx-bfloat-lowering", cl::init(true),
                             cl::Hidden,
                             cl::desc("Enable GenX bfloat lowering."));

namespace {
// GenXBFloatLowering
class GenXBFloatLowering : public FunctionPass,
                           public InstVisitor<GenXBFloatLowering> {
  bool Modify = false;
  const GenXSubtarget *ST = nullptr;

public:
  explicit GenXBFloatLowering() : FunctionPass(ID) {}
  StringRef getPassName() const override { return "GenX BFloat lowering"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;

public:
  static char ID;
#if LLVM_VERSION_MAJOR > 10
  // fadd, fsub e.t.c
  void visitBinaryOperator(BinaryOperator &Inst);
  // fneg
  void visitUnaryOperator(UnaryOperator &Inst);
  // fcmp
  void visitFCmpInst(FCmpInst &Inst);
  // fptrunc double to bfloat
  void visitFPTruncInst(FPTruncInst &Inst);
  // fpext bfloat to double
  void visitFPExtInst(FPExtInst &Inst);
  // bfloat<->integer cast
  void visitFPToSIInst(FPToSIInst &Inst);
  void visitFPToUIInst(FPToUIInst &Inst);
  void visitSIToFPInst(SIToFPInst &Inst);
  void visitUIToFPInst(UIToFPInst &Inst);

  // lower intrinsic instructions
  void visitCallInst(CallInst &Inst);

private:
  void lowerCastToBFloat(CastInst &Inst);
  void lowerCastFromBFloat(CastInst &Inst);

#endif // LLVM_VERSION_MAJOR > 10
};

} // end namespace

char GenXBFloatLowering::ID = 0;
namespace llvm {
void initializeGenXBFloatLoweringPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXBFloatLowering, "GenXBFloatLowering",
                      "GenXBFloatLowering", false, false)
INITIALIZE_PASS_END(GenXBFloatLowering, "GenXBFloatLowering",
                    "GenXBFloatLowering", false, false)

FunctionPass *llvm::createGenXBFloatLoweringPass() {
  initializeGenXBFloatLoweringPass(*PassRegistry::getPassRegistry());
  return new GenXBFloatLowering;
}

void GenXBFloatLowering::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
}

#if LLVM_VERSION_MAJOR > 10
static Type *getFloatTyFromBfloat(Type *Ty) {
  IGC_ASSERT_EXIT(Ty->getScalarType()->isBFloatTy());
  auto *FloatTy = Type::getFloatTy(Ty->getContext());

  if (Ty->isBFloatTy())
    return FloatTy;

  auto *VecTy = cast<IGCLLVM::FixedVectorType>(Ty);
  return IGCLLVM::FixedVectorType::get(FloatTy, VecTy->getNumElements());
}

// fadd, fsub e.t.c
void GenXBFloatLowering::visitBinaryOperator(BinaryOperator &Inst) {
  auto *Ty = Inst.getType();
  if (!Inst.getType()->getScalarType()->isBFloatTy())
    return;
  LLVM_DEBUG(dbgs() << "GenXBFloatLowering: apply on BinaryOperator\n"
                    << Inst << "\n");
  auto *Src0 = Inst.getOperand(0);
  auto *Src1 = Inst.getOperand(1);
  IGC_ASSERT_EXIT(Src0->getType()->getScalarType()->isBFloatTy());
  IGC_ASSERT_EXIT(Src1->getType()->getScalarType()->isBFloatTy());
  IRBuilder<> Builder(&Inst);
  auto *FloatTy = getFloatTyFromBfloat(Src0->getType());
  Instruction::BinaryOps Opcode = Inst.getOpcode();
  auto *Op0Conv = Builder.CreateFPExt(Src0, FloatTy);
  auto *Op1Conv = Builder.CreateFPExt(Src1, FloatTy);

  auto *InstUpdate =
      cast<Instruction>(Builder.CreateBinOp(Opcode, Op0Conv, Op1Conv));
  InstUpdate->setFastMathFlags(Inst.getFastMathFlags());

  auto *Trunc = Builder.CreateFPTrunc(InstUpdate, Ty);
  Inst.replaceAllUsesWith(Trunc);
  Inst.eraseFromParent();
  Modify = true;
}

void GenXBFloatLowering::visitFCmpInst(FCmpInst &Inst) {
  auto *Src0 = Inst.getOperand(0);
  auto *Src1 = Inst.getOperand(1);
  auto *SrcTy = Src0->getType();
  if (!SrcTy->getScalarType()->isBFloatTy())
    return;
  LLVM_DEBUG(dbgs() << "GenXBFloatLowering: apply on FCmp\n" << Inst << "\n");
  IGC_ASSERT_EXIT(Src1->getType()->getScalarType()->isBFloatTy());
  IRBuilder<> Builder(&Inst);
  auto *FloatTy = getFloatTyFromBfloat(SrcTy);
  auto *Op0Conv = Builder.CreateFPExt(Src0, FloatTy);
  auto *Op1Conv = Builder.CreateFPExt(Src1, FloatTy);
  auto *InstUpdate = Builder.CreateFCmp(Inst.getPredicate(), Op0Conv, Op1Conv);
  Inst.replaceAllUsesWith(InstUpdate);
  Inst.eraseFromParent();
  Modify = true;
}

// fneg
void GenXBFloatLowering::visitUnaryOperator(UnaryOperator &Inst) {
  auto *Src = Inst.getOperand(0);
  auto *SrcTy = Src->getType();
  if (!SrcTy->getScalarType()->isBFloatTy())
    return;
  LLVM_DEBUG(dbgs() << "GenXBFloatLowering: apply on UnaryOperator\n"
                    << Inst << "\n");
  IGC_ASSERT_EXIT(Inst.getOpcode() == Instruction::FNeg);
  IRBuilder<> Builder(&Inst);
  auto *FloatTy = getFloatTyFromBfloat(SrcTy);
  auto *SrcExt = Builder.CreateFPExt(Src, FloatTy);
  auto *InstUpdate = Builder.CreateUnOp(Inst.getOpcode(), SrcExt);
  auto *Trunc = Builder.CreateFPTrunc(InstUpdate, SrcTy);
  Inst.replaceAllUsesWith(Trunc);
  Inst.eraseFromParent();
  Modify = true;
}

void GenXBFloatLowering::visitFPTruncInst(FPTruncInst &Inst) {
  lowerCastToBFloat(Inst);
}

void GenXBFloatLowering::visitFPExtInst(FPExtInst &Inst) {
  lowerCastFromBFloat(Inst);
}

void GenXBFloatLowering::visitFPToSIInst(FPToSIInst &Inst) {
  lowerCastFromBFloat(Inst);
}

void GenXBFloatLowering::visitFPToUIInst(FPToUIInst &Inst) {
  lowerCastFromBFloat(Inst);
}

void GenXBFloatLowering::visitSIToFPInst(SIToFPInst &Inst) {
  lowerCastToBFloat(Inst);
}

void GenXBFloatLowering::visitUIToFPInst(UIToFPInst &Inst) {
  lowerCastToBFloat(Inst);
}

void GenXBFloatLowering::visitCallInst(CallInst &Inst) {
  auto IID = vc::getAnyIntrinsicID(&Inst);
  auto *Ty = Inst.getType();
  SmallVector<Type *, 2> Types;

  switch (IID) {
  default:
    return;
  case GenXIntrinsic::genx_sat:
    break;
  case Intrinsic::cos:
  case Intrinsic::exp2:
  case Intrinsic::fabs:
  case Intrinsic::fma:
  case Intrinsic::fmuladd:
  case Intrinsic::log2:
  case Intrinsic::maximum:
  case Intrinsic::maxnum:
  case Intrinsic::minimum:
  case Intrinsic::minnum:
  case Intrinsic::pow:
  case Intrinsic::sin:
  case Intrinsic::sqrt:
    break;
  case Intrinsic::fptosi_sat:
  case Intrinsic::fptoui_sat:
    Types.push_back(Ty);
    Ty = Inst.getArgOperand(0)->getType();
    break;
  }

  if (!Ty->getScalarType()->isBFloatTy())
    return;

  LLVM_DEBUG(dbgs() << "GenXBFloatLowering: apply on Intrinsic\n"
                    << Inst << "\n");

  auto *ExtTy = getFloatTyFromBfloat(Ty);
  Types.push_back(ExtTy);

  IRBuilder<> Builder(&Inst);
  if (isa<FPMathOperator>(Inst))
    Builder.setFastMathFlags(Inst.getFastMathFlags());

  SmallVector<Value *, 4> Args;
  llvm::transform(Inst.args(), std::back_inserter(Args),
                  [&Builder, ExtTy](Value *Arg) {
                    auto *Ty = Arg->getType();
                    if (!Ty->getScalarType()->isBFloatTy())
                      return Arg;
                    return Builder.CreateFPExt(Arg, ExtTy);
                  });

  auto *Func = vc::getAnyDeclaration(Inst.getModule(), IID, Types);
  Value *NewInst = Builder.CreateCall(Func, Args);

  if (NewInst->getType()->getScalarType()->isFloatTy())
    NewInst = Builder.CreateFPTrunc(NewInst, Inst.getType());

  Inst.replaceAllUsesWith(NewInst);
  Inst.eraseFromParent();
  Modify = true;
}

void GenXBFloatLowering::lowerCastToBFloat(CastInst &Inst) {
  auto *ResTy = Inst.getType();
  if (!ResTy->getScalarType()->isBFloatTy())
    return;

  auto *Src = Inst.getOperand(0);
  auto *SrcTy = Src->getType();

  if (SrcTy->getScalarType()->isFloatTy())
    return;

  LLVM_DEBUG(dbgs() << "GenXBFloatLowering: apply on CastInst\n"
                    << Inst << "\n");

  IRBuilder<> Builder(&Inst);

  auto *FloatTy = getFloatTyFromBfloat(ResTy);

  auto *Cast = Builder.CreateCast(Inst.getOpcode(), Src, FloatTy);
  auto *Trunc = Builder.CreateFPTrunc(Cast, ResTy);

  Inst.replaceAllUsesWith(Trunc);
  Inst.eraseFromParent();
  Modify = true;
}

void GenXBFloatLowering::lowerCastFromBFloat(CastInst &Inst) {
  auto *ResTy = Inst.getType();
  if (ResTy->getScalarType()->isFloatTy())
    return;

  auto *Src = Inst.getOperand(0);
  auto *SrcTy = Src->getType();

  if (!SrcTy->getScalarType()->isBFloatTy())
    return;

  LLVM_DEBUG(dbgs() << "GenXBFloatLowering: apply on CastInst\n"
                    << Inst << "\n");

  IRBuilder<> Builder(&Inst);

  auto *FloatTy = getFloatTyFromBfloat(SrcTy);
  auto *Ext = Builder.CreateFPExt(Src, FloatTy);
  auto *Cast = Builder.CreateCast(Inst.getOpcode(), Ext, ResTy);

  Inst.replaceAllUsesWith(Cast);
  Inst.eraseFromParent();
  Modify = true;
}
#endif // LLVM_VERSION_MAJOR > 10

/***********************************************************************
 * GenXBFloatLowering::runOnFunction
 */
bool GenXBFloatLowering::runOnFunction(Function &F) {
  if (!EnableGenXBFloatLowering)
    return false;
  LLVM_DEBUG(dbgs() << "GenXBFloatLowering started\n");

#if LLVM_VERSION_MAJOR > 10
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();

  visit(F);
#endif // LLVM_VERSION_MAJOR > 10

  LLVM_DEBUG(dbgs() << "GenXBFloatLowering ended\n");
  return Modify;
}
