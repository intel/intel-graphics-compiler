/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ProcessBICodeAssumption/ProcessBICodeAssumption.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PatternMatch.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"

using namespace llvm;
using namespace llvm::PatternMatch;
using namespace IGC;

// OpenCL standard defines built-in variables like get_global_id as size_t, which translates to i64, but in reality
// many workloads use values that fit in i32. This pass checks if there are assumptions on built-in variables and adds
// trunc/zext intructions to reflect that the upper 32 bits are not used. This helps instcombine to optimize code after
// BIImport.
class ProcessBICodeAssumption : public llvm::FunctionPass, public llvm::InstVisitor<ProcessBICodeAssumption> {
public:
  static char ID;
  ProcessBICodeAssumption();

  virtual llvm::StringRef getPassName() const override { return "ProcessBICodeAssumption"; }

  virtual bool runOnFunction(Function &F) override;
  void visitCallInst(CallInst &I);

private:
  bool matchCmp(ICmpInst::Predicate Pred, ConstantInt *CI);
  bool matchBuiltin(Instruction *I);
  void matchVectorPattern(Instruction *I);

  SmallPtrSet<Instruction *, 8> ToTruncate;
};

ProcessBICodeAssumption::ProcessBICodeAssumption() : FunctionPass(ID) {
  initializeProcessBICodeAssumptionPass(*PassRegistry::getPassRegistry());
}

bool ProcessBICodeAssumption::runOnFunction(Function &F) {

  ToTruncate.clear();
  visit(F);

  if (ToTruncate.empty())
    return false;

  // Insert trunc/zext for each matched builtin call.
  for (auto *I : ToTruncate) {

    IRBuilder<> Builder(I);
    Builder.SetInsertPoint(I->getNextNode());

    auto Trunc = Builder.CreateTrunc(I, Builder.getInt32Ty());
    auto Zext = Builder.CreateZExt(Trunc, Builder.getInt64Ty());

    for (auto It = I->use_begin(), E = I->use_end(); It != E; ) {
      auto Use = It++;
      if (Use->getUser() != Trunc)
        Use->set(Zext);
    }
  }

  return true;
}

void ProcessBICodeAssumption::visitCallInst(CallInst &CI) {

  Instruction *I = nullptr;
  ICmpInst::Predicate Pred;
  ConstantInt *Const = nullptr;

  // Look for assume:
  //   %9 = icmp ult i64 %8, 2147483648
  //   call void @llvm.assume(i1 %9)
  if (!match(&CI, m_Intrinsic<Intrinsic::assume>(m_ICmp(Pred, m_Instruction(I), m_ConstantInt(Const)))))
    return;

  if (!matchCmp(Pred, Const))
    return;

  if (matchBuiltin(I)) {
    ToTruncate.insert(I);
    return;
  }

  matchVectorPattern(I);
}

// Look for pattern with insertelement/extractelement:
//   %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
//   %2 = insertelement <3 x i64> undef, i64 %1, i32 0
//   %3 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1)
//   %4 = insertelement <3 x i64> %2, i64 %3, i32 1
//   %5 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2)
//   %6 = insertelement <3 x i64> %4, i64 %5, i32 2
//   %7 = extractelement <3 x i64> %6, i32 0
void ProcessBICodeAssumption::matchVectorPattern(Instruction *I) {

  Instruction *EE = I;

  // Optional select
  //   %7 = extractelement <3 x i64> %6, i32 0
  //   %8 = select i1 true, i64 %7, i64 0
  if (match(EE, m_Select(m_One(), m_Instruction(I), m_Value())))
    EE = I;

  Value *IE = nullptr, *NextIE = nullptr;
  ConstantInt *EConst = nullptr, *IConst = nullptr;

  if (!match(EE, m_ExtractElt(m_Value(IE), m_ConstantInt(EConst))))
    return;

  while (match(IE, m_InsertElt(m_Value(NextIE), m_Instruction(I), m_ConstantInt(IConst)))) {
    if (IConst->getZExtValue() == EConst->getZExtValue()) {
      if (matchBuiltin(I))
        ToTruncate.insert(I);
      return;
    }
    IE = NextIE;
  }
}

bool ProcessBICodeAssumption::matchCmp(ICmpInst::Predicate Pred, ConstantInt *CI) {
  switch (Pred) {
  case ICmpInst::ICMP_ULE:
    return CI->getZExtValue() <= llvm::APInt::getMaxValue(32).getZExtValue();
  case ICmpInst::ICMP_ULT:
    return CI->getZExtValue() <= llvm::APInt::getMaxValue(32).getZExtValue() + 1;
  case ICmpInst::ICMP_SLE:
    return CI->getZExtValue() <= llvm::APInt::getSignedMaxValue(32).getZExtValue();
  case ICmpInst::ICMP_SLT:
    return CI->getZExtValue() <= llvm::APInt::getSignedMaxValue(32).getZExtValue() + 1;
  default:
    return false;
  }
}

bool ProcessBICodeAssumption::matchBuiltin(Instruction *I) {
  if (auto CI = dyn_cast<CallInst>(I)) {
    return CI->getCalledFunction()->getName() == "_Z33__spirv_BuiltInGlobalInvocationIdi" ||
           CI->getCalledFunction()->getName() == "_Z29__spirv_BuiltInGlobalLinearIdv";
  }
  return false;
}

// Register pass to igc-opt
#define PASS_FLAG "igc-process-bi-code-assumption"
#define PASS_DESCRIPTION "Processes code assumptions assigned to builtin variables"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ProcessBICodeAssumption, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ProcessBICodeAssumption, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ProcessBICodeAssumption::ID = 0;

FunctionPass *IGC::createProcessBICodeAssumptionPass() { return new ProcessBICodeAssumption(); }
