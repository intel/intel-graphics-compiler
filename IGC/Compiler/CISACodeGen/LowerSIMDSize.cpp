/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// LowerSIMDSize
//
// Replaces every GenISA_simdSize() intrinsic call with the constant SIMD lane
// count once the SIMD mode is known (i.e. ctx->m_CurSimdMode is set before the
// pass runs, which happens in AddCodeGenPasses).  The pass is a no-op when the
// SIMD mode has not been determined yet (SIMDMode::UNKNOWN).
//
// After seeding the worklist with the simdSize replacements, the pass runs a
// closure loop identical to IGCConstProp: it calls ConstantFoldInstruction on
// every user of a newly-constant value so that arithmetic derived from the SIMD
// size (comparisons, selects, multiplies, etc.) is folded away in the same
// pass.

#include "LowerSIMDSize.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Analysis/TargetLibraryInfo.h"

using namespace llvm;
using namespace IGC;

// Testing hook: when non-zero, this lane count is used instead of querying the
// CodeGenContext.  Set via --igc-lower-simd-size-override=<8|16|32>.
static cl::opt<unsigned> OverrideSIMDLanes("igc-lower-simd-size-override", cl::init(0), cl::Hidden,
                                           cl::desc("Override SIMD lane count for LowerSIMDSize (testing only)"));

class LowerSIMDSize : public FunctionPass {
public:
  static char ID;
  LowerSIMDSize() : FunctionPass(ID) { initializeLowerSIMDSizePass(*PassRegistry::getPassRegistry()); }
  StringRef getPassName() const override { return "Lower simdSize intrinsic"; }
  bool runOnFunction(Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
  }
};

char LowerSIMDSize::ID = 0;

#define PASS_FLAG "igc-lower-simd-size"
#define PASS_DESCRIPTION "Replace GenISA_simdSize() with a constant lane count"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowerSIMDSize, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(LowerSIMDSize, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool LowerSIMDSize::runOnFunction(Function &F) {
  uint16_t simdsize;

  if (OverrideSIMDLanes > 0) {
    // OverrideSIMDLanes is a command-line option used only for igc-opt / lit tests
    // to inject a fixed SIMD width without a full driver context.
    simdsize = (uint16_t)OverrideSIMDLanes;
  } else {
    CodeGenContext *ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    // SIMDMode::UNKNOWN  — No mode set yet (pass runs before CodeGen phase).
    // SIMDMode::END      — The PassManager has multiple registered EmitVISA passes for different SIMD sizes.
    //                      It's wrong to lowers GenISA_simdSize() to a specific constant simd size before EmitVISA
    //                      passes.
    // So opt out for these simd modes.
    if (!ctx || ctx->m_CurSimdMode == SIMDMode::UNKNOWN || ctx->m_CurSimdMode == SIMDMode::END)
      return false;
    simdsize = numLanes(ctx->m_CurSimdMode);
  }

  const DataLayout &DL = F.getParent()->getDataLayout();
  TargetLibraryInfo &TLI = getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

  // Look up the simdSize intrinsic declaration by name.
  std::string simdSizeName = GenISAIntrinsic::getName(GenISAIntrinsic::GenISA_simdSize);
  Function *simdSizeFn = F.getParent()->getFunction(simdSizeName);
  if (!simdSizeFn || simdSizeFn->use_empty())
    return false;

  // Collect calls within F.
  SmallVector<CallInst *, 4> simdSizeCalls;
  for (User *U : simdSizeFn->users()) {
    auto *CI = dyn_cast<CallInst>(U);
    if (CI && CI->getParent()->getParent() == &F)
      simdSizeCalls.push_back(CI);
  }

  if (simdSizeCalls.empty())
    return false;

  // Seed the worklist with every user of a simdSize call.  Replace the call
  // itself with the constant SIMD size, then run a closure loop (same pattern
  // as IGCConstProp) to fold any instructions whose operands just became
  // constant.
  SetVector<Instruction *> WorkList;
  Constant *C = ConstantInt::get(simdSizeCalls[0]->getType(), (int32_t)simdsize);
  for (CallInst *CI : simdSizeCalls) {
    for (auto *U : CI->users())
      WorkList.insert(cast<Instruction>(U));
    CI->replaceAllUsesWith(C);
    CI->eraseFromParent();
  }

  // Constant-propagation closure loop (mirrors IGCConstProp::runOnFunction).
  bool NotClosed;
  do {
    NotClosed = false;
    while (!WorkList.empty()) {
      Instruction *I = WorkList.pop_back_val();
      if (I->use_empty())
        continue;

      Constant *FoldedC = ConstantFoldInstruction(I, DL, &TLI);
      if (!FoldedC)
        continue;

      // Enqueue users before replacing so we visit them next.
      for (auto *U : I->users())
        WorkList.insert(cast<Instruction>(U));

      I->replaceAllUsesWith(FoldedC);
      I->eraseFromParent();
      NotClosed = true;
    }
  } while (NotClosed);

  return true;
}

FunctionPass *createLowerSIMDSizePass() { return new LowerSIMDSize(); }
