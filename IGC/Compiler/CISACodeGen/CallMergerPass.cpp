/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "CallMergerPass.hpp"

#include "CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "llvmWrapper/IR/BasicBlock.h"

#include "Probe/Assertion.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Use.h>
#include <llvm/Pass.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

// Register pass to igc-opt
namespace IGC
{
#define PASS_FLAG "call-merger-pass"
#define PASS_DESCRIPTION                                                       \
  "Merge mutually exclusive calls to enable further inlining."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CallMerger, PASS_FLAG, PASS_DESCRIPTION,
                          PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(EstimateFunctionSize)
IGC_INITIALIZE_PASS_END(CallMerger, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                        PASS_ANALYSIS)
} // namespace IGC

using CallSiteMap = DenseMap<Function *, SmallVector<CallInst *, 2>>;

namespace {
CallSiteMap collectAllCallSites(Function &F) {
  CallSiteMap callSites;
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *callInst = dyn_cast<CallInst>(&I)) {
        auto *calledFunc = callInst->getCalledFunction();
        if (calledFunc && !calledFunc->isIntrinsic()) {
          callSites[calledFunc].push_back(callInst);
        }
      }
    }
  }
  return callSites;
}

void setNewTerminator(BasicBlock *oldBB, BasicBlock *newBB) {
  auto *oldTerminator = oldBB->getTerminator();
  oldTerminator->eraseFromParent();
  IGCLLVM::pushBackInstruction(oldBB, BranchInst::Create(newBB));
}

// We assume that BBs with call instructions have terminator with single successor
// and that the list of uses of both calls is the same.
void mergeCalls(Function* F, CallInst *call1, CallInst *call2) {
  auto* parentBB1 = call1->getParent();
  auto* parentBB2 = call2->getParent();
  auto* successorBB = parentBB1->getSingleSuccessor();

  auto* newBB = llvm::BasicBlock::Create(F->getContext(), "mergedCallsBB", F, successorBB);
  llvm::IRBuilder<> Builder(newBB);

  IGC_ASSERT(call1->arg_size() == call2->arg_size());

  SmallVector<Value*, 4> args;
  for (unsigned i = 0; i < call1->arg_size(); ++i) {
    auto *arg1 = call1->getArgOperand(i);
    auto *arg2 = call2->getArgOperand(i);
    if (arg1 == arg2) {
      args.push_back(arg1);
      continue;
    }
    auto *PN = Builder.CreatePHI(arg1->getType(), 2);
    PN->addIncoming(arg1, parentBB1);
    PN->addIncoming(arg2, parentBB2);
    args.push_back(PN);
  }
  auto* newCall = Builder.CreateCall(call1->getCalledFunction(), args);
  newCall->setCallingConv(call1->getCallingConv());
  newCall->setAttributes(call1->getAttributes());
  newCall->setTailCall(call1->isTailCall());
  Builder.CreateBr(successorBB);

  setNewTerminator(parentBB1, newBB);
  setNewTerminator(parentBB2, newBB);
  for (auto& u: call1->uses()) {
    auto *userI = cast<Instruction>(u.getUser());
    userI->replaceUsesOfWith(call1, newCall);
  }
  for (auto& u: call2->uses()) {
    auto *userI = cast<Instruction>(u.getUser());
    userI->replaceUsesOfWith(call2, newCall);
  }
  call1->eraseFromParent();
  call2->eraseFromParent();
}

bool haveSingleCommonSuccessor(CallInst *call1, CallInst *call2) {
  auto *successor1 = call1->getParent()->getSingleSuccessor();
  auto *successor2 = call2->getParent()->getSingleSuccessor();
  if (!successor1 || !successor2 || successor1 != successor2) {
    return false;
  }
  return true;
}

bool isAfterInstInBB(Instruction* inst1, Instruction* inst2){
  for (auto& I : *inst1->getParent()) {
    if (&I == inst1) {
      return false;
    }
    if (&I == inst2) {
      return true;
    }
  }
  return false;
}

bool hasUsesInCurrentBB(CallInst *call) {
  auto *currentBB = call->getParent();

  // Check if call results is used in same block as call
  for (auto *user : call->users()) {
    auto* userI = cast<Instruction>(user);
    if (userI->getParent() == currentBB) {
      return true;
    }
  }

  // Check if any non const argument is used in call block
  // after call
  for (auto& arg : call->args()) {
    if (!arg->getType()->isPointerTy()) {
      continue;
    }
    for (auto *user : arg->users()) {
      if (auto* userI = dyn_cast<Instruction>(user)) {
        if (userI == call || userI->getParent() != currentBB) {
          continue;
        }
        if (isAfterInstInBB(call, userI)) {
          continue;
        }
        return true;
      }
    }
  }
  return false;
}

bool hasSameUsesAs(CallInst *call1, CallInst *call2) {
  if (call1->getNumUses() != call2->getNumUses()) {
    return false;
  }

  for (auto &use1 : call1->uses()) {
    bool matched = false;
    for (auto &use2 : call2->uses()) {
      if (use1 == use2) {
        matched = true;
        break;
      }
    }
    if (!matched) {
      return false;
    }
  }
  return true;
}

void filterCallSites(CallSiteMap &callSites, EstimateFunctionSize *EFS) {
  SmallVector<Function*, 4> elementsToErase;
  size_t PerFuncThreshold = IGC_GET_FLAG_VALUE(SubroutineInlinerThreshold);

  for (const auto&[calledFunc, callInsts] : callSites) {
    if (callInsts.size() != 2) {
      elementsToErase.push_back(calledFunc);
      continue;
    }

    // We don't need to process function that can't get inlined
    if (calledFunc->hasFnAttribute(llvm::Attribute::NoInline) ||
        calledFunc->hasFnAttribute("igc-force-stackcall") ||
        calledFunc->hasFnAttribute("KMPLOCK")){
      elementsToErase.push_back(calledFunc);
      continue;
    }

    // We can skip functions that are small enough to be inlined.
    if (EFS->getExpandedSize(calledFunc) <= PerFuncThreshold) {
        elementsToErase.push_back(calledFunc);
        continue;
    }

    // We can merge calls with common successor, without result or args having uses in
    // call block. We also only merge function calls with same use list.
    if (!haveSingleCommonSuccessor(callInsts[0], callInsts[1]) ||
        hasUsesInCurrentBB(callInsts[0]) ||
        hasUsesInCurrentBB(callInsts[1]) ||
      !hasSameUsesAs(callInsts[0], callInsts[1])) {
      elementsToErase.push_back(calledFunc);
      continue;
    }
  }

  for (auto *calledFunc : elementsToErase) {
    callSites.erase(calledFunc);
  }
}
} // anonymous namespace

char CallMerger::ID = 0;

CallMerger::CallMerger() : ModulePass(ID) {
  initializeCallMergerPass(*PassRegistry::getPassRegistry());
}

void CallMerger::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<EstimateFunctionSize>();
}

bool CallMerger::runOnFunction(Function& F) {
    auto callSites = collectAllCallSites(F);

    filterCallSites(callSites, EFS);
    if (callSites.empty()) {
        return false;
    }

    for (auto&[calledFunc, callInsts] : callSites) {
      mergeCalls(&F, callInsts[0], callInsts[1]);
    }

    return true;
}

bool CallMerger::runOnModule(Module &M) {
  EFS = &getAnalysis<EstimateFunctionSize>();
  CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  // We don't need to do any work if all functions will get inlined
  // or function control is not default.
  if (IGC::ForceAlwaysInline(CTX) ||
      CTX->m_enableSubroutine == false ||
      getFunctionControl(CTX) != FLAG_FCALL_DEFAULT ||
      !EFS->shouldEnableSubroutine()) {
    return false;
  }

  bool changed = false;
  for (auto &F : M) {
    if (F.isDeclaration() || F.isIntrinsic() || F.hasOptNone()) {
      continue;
    }
    changed |= runOnFunction(F);
  }
  return changed;
}