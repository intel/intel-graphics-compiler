/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/GenericCastToPtrOpt/GenericCastToPtrOpt.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "CISACodeGen/CastToGASAnalysis.h"
#include "Probe/Assertion.h"
#include "Compiler/IGCPassSupport.h"
#include "llvmWrapper/ADT/Optional.h"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-generic-cast-to-ptr-opt"
#define PASS_DESCRIPTION "Optimize GenericCastToPtrExplicit casts"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenericCastToPtrOpt, PASS_FLAG, PASS_DESCRIPTION,
                          PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CastToGASAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(GenericCastToPtrOpt, PASS_FLAG, PASS_DESCRIPTION,
                        PASS_CFG_ONLY, PASS_ANALYSIS)

char GenericCastToPtrOpt::ID = 0;

constexpr std::string_view GENERIC_CAST_TO_PTR_FN_NAME =
    "spirv_GenericCastToPtrExplicit_ToGlobal";

static void replaceGenericCastToPtrCall(CallInst *TargetFnCall) {
  IRBuilder<> Builder(TargetFnCall->getParent());
  Builder.SetInsertPoint(TargetFnCall);
  auto *AddrSpaceCast = Builder.CreateAddrSpaceCast(
      TargetFnCall->getArgOperand(0),
      TargetFnCall->getCalledFunction()->getReturnType(),
      "generic_cast_to_ptr");
  IGC_ASSERT(TargetFnCall->getCalledFunction()->getReturnType()->isPointerTy());
  IGC_ASSERT(TargetFnCall->getArgOperand(0)->getType()->isPointerTy());
  TargetFnCall->replaceAllUsesWith(AddrSpaceCast);
  TargetFnCall->eraseFromParent();

  // Clean up the users of the address space cast
  auto *NewAddrSpaceCast = dyn_cast<AddrSpaceCastInst>(AddrSpaceCast);
  SmallVector<AddrSpaceCastInst *, 32> UsersToRemove;
  for (auto *User : NewAddrSpaceCast->users()) {
    if (auto *AddrSpaceCastUser = dyn_cast<AddrSpaceCastInst>(User)) {
      if (NewAddrSpaceCast->getSrcTy() == AddrSpaceCastUser->getDestTy()) {
        AddrSpaceCastUser->replaceAllUsesWith(NewAddrSpaceCast->getOperand(0));
        UsersToRemove.push_back(AddrSpaceCastUser);
      }
    }
  }
  for (auto *User : UsersToRemove) {
    User->eraseFromParent();
  }
}

GenericCastToPtrOpt::GenericCastToPtrOpt() : ModulePass(ID) {
  initializeGenericCastToPtrOptPass(*PassRegistry::getPassRegistry());
}

void GenericCastToPtrOpt::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<CastToGASAnalysis>();
  AU.addRequired<CallGraphWrapperPass>();

  AU.setPreservesCFG();
}

bool GenericCastToPtrOpt::runOnModule(Module &M) {
  if (skipModule(M)) {
    return false;
  }

  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  GASInfo &GI = getAnalysis<CastToGASAnalysis>().getGASInfo();
  const bool noGenericPtToLocalOrPrivate =
      GI.isNoLocalToGenericOptionEnabled() &&
      GI.isPrivateAllocatedInGlobalMemory();

  bool modified = false;

  for (auto &[Fn, FnCallGraph] : CG) {
    if (!Fn || Fn->hasOptNone()) {
      continue;
    }

    const bool noGenericPtToLocalOrPrivateFn =
        !GI.canGenericPointToLocal(*FnCallGraph->getFunction()) &&
        !GI.canGenericPointToPrivate(*FnCallGraph->getFunction());
    if (!noGenericPtToLocalOrPrivate && !noGenericPtToLocalOrPrivateFn) {
      continue;
    }

    for (auto &[CallInstVH, CallRecordNode] : *FnCallGraph.get()) {
      // We check only direct calls
      auto CallInstVHOptional = IGCLLVM::makeOptional(CallInstVH);
      if (!CallInstVHOptional.has_value()) {
        continue;
      }
      auto *CallRecordFn = CallRecordNode->getFunction();
      if (CallRecordFn && CallRecordFn->getName().contains(llvm::StringRef(
                              GENERIC_CAST_TO_PTR_FN_NAME.data(),
                              GENERIC_CAST_TO_PTR_FN_NAME.size()))) {
        replaceGenericCastToPtrCall(cast<CallInst>(CallInstVHOptional.value()));
        modified = true;
      }
    }
  }
  return modified;
}