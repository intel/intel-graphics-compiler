/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "IGC/Compiler/CodeGenPublic.h"
#include "LLVM3DBuilder/BuiltinsFrontend.hpp"
#include "Probe/Assertion.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/PassInfo.h"
#include "llvm/PassRegistry.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// clang-format off
////////////////////////////////////////////////////////////////////////////////
// This pass performs common subexpression elimination for GenISA_WaveBallot
// intrinsics within basic blocks. Since WaveBallot is a convergent intrinsic
// that EarlyCSE doesn't handle, this pass identifies redundant calls with
// matching arguments in the same basic block and replaces later uses with
// the first occurrence.
//
// Code example:
// Before:
//     %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
//     ; other instructions...
//     %mask2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
//     ; use %mask1 and %mask2...
// After:
//     %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
//     ; other instructions...
//     ; %mask2 removed, all uses replaced with %mask1
//     ; use %mask1...
//
////////////////////////////////////////////////////////////////////////////////
// clang-format on

class WaveBallotCSE : public llvm::FunctionPass {
public:
  static char ID;

  WaveBallotCSE();

  llvm::StringRef getPassName() const override { return "WaveBallotCSE"; }

  ////////////////////////////////////////////////////////////////////////
  bool runOnFunction(llvm::Function &F) override;

  ////////////////////////////////////////////////////////////////////////
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

private:
  bool ProcessFunction(llvm::Function &F);
  bool ProcessWaveBallotInBB(BasicBlock &BB);
};

#define PASS_FLAG "wave-ballot-cse"
#define PASS_DESCRIPTION "WaveBallotCSE"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(WaveBallotCSE, PASS_FLAG, PASS_DESCRIPTION,
                          PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(WaveBallotCSE, PASS_FLAG, PASS_DESCRIPTION,
                        PASS_CFG_ONLY, PASS_ANALYSIS)

char WaveBallotCSE::ID = 0;

////////////////////////////////////////////////////////////////////////////
WaveBallotCSE::WaveBallotCSE() : llvm::FunctionPass(ID) {
  initializeWaveBallotCSEPass(*PassRegistry::getPassRegistry());
}

bool WaveBallotCSE::ProcessWaveBallotInBB(BasicBlock &BB) {

  std::vector<GenIntrinsicInst *> waveBallotCalls;
  std::vector<GenIntrinsicInst *> removeList;

  bool modified = false;

  for (auto &I : BB) {
    if (auto *waveIntr = dyn_cast<GenIntrinsicInst>(&I)) {
      if (GenISAIntrinsic::GenISA_WaveBallot == waveIntr->getIntrinsicID()) {
        waveBallotCalls.push_back(waveIntr);
      }
    }
  }

  // If we have less than 2 calls, no CSE opportunity
  if (waveBallotCalls.size() < 2)
    return false;

  // For each call, check if there's an earlier equivalent call
  for (size_t i = 1; i < waveBallotCalls.size(); ++i) {
    GenIntrinsicInst *CurrentCall = waveBallotCalls[i];

    // Look for an earlier call with the same arguments
    for (size_t j = 0; j < i; ++j) {
      GenIntrinsicInst *EarlierCall = waveBallotCalls[j];

      if (CurrentCall->isIdenticalToWhenDefined(EarlierCall)) {
        // Save the redundant WaveBallot
        removeList.push_back(CurrentCall);
        // Replace all uses of the current call with the earlier one
        CurrentCall->replaceAllUsesWith(EarlierCall);

        modified = true;
        break; // Found a replacement, move to next call
      }
    }
  }

  // Now remove redundant WaveBallot
  for (auto *it : removeList) {
    it->eraseFromParent();
  }

  return modified;
}

bool WaveBallotCSE::ProcessFunction(llvm::Function &F) {
  bool modified = false;

  // Process each basic block independently
  // This might not be ideal. But for safety, we currently keep each BB to has
  // its own unique WaveBallot intrinsic and remove the rest redundants.
  for (BasicBlock &BB : F) {
    modified |= ProcessWaveBallotInBB(BB);
  }

  return modified;
}

////////////////////////////////////////////////////////////////////////////
bool WaveBallotCSE::runOnFunction(llvm::Function &F) {

  return ProcessFunction(F);
}

////////////////////////////////////////////////////////////////////////
void WaveBallotCSE::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesCFG();
}

////////////////////////////////////////////////////////////////////////
FunctionPass *createWaveBallotCSE() { return new WaveBallotCSE(); }
