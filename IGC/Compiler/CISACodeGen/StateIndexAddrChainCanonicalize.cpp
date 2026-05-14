/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "StateIndexAddrChainCanonicalize.hpp"

#include "Compiler/CodeGenPublic.h"
#include "common/ResourceAddrSpace.h"
#include "common/igc_regkeys.hpp"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Canonicalizes addr-chains feeding inttoptr-to-stateful-resource-pointer so
// that any buried constant offset is hoisted into a single ConstantInt operand
// at the outermost add. After this pass, the downstream `matchStateIndex`
// lookup needs only a one-shot
//   m_IntToPtr(m_c_Add(m_Value, m_ConstantInt))
// match — no mid-pattern-match IR mutation is required.
//
// Targeting: runOnFunction skips inttoptr whose destination address space is
// not stateful (isStatefulAddrSpace). The SSSize-alignment guard in analyze()
// further restricts rewrites to chains whose constant sum matches the
// matchStateIndex alignment contract.
//
// Must run before WIAnalysis: any new add instructions created here need
// fresh WI-dep entries, which the subsequent WIAnalysis pass produces
// automatically when it walks the canonicalized IR.
class StateIndexAddrChainCanonicalize : public llvm::FunctionPass {
public:
  static char ID;
  StateIndexAddrChainCanonicalize();

  bool runOnFunction(llvm::Function &F) override;

  llvm::StringRef getPassName() const override { return "StateIndexAddrChainCanonicalize"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

#define PASS_FLAG "igc-state-index-addr-chain-canonicalize"
#define PASS_DESCRIPTION "IGC State-Index Addr-Chain Canonicalize"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(StateIndexAddrChainCanonicalize, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(StateIndexAddrChainCanonicalize, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char StateIndexAddrChainCanonicalize::ID = 0;

StateIndexAddrChainCanonicalize::StateIndexAddrChainCanonicalize() : llvm::FunctionPass(ID) {
  initializeStateIndexAddrChainCanonicalizePass(*llvm::PassRegistry::getPassRegistry());
}

void StateIndexAddrChainCanonicalize::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<CodeGenContextWrapper>();
  // We mutate IR (create new adds, delete the old chain) but preserve CFG.
  AU.setPreservesCFG();
}

namespace {

// Cap nested-add depth to bound the leaves' live-range extension.
// Walks the add tree to a maximum recursion depth of MaxDepth+1 (top plus
// MaxDepth nested levels). An Add observed at recursion depth MaxDepth+1
// triggers bail; partial collection is discarded.
constexpr unsigned MaxDepth = 2;

// Maximum number of non-constant leaves allowed in a canonicalized chain.
// Each leaf's live range is extended to the inttoptr site. With 3+ leaves
// the compound GRF pressure increase often outweighs the single saved add.
constexpr unsigned MaxNonConstLeaves = 2;

struct CanonPlan {
  SmallVector<Value *, 4> NonConstLeaves;
  uint64_t ConstSum = 0;
  bool Viable = false;
  bool Bail = false;
  bool FoundBuriedConst = false;
};

static void collect(CanonPlan &Plan, Value *V, bool IsTop, unsigned Depth) {
  if (Plan.Bail)
    return;
  if (auto *CI = dyn_cast<ConstantInt>(V)) {
    if (CI->getValue().getActiveBits() > 64) {
      Plan.Bail = true;
      return;
    }
    Plan.ConstSum += CI->getZExtValue();
    return;
  }
  auto *I = dyn_cast<Instruction>(V);
  if (!I || I->getOpcode() != Instruction::Add || (!IsTop && !I->hasOneUse())) {
    Plan.NonConstLeaves.push_back(V);
    return;
  }
  if (Depth > MaxDepth) {
    Plan.Bail = true;
    return;
  }
  if (!IsTop && (isa<ConstantInt>(I->getOperand(0)) || isa<ConstantInt>(I->getOperand(1))))
    Plan.FoundBuriedConst = true;
  collect(Plan, I->getOperand(0), false, Depth + 1);
  collect(Plan, I->getOperand(1), false, Depth + 1);
}

// Pure analysis: walks the add-chain rooted at I2P->getOperand(0). Does not
// mutate IR. Bails on shapes where the rewrite would be a regression
// (multi-use intermediates, multi-use top, deep chains, no buried constant).
static CanonPlan analyze(IntToPtrInst *I2P, CodeGenContext *Ctx) {
  CanonPlan Plan;

  Value *Top = I2P->getOperand(0);
  // matchStateIndex requires the base to be 64-bit (PatternMatchPass.cpp).
  // Rewriting a narrower chain extends leaf live ranges for zero benefit
  // because matchStateIndex would reject the result on the width check.
  if (!Top->getType()->isIntegerTy(64))
    return Plan;
  // Top must be single-use: if other consumers hold the chain alive, deleting
  // it is impossible and we'd net out at "old chain + new chain" — strict
  // regression.
  if (!Top->hasOneUse())
    return Plan;

  collect(Plan, Top, true, 0);

  // Bail if the chain has too many non-const leaves. Each leaf's live range
  // is extended to the inttoptr site after canonicalization, increasing the
  // number of simultaneously live values. With N >= 3 leaves the compound
  // GRF pressure increase can cause spills costing more ALU time than the
  // single add instruction saved by matchStateIndex folding.
  if (Plan.NonConstLeaves.size() > MaxNonConstLeaves)
    return Plan;

  if (Plan.Bail || !Plan.FoundBuriedConst || Plan.NonConstLeaves.empty() || Plan.ConstSum == 0)
    return Plan;

  // Live-range distance guard: bail if any non-const leaf is defined in a
  // different basic block than the inttoptr. Cross-BB live-range extensions
  // are the primary regression mechanism -- the leaf must stay live across
  // intervening blocks, increasing GRF pressure at join points. Same-BB
  // extensions are bounded by block size and typically benign.
  BasicBlock *I2PBlock = I2P->getParent();
  for (Value *Leaf : Plan.NonConstLeaves) {
    if (auto *LI = dyn_cast<Instruction>(Leaf)) {
      if (LI->getParent() != I2PBlock) {
        return Plan; // Plan.Viable remains false
      }
    }
  }

  // Guards mirror the downstream matchStateIndex contract in
  // CodeGenPatternMatch::matchStateIndex (PatternMatchPass.cpp). Without
  // them, the chain is restructured but matchStateIndex still returns {},
  // so leaf live ranges grow for zero benefit — pure ALU-time regression.
  unsigned SSSize = Ctx->m_DriverInfo.getSurfaceStateSize();
  if (SSSize == 0)
    return Plan;

  // (1) Alignment: ConstSum must be a multiple of the surface state size.
  if (Plan.ConstSum % SSSize != 0)
    return Plan;

  // (2) Encoding range: matchStateIndex caps the folded index at < 32
  if (Plan.ConstSum / SSSize >= 32)
    return Plan;

  // (3) Sampler-path guard: matchStateIndex uses GetBindlessSamplerSize()
  //     (not getSurfaceStateSize()) and caps the sampler index at < 7 when
  //     the inttoptr feeds a sampler intrinsic. We cannot distinguish
  //     surface vs. sampler consumers at this point, so conservatively
  //     reject chains whose constant sum would pass the surface check but
  //     fail the sampler check. This avoids restructuring + live-range
  //     extension for zero benefit on the sampler path.
  unsigned BSSize = Ctx->platform.GetBindlessSamplerSize();
  if (BSSize > 0 && Plan.ConstSum % BSSize == 0 && +Plan.ConstSum / BSSize >= 7)
    return Plan;

  // Note: live-range/RP risk is throttled at the function-level instruction
  // count gate in runOnFunction, not per-leaf here.

  Plan.Viable = true;
  return Plan;
}

// Builds the new chain at the old top's position and rewires the inttoptr.
// Old chain becomes dead and is DCE'd.
static void commit(IntToPtrInst *I2P, const CanonPlan &Plan) {
  IGC_ASSERT(Plan.Viable);
  Value *Top = I2P->getOperand(0);

  auto *TopI = cast<Instruction>(Top);
  IRBuilder<> B(TopI);
  B.SetCurrentDebugLocation(TopI->getDebugLoc());
  Value *Acc = Plan.NonConstLeaves[0];
  for (size_t i = 1; i < Plan.NonConstLeaves.size(); ++i)
    Acc = B.CreateAdd(Acc, Plan.NonConstLeaves[i]);
  Value *NewTop = B.CreateAdd(Acc, ConstantInt::get(Top->getType(), Plan.ConstSum));

  I2P->setOperand(0, NewTop);
  RecursivelyDeleteTriviallyDeadInstructions(Top);
}

} // namespace

bool StateIndexAddrChainCanonicalize::runOnFunction(Function &F) {
  if (IGC_IS_FLAG_ENABLED(DisableStatefulFolding))
    return false;
  if (IGC_IS_FLAG_DISABLED(EnableStateIndexAddrChainCanonicalize))
    return false;

  auto *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (!Ctx->platform.hasEfficient64bEnabled())
    return false;

  // Hoisting moves the new chain to the inttoptr site, extending every
  // non-const leaf's live range to that point. In large functions near the
  // GRF pressure limit this can perturb the register allocator and cause
  // ALU-time regressions. Skip the pass when the function's instruction
  // count exceeds the threshold (0 disables the gate).
  unsigned InstThreshold = IGC_GET_FLAG_VALUE(StateIndexAddrChainCanonicalizeInstThreshold);
  if (InstThreshold != 0) {
    unsigned FuncInstCount = 0;
    for (const BasicBlock &BB : F)
      FuncInstCount += BB.size();
    if (FuncInstCount > InstThreshold)
      return false;
  }

  bool Changed = false;
  // Iterate using make_early_inc_range — commit() may delete the inttoptr's
  // operand chain (instructions earlier in the BB), but never deletes the
  // inttoptr itself, so the outer iteration is safe with this guard.
  for (BasicBlock &BB : F) {
    for (Instruction &I : llvm::make_early_inc_range(BB)) {
      auto *I2P = dyn_cast<IntToPtrInst>(&I);
      if (!I2P)
        continue;
      if (!IGC::isStatefulAddrSpace(I2P->getType()->getPointerAddressSpace()))
        continue;
      CanonPlan Plan = analyze(I2P, Ctx);
      if (!Plan.Viable)
        continue;
      commit(I2P, Plan);
      Changed = true;
    }
  }
  return Changed;
}

FunctionPass *createStateIndexAddrChainCanonicalizePass() { return new StateIndexAddrChainCanonicalize(); }
