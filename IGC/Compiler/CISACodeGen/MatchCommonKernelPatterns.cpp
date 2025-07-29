/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "MatchCommonKernelPatterns.hpp"

using namespace llvm;
using namespace IGC;

char MatchCommonKernelPatterns::ID = 0;

#define INTERPRETER_CASES_NUMBER 18
#define PASS_FLAG "match-common-kernel-patterns"
#define PASS_DESCRIPTION "Match common kernel patterns"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MatchCommonKernelPatterns, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(MatchCommonKernelPatterns, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

MatchCommonKernelPatterns::MatchCommonKernelPatterns() : FunctionPass(ID) {
  initializeMatchCommonKernelPatternsPass(*PassRegistry::getPassRegistry());
}

bool MatchCommonKernelPatterns::runOnFunction(Function &F) {
  Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  // Check if F is part of interpreter pattern.
  if (IGC_IS_FLAG_ENABLED(EnableInterpreterPatternMatching) && !Ctx->platform.isCoreXE3())
    if (isInterpreterPattern(F))
      Ctx->m_kernelsWithForcedRetry.push_back(&F);

  return false;
}

bool MatchCommonKernelPatterns::isInterpreterPattern(const Function &F) const {
  // Skip non-kernel functions.
  if (F.getCallingConv() != CallingConv::SPIR_KERNEL)
    return false;

  // Skip kernels with less than 100 basic blocks.
  if (F.size() < 100)
    return false;

  // Iterate over function and check if it has a pattern of Interpreter kernel.
  for (const auto &BB : F)
    if (isBBPartOfInterpreterPattern(&BB))
      return true;

  return false;
}

bool MatchCommonKernelPatterns::isBBPartOfInterpreterPattern(const BasicBlock *BB) const {
  // Try to find BB that is a case of the switch. Try to get a switch argument.
  if (BB->size() != 2)
    return false;

  // Try to find the first switch case.
  Value *CurrentSwitchArg = getInterpreterSwitchArg(BB);
  if (!CurrentSwitchArg)
    return false;

  // Try to find the entrance of the switch.
  const BasicBlock *SwitchEntrance = BB->getSinglePredecessor();
  if (!SwitchEntrance)
    return false;

  // The switch entrance should have conditional branch and the switch argument should be loaded from memory.
  Value *SwitchArg = getInterpreterSwitchArg(SwitchEntrance);
  if (SwitchArg != CurrentSwitchArg)
    return false;

  if (auto Extr = dyn_cast<ExtractElementInst>(SwitchArg)) {
    if (!isa<LoadInst>(Extr->getOperand(0)))
      return false;
  } else if (!isa<LoadInst>(SwitchArg)) {
    return false;
  }

  // Try to find other switch cases.
  size_t Cases = 0;
  for (auto User = SwitchArg->user_begin(), End = SwitchArg->user_end(); User != End; ++User) {
    BasicBlock *CurrentBB = cast<Instruction>(*User)->getParent();
    // Skip the switch entrance and BB that were already checked.
    if (BB == CurrentBB || SwitchEntrance == CurrentBB)
      continue;

    if (getInterpreterSwitchArg(CurrentBB) != SwitchArg)
      continue;

    // Check if CurrentBB has at least one branch which leads to the switch entrance.
    bool HasBackEdge = false;
    for (auto SI = succ_begin(CurrentBB), E = succ_end(CurrentBB); SI != E; ++SI)
      HasBackEdge |= isBBBackEdge(*SI, SwitchEntrance);

    if (HasBackEdge)
      Cases++;

    // If switch has more than INTERPRETER_CASES_NUMBER cases then it is a Interpreter pattern.
    if (Cases == INTERPRETER_CASES_NUMBER)
      return true;
  }

  return false;
}

bool MatchCommonKernelPatterns::isBBBackEdge(const BasicBlock *BB, const BasicBlock *EntryBB) const {
  // Try to find critical edge or back edge.
  const BasicBlock *CritOrBackEdge = BB->getSingleSuccessor();
  if (!CritOrBackEdge)
    return false;

  if (CritOrBackEdge == EntryBB)
    return true;

  return CritOrBackEdge->getSingleSuccessor() == EntryBB;
}

Value *MatchCommonKernelPatterns::getInterpreterSwitchArg(const BasicBlock *BB) const {
  const BranchInst *Br = dyn_cast<const BranchInst>(BB->getTerminator());
  if (!Br || !Br->isConditional())
    return nullptr;

  const Value *Condition = Br->getCondition();
  if (!isa<ICmpInst>(Condition))
    return nullptr;

  const ICmpInst *Cmp = cast<const ICmpInst>(Condition);
  Value *NonConstOperand = nullptr;
  if (isa<ConstantInt>(Cmp->getOperand(0))) {
    NonConstOperand = Cmp->getOperand(1);
  } else if (isa<ConstantInt>(Cmp->getOperand(1))) {
    NonConstOperand = Cmp->getOperand(0);
  } else {
    return nullptr;
  }

  return NonConstOperand;
}