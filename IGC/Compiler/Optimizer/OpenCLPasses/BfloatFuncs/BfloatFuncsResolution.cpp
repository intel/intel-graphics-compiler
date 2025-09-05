/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/BfloatFuncs/BfloatFuncsResolution.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Demangle/Demangle.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char BfloatFuncsResolution::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-bfloat-funcs-resolution"
#define PASS_DESCRIPTION "BfloatFuncsResolution"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BfloatFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(BfloatFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// This pass lowers validation functions for bfloat operations from OpenCL-C level.
// This is only needed for validation efforts, no declarations are provided in our headers,
// we take mangled overloaded forms as input.
//
// Full list of supported functions is at the bottom of the file.

BfloatFuncsResolution::BfloatFuncsResolution(void) : FunctionPass(ID) {
  initializeBfloatFuncsResolutionPass(*PassRegistry::getPassRegistry());
}

bool BfloatFuncsResolution::runOnFunction(Function &F) {
  llvm::IRBuilder<> builder(F.getContext());
  m_builder = &builder;
  m_changed = false;
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  visit(F);

  for (auto I : m_instructionsToRemove) {
    I->eraseFromParent();
  }

  m_instructionsToRemove.clear();

  return m_changed;
}

void BfloatFuncsResolution::visitCallInst(CallInst &CI) {
  // The functions that we are about to resolve are in mangled form.
  // Quick check before demangling to save compilation time.
  if (!CI.getCalledFunction() || !CI.getCalledFunction()->getName().contains("__builtin_bf16"))
    return;

  std::string DNameStr = llvm::demangle(CI.getCalledFunction()->getName().str());
  StringRef DName(DNameStr);

  if (!DName.startswith("__builtin_bf16"))
    return;

  m_builder->SetInsertPoint(&CI);

  m_changed = true;

  llvm::StringSwitch<std::function<void()>>(DName)
      .StartsWith("__builtin_bf16_isequal", [&]() { handleCompare(CI, CmpInst::Predicate::FCMP_OEQ); })
      .StartsWith("__builtin_bf16_isgreaterequal", [&]() { handleCompare(CI, CmpInst::Predicate::FCMP_OGE); })
      .StartsWith("__builtin_bf16_isgreater", [&]() { handleCompare(CI, CmpInst::Predicate::FCMP_OGT); })
      .StartsWith("__builtin_bf16_islessequal", [&]() { handleCompare(CI, CmpInst::Predicate::FCMP_OLE); })
      .StartsWith("__builtin_bf16_isless", [&]() { handleCompare(CI, CmpInst::Predicate::FCMP_OLT); })
      .StartsWith("__builtin_bf16_isnotequal", [&]() { handleCompare(CI, CmpInst::Predicate::FCMP_UNE); })
      .StartsWith("__builtin_bf16_isunordered", [&]() { handleCompare(CI, CmpInst::Predicate::FCMP_UNO); })
      .StartsWith("__builtin_bf16_select", [&]() { handleSelect(CI); })
      .StartsWith("__builtin_bf16_min", [&]() { handleMinMax(CI, CmpInst::Predicate::FCMP_OLT); })
      .StartsWith("__builtin_bf16_max", [&]() { handleMinMax(CI, CmpInst::Predicate::FCMP_OGT); })
      .StartsWith("__builtin_bf16_add", [&]() { handleArithmetic(CI, Instruction::FAdd); })
      .StartsWith("__builtin_bf16_sub", [&]() { handleArithmetic(CI, Instruction::FSub); })
      .StartsWith("__builtin_bf16_mul", [&]() { handleArithmetic(CI, Instruction::FMul); })
      .StartsWith("__builtin_bf16_mad", [&]() { handleArithmetic(CI, -1, true); })

      .StartsWith("__builtin_bf16_log", [&]() { handleMath(CI, Intrinsic::log2); })
      .StartsWith("__builtin_bf16_exp", [&]() { handleMath(CI, Intrinsic::exp2); })
      .StartsWith("__builtin_bf16_sqrt", [&]() { handleMath(CI, Intrinsic::sqrt); })
      .StartsWith("__builtin_bf16_sin", [&]() { handleMath(CI, Intrinsic::sin); })
      .StartsWith("__builtin_bf16_cos", [&]() { handleMath(CI, Intrinsic::cos); })
      .StartsWith("__builtin_bf16_inv", [&]() { handleMath(CI, GenISAIntrinsic::GenISA_inv, true); })
      .Default([&]() {
        IGC_ASSERT(0);
        m_ctx->EmitError("Unhandled __builtin_bf16_ instruction!", &CI);
        m_changed = false;
      })();
}

// Helper function. Returns scalar or vector type based on input OrgType.
// E.g. With OrgType = <2 x i32> and DesiredTypeScalar = bfloat it will
// return <2 x bfloat>
Type *BfloatFuncsResolution::getTypeBasedOnType(Type *OrgType, Type *DesiredTypeScalar) {
  auto DestType = OrgType->isVectorTy()
                      ? IGCLLVM::FixedVectorType::get(
                            DesiredTypeScalar, (unsigned)cast<IGCLLVM::FixedVectorType>(OrgType)->getNumElements())
                      : DesiredTypeScalar;
  return DestType;
}

// Helper function that bitcasts given scalar/vector value to bfloat type.
// Note: m_builder needs to have insert point set before calling this helper.
Value *BfloatFuncsResolution::bitcastToBfloat(Value *V) {
  IGC_ASSERT(V && V->getType()->getScalarSizeInBits() == m_builder->getBFloatTy()->getScalarSizeInBits());

  auto DestType = getTypeBasedOnType(V->getType(), m_builder->getBFloatTy());
  return m_builder->CreateBitCast(V, DestType);
}

void BfloatFuncsResolution::handleCompare(CallInst &CI, CmpInst::Predicate Pred) {
  auto Op0 = CI.getOperand(0);
  auto Op1 = CI.getOperand(1);

  if (IGCLLVM::getNumArgOperands(&CI) != 2 || !Op0->getType()->getScalarType()->isIntegerTy(16) ||
      !Op1->getType()->getScalarType()->isIntegerTy(16) || !CI.getType()->getScalarType()->isIntegerTy(32)) {
    m_ctx->EmitError("Unexpected function signature", &CI);
    return;
  }

  auto Op0Bf = bitcastToBfloat(Op0);
  auto Op1Bf = bitcastToBfloat(Op1);

  auto CompareInst = m_builder->CreateFCmp(Pred, Op0Bf, Op1Bf);
  auto ExtendInst = m_builder->CreateZExt(CompareInst, CI.getType());

  CI.replaceAllUsesWith(ExtendInst);
  m_instructionsToRemove.push_back(&CI);
}

void BfloatFuncsResolution::handleSelect(CallInst &CI) {
  auto CondOp = CI.getOperand(0);
  auto Op1 = CI.getOperand(1);
  auto Op2 = CI.getOperand(2);

  if (IGCLLVM::getNumArgOperands(&CI) != 3 || !CondOp->getType()->getScalarType()->isIntegerTy() ||
      !Op1->getType()->getScalarType()->isIntegerTy(16) || !Op2->getType()->getScalarType()->isIntegerTy(16) ||
      !CI.getType()->getScalarType()->isIntegerTy(16)) {
    m_ctx->EmitError("Unexpected function signature", &CI);
    return;
  }

  // Condition operand is defined to be short or int.
  // Trunc it to i1.
  Type *SelectType =
      CondOp->getType()->isVectorTy()
          ? IGCLLVM::FixedVectorType::get(m_builder->getInt1Ty(),
                                          (unsigned)cast<IGCLLVM::FixedVectorType>(CondOp->getType())->getNumElements())
          : cast<Type>(m_builder->getInt1Ty());

  auto CondTrunced = m_builder->CreateTrunc(CondOp, SelectType);

  // Technically we don't need to convert to bfloat for select,
  // but this function is for validation purposes so that that nevertheless.
  auto Op1Bf = bitcastToBfloat(Op1);
  auto Op2Bf = bitcastToBfloat(Op2);
  auto SelectInst = m_builder->CreateSelect(CondTrunced, Op1Bf, Op2Bf);
  auto Res = m_builder->CreateBitCast(SelectInst, CI.getType());

  CI.replaceAllUsesWith(Res);
  m_instructionsToRemove.push_back(&CI);
}

void BfloatFuncsResolution::handleMinMax(CallInst &CI, CmpInst::Predicate Pred) {
  auto Op0 = CI.getOperand(0);
  auto Op1 = CI.getOperand(1);

  if (IGCLLVM::getNumArgOperands(&CI) != 2 || !Op0->getType()->getScalarType()->isIntegerTy(16) ||
      !Op1->getType()->getScalarType()->isIntegerTy(16) || !CI.getType()->getScalarType()->isIntegerTy(16)) {
    m_ctx->EmitError("Unexpected function signature", &CI);
    return;
  }

  auto Op0Bf = bitcastToBfloat(Op0);
  auto Op1Bf = bitcastToBfloat(Op1);

  // According to OpenCL C spec:
  // If one argument is a NaN, fmax() or fmin() returns the other argument.
  // If both arguments are NaNs, fmax() or fmin() returns a NaN.
  auto CompareInst = m_builder->CreateFCmp(Pred, Op0Bf, Op1Bf);
  auto SelectInst = m_builder->CreateSelect(CompareInst, Op0Bf, Op1Bf);
  auto IsNaNOp0 = m_builder->CreateFCmp(CmpInst::Predicate::FCMP_UNO, Op0Bf, Op0Bf);
  auto OtherVal = m_builder->CreateSelect(IsNaNOp0, Op1Bf, Op0Bf);
  auto CompareNaNInst = m_builder->CreateFCmp(CmpInst::Predicate::FCMP_ORD, Op0Bf, Op1Bf);
  auto SelectInst3 = m_builder->CreateSelect(CompareNaNInst, SelectInst, OtherVal);
  auto Res = m_builder->CreateBitCast(SelectInst3, CI.getType());

  CI.replaceAllUsesWith(Res);
  m_instructionsToRemove.push_back(&CI);
}

void BfloatFuncsResolution::handleArithmetic(llvm::CallInst &CI, unsigned Opcode, bool IsMadInstruction) {

  llvm::SmallVector<Value *, 3> Operands;
  int FloatOperandIndex = -1;
  for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(&CI); ++i) {
    auto Op = CI.getOperand(i);
    if (Op->getType()->getScalarType()->isFloatTy()) {
      if (FloatOperandIndex != -1) {
        m_ctx->EmitError("Only one operand expected to be float!", &CI);
        return;
      }
      FloatOperandIndex = i;
    } else if (!Op->getType()->getScalarType()->isIntegerTy(16)) {
      m_ctx->EmitError("Arguments expected to be either float or short!", &CI);
      return;
    }
    Operands.push_back(Op);
  }

  bool IsResFloat = CI.getType()->getScalarType()->isFloatTy();

  if (IsResFloat || (FloatOperandIndex != -1)) {
    // 1. If we have float on destination, or float source extend the
    // short sources to float. Let vISA handle the mix mode propagation.
    for (size_t i = 0; i < Operands.size(); ++i) {
      if (i == FloatOperandIndex)
        continue;
      auto Op = bitcastToBfloat(Operands[i]);
      Operands[i] = m_builder->CreateFPExt(Op, getTypeBasedOnType(Op->getType(), m_builder->getFloatTy()));
    }
  } else if (FloatOperandIndex == -1) {
    // 3. If we have only shorts on source, just
    // bitcast sources to bfloat.
    for (size_t i = 0; i < Operands.size(); ++i) {
      Operands[i] = bitcastToBfloat(Operands[i]);
    }
  } else {
    IGC_ASSERT_MESSAGE(0, "Unexpected param types");
  }

  Value *Res = nullptr;

  if (Operands.size() == 2) {
    Res = m_builder->CreateBinOp((Instruction::BinaryOps)Opcode, Operands[0], Operands[1]);
  } else if (Operands.size() == 3 && IsMadInstruction) {
    Res = m_builder->CreateFMul(Operands[0], Operands[1]);
    Res = m_builder->CreateFAdd(Res, Operands[2]);
  } else {
    IGC_ASSERT_MESSAGE(0, "Unsupported number of operands.");
    return;
  }

  if (Res && Res->getType()->getScalarType()->isBFloatTy()) {
    if (IsResFloat) {
      IGC_ASSERT_MESSAGE(0, "Not expected path");
    } else {
      Res = m_builder->CreateBitCast(Res, CI.getType());
    }
  } else {
    IGC_ASSERT(Res && Res->getType()->getScalarType()->isFloatTy());
    if (!CI.getType()->getScalarType()->isFloatTy()) {
      Res = m_builder->CreateFPTrunc(Res, getTypeBasedOnType(Res->getType(), m_builder->getBFloatTy()));
      Res = m_builder->CreateBitCast(Res, CI.getType());
    }
  }

  CI.replaceAllUsesWith(Res);
  m_instructionsToRemove.push_back(&CI);
}

void BfloatFuncsResolution::handleMath(llvm::CallInst &CI, llvm::Intrinsic::ID Operation, bool needsGenISAIntrinsic) {
  auto Op0 = CI.getOperand(0);

  if (IGCLLVM::getNumArgOperands(&CI) != 1 || !Op0->getType()->getScalarType()->isIntegerTy(16) ||
      !CI.getType()->getScalarType()->isIntegerTy(16)) {
    m_ctx->EmitError("Unexpected function signature", &CI);
    return;
  }

  Op0 = bitcastToBfloat(CI.getOperand(0));

  Value *Res = UndefValue::get(Op0->getType());

  int numScalarOperations =
      Op0->getType()->isVectorTy() ? cast<IGCLLVM::FixedVectorType>(Op0->getType())->getNumElements() : 1;
  Type *scalarType = Op0->getType()->getScalarType();
  for (int i = 0; i < numScalarOperations; i++) {
    Value *Operand = Op0->getType()->isVectorTy() ? m_builder->CreateExtractElement(Op0, i) : Op0;
    Value *CallRes = nullptr;
    if (needsGenISAIntrinsic) {
      auto pFunc = llvm::GenISAIntrinsic::getDeclaration(CI.getModule(), (GenISAIntrinsic::ID)Operation, scalarType);
      CallRes = m_builder->CreateCall(pFunc, Operand);
    } else {
      CallRes = m_builder->CreateIntrinsic(Operation, {scalarType}, {Operand});
    }
    Res = Op0->getType()->isVectorTy() ? m_builder->CreateInsertElement(Res, CallRes, i) : CallRes;
  }
  Res = m_builder->CreateBitCast(Res, CI.getType());
  CI.replaceAllUsesWith(Res);
  m_instructionsToRemove.push_back(&CI);
}
/*
Supported functions list:

intn __builtin_bf16_isequal(ushortn, ushortn)
intn __builtin_bf16_isgreater(ushortn, ushortn)
intn __builtin_bf16_isless(ushortn, ushortn)
intn __builtin_bf16_isnotequal(ushortn, ushortn)
intn __builtin_bf16_islessequal(ushortn, ushortn)
intn __builtin_bf16_isgreaterequal(ushortn, ushortn)
intn __builtin_bf16_isunordered(ushortn, ushortn)

ushortn __builtin_bf16_select(intn, ushortn, ushortn)
ushortn __builtin_bf16_select(shortn, ushortn, ushortn)

ushortn __builtin_bf16_min(ushortn, ushortn)
ushortn __builtin_bf16_max(ushortn, ushortn)

ushortn __builtin_bf16_add(ushortn, ushortn)
ushortn __builtin_bf16_add(floatn, ushortn)
ushortn __builtin_bf16_add(ushortn, floatn)
floatn __builtin_bf16_addf(floatn, ushortn)
floatn __builtin_bf16_addf(ushortn, floatn)
floatn __builtin_bf16_addf(ushortn, ushortn)

ushortn __builtin_bf16_sub(ushortn, ushortn)
ushortn __builtin_bf16_sub(floatn, ushortn)
ushortn __builtin_bf16_sub(ushortn, floatn)
floatn __builtin_bf16_subf(floatn, ushortn)
floatn __builtin_bf16_subf(ushortn, floatn)
floatn __builtin_bf16_subf(ushortn, ushortn)

ushortn __builtin_bf16_mul(ushortn, ushortn)
ushortn __builtin_bf16_mul(floatn, ushortn)
floatn __builtin_bf16_mulf(ushortn, ushortn)
floatn __builtin_bf16_mulf(floatn, ushortn)

ushortn __builtin_bf16_mad(ushortn a, ushortn b, ushortn c)
ushortn __builtin_bf16_mad(floatn a, ushortn b, ushortn c)
ushortn __builtin_bf16_mad(ushortn a, ushortn b, floatn c)
floatn __builtin_bf16_madf(ushortn, ushortn, ushortn)
floatn __builtin_bf16_madf(floatn, ushortn, ushortn)
floatn __builtin_bf16_madf(ushortn, ushortn, floatn)

ushortn __builtin_bf16_inv(ushortn)
ushortn __builtin_bf16_log(ushortn)
ushortn __builtin_bf16_exp(ushortn)
ushortn __builtin_bf16_sqrt(ushortn)
ushortn __builtin_bf16_sin(ushortn)
ushortn __builtin_bf16_cos(ushortn)
*/
