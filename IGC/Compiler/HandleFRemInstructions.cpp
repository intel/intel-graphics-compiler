/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/HandleFRemInstructions.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/OpenCLPasses/LongVectorBuiltinScalarization/PerElementEmitter.hpp"
#include "Probe/Assertion.h"
#include "llvmWrapper/IR/Instructions.h"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-handle-frem-inst"
#define PASS_DESCRIPTION "Replace FRem instructions with proper builtin calls"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HandleFRemInstructionsLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(HandleFRemInstructionsLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HandleFRemInstructionsLPM::ID = 0;

HandleFRemInstructionsLPM::HandleFRemInstructionsLPM() : ModulePass(ID) {
  initializeHandleFRemInstructionsLPMPass(*PassRegistry::getPassRegistry());
}

void HandleFRemInstructions::visitFRem(llvm::BinaryOperator &I) { m_FRems.push_back(&I); }

void HandleFRemInstructions::replaceFRem(llvm::BinaryOperator &I) {
  auto Val1 = I.getOperand(0);
  auto Val2 = I.getOperand(1);
  auto ValType = Val1->getType();
  auto ScalarType = ValType->getScalarType();

  IGC_ASSERT_MESSAGE(Val1->getType() == Val2->getType(), "Operands of frem instruction must have same type");
  IGC_ASSERT_MESSAGE(ScalarType->isFloatingPointTy(), "Operands of frem instruction must have floating point type");

  std::string VecStr = "";
  std::string FpTypeStr;
  IRBuilder<> Builder(&I);

  if (ScalarType->isHalfTy() || ScalarType->isFloatTy() || ScalarType->isDoubleTy()) {
    auto TypeWidth = ScalarType->getScalarSizeInBits();
    FpTypeStr = "f" + std::to_string(TypeWidth);
  } else if (IGCLLVM::isBFloatTy(ScalarType)) {
    FpTypeStr = "f32";
    Type *FloatTy = Type::getFloatTy(I.getContext());
    ValType = ValType->isVectorTy() ? IGCLLVM::FixedVectorType::get(
                                          FloatTy, (unsigned)cast<IGCLLVM::FixedVectorType>(ValType)->getNumElements())
                                    : FloatTy;
    Val1 = Builder.CreateFPExt(Val1, ValType);
    Val2 = Builder.CreateFPExt(Val2, ValType);
  } else {
    IGC_ASSERT_MESSAGE(0, "Unsupported type");
  }

  // SPV_EXT_long_vector allows widths BiF has no OpFRem variant for. These
  // call the scalar variant for each element.
  auto *VecType = dyn_cast<IGCLLVM::FixedVectorType>(ValType);
  bool PerElement = VecType && !isOpenCLVectorWidth((unsigned)VecType->getNumElements());
  if (VecType && !PerElement)
    VecStr = "v" + std::to_string(VecType->getNumElements());

  Type *CalleeType = PerElement ? ValType->getScalarType() : ValType;
  std::string TypeStr = "_" + VecStr + FpTypeStr;
  std::string FuncName = "__builtin_spirv_OpFRem" + TypeStr + TypeStr;
  auto FT = FunctionType::get(CalleeType, {CalleeType, CalleeType}, false);
  auto Callee = m_module->getOrInsertFunction(FuncName, FT);

  Value *NewFRem = nullptr;
  if (PerElement) {
    PerElementEmitter E(I, (unsigned)VecType->getNumElements());
    IRBuilder<> &B = E.builder();
    NewFRem = E.emit(VecType, [&](Value *Lane) {
      return B.CreateCall(Callee, {B.CreateExtractElement(Val1, Lane), B.CreateExtractElement(Val2, Lane)});
    });
    // The loop moves I to a new block.
    Builder.SetInsertPoint(&I);
  } else {
    NewFRem = Builder.CreateCall(Callee, {Val1, Val2});
  }
  if (IGCLLVM::isBFloatTy(ScalarType))
    NewFRem = Builder.CreateFPTrunc(NewFRem, I.getType());
  NewFRem->takeName(&I);
  I.replaceAllUsesWith(NewFRem);
  I.eraseFromParent();
  m_changed = true;
}

bool HandleFRemInstructions::run(llvm::Module &M) {
  m_changed = false;
  m_module = &M;

  visit(M);
  for (BinaryOperator *I : m_FRems)
    replaceFRem(*I);
  m_FRems.clear();

  m_module = nullptr;
  return m_changed;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses HandleFRemInstructionsNPM::run(Module &M, ModuleAnalysisManager &) {
  bool changed = HandleFRemInstructions().run(M);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
