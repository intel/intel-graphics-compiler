/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "StateIndexAddrChainCanonicalize.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "common/ResourceAddrSpace.h"
#include "common/igc_regkeys.hpp"
#include "Probe/Assertion.h"

#include <functional>
#include <optional>
#include <utility>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
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
// Constants under `zext(add X, C)` are hoisted too (see matchZExtOfAddConst).
// Equivalent bases are then merged per block (unifyBases) so constant-indexed
// descriptor arrays share one base with different state indices.
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

// Non-constant leaf; NeedsZExt marks X peeled out of `zext(add X, C)`.
struct Leaf {
  Value *V = nullptr;
  bool NeedsZExt = false;
};

struct CanonPlan {
  SmallVector<Leaf, 4> NonConstLeaves;
  uint64_t ConstSum = 0;
  bool Viable = false;
  bool Bail = false;
  bool FoundBuriedConst = false;
  // StateIndexCanonMode::FoldZExt: zext fold and shared chains.
  bool FoldZExt = false;
  // StateIndexCanonMode::MergeBases: reuse existing zext/add bases.
  bool MergeBases = false;
  // See matchZExtOfAddConst().
  bool AssumeNoOffsetWrap = false;
};

// Max C for `zext(add i32 X, C)` without nuw. Covers the encodable index
// range and rejects "negative" constants such as 0xFFFFFFC0 (X - 64).
constexpr uint64_t MaxAssumedNoWrapOffset = 1u << 16;

// Matches single-use `zext(add X, C)` so C can be hoisted:
// zext(X + C) == zext(X) + C when X + C does not wrap. Exact with nuw.
// Without nuw (dropped by frontend optimizations) it is accepted only for
// bindless surfaces whose X is a driver-supplied base offset (argument, load
// or call), not in-shader arithmetic: X is then an in-heap offset, and a wrap
// of X + C would be an out-of-bounds descriptor access.
static bool matchZExtOfAddConst(Value *V, bool AssumeNoOffsetWrap, Value *&X, ConstantInt *&C) {
  auto *ZE = dyn_cast<ZExtInst>(V);
  if (!ZE || !ZE->hasOneUse())
    return false;
  auto *Add = dyn_cast<BinaryOperator>(ZE->getOperand(0));
  if (!Add || Add->getOpcode() != Instruction::Add || !Add->hasOneUse())
    return false;
  if (auto *CI = dyn_cast<ConstantInt>(Add->getOperand(1))) {
    X = Add->getOperand(0);
    C = CI;
  } else if (auto *CI = dyn_cast<ConstantInt>(Add->getOperand(0))) {
    X = Add->getOperand(1);
    C = CI;
  } else {
    return false;
  }
  if (isa<Constant>(X))
    return false;
  if (Add->hasNoUnsignedWrap())
    return true;
  return AssumeNoOffsetWrap && Add->getType()->isIntegerTy(32) && C->getValue().ult(MaxAssumedNoWrapOffset) &&
         isa<Argument, LoadInst, CallInst>(X);
}

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
  {
    Value *X = nullptr;
    ConstantInt *C = nullptr;
    if (Plan.FoldZExt && matchZExtOfAddConst(V, Plan.AssumeNoOffsetWrap, X, C)) {
      if (Depth > MaxDepth) {
        Plan.Bail = true;
        return;
      }
      Plan.FoundBuriedConst = true;
      Plan.ConstSum += C->getZExtValue();
      Plan.NonConstLeaves.push_back({X, /*NeedsZExt=*/true});
      return;
    }
  }
  auto *I = dyn_cast<Instruction>(V);
  if (!I || I->getOpcode() != Instruction::Add || (!IsTop && !I->hasOneUse())) {
    Plan.NonConstLeaves.push_back({V, /*NeedsZExt=*/false});
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
// (multi-use intermediates, a top user that is not a same-block stateful
// inttoptr, deep chains, no buried constant).
static CanonPlan analyze(IntToPtrInst *I2P, CodeGenContext *Ctx, StateIndexCanonMode Mode) {
  CanonPlan Plan;
  Plan.FoldZExt = Mode >= StateIndexCanonMode::FoldZExt;
  Plan.MergeBases = Mode >= StateIndexCanonMode::MergeBases;

  Value *Top = I2P->getOperand(0);
  // matchStateIndex requires the base to be 64-bit (PatternMatchPass.cpp).
  // Rewriting a narrower chain extends leaf live ranges for zero benefit
  // because matchStateIndex would reject the result on the width check.
  if (!Top->getType()->isIntegerTy(64))
    return Plan;
  if (!Plan.FoldZExt && !Top->hasOneUse())
    return Plan;
  // Every user of Top must be a stateful inttoptr in this block: commit()
  // rewrites them all. Any other user would keep the old chain alive and we'd
  // net out at "old chain + new chain" — strict regression.
  BasicBlock *I2PBlock = I2P->getParent();
  bool MayBeSampler = false;
  Plan.AssumeNoOffsetWrap = true;
  for (User *U : Top->users()) {
    auto *UI = dyn_cast<IntToPtrInst>(U);
    if (!UI || UI->getParent() != I2PBlock || !isStatefulAddrSpace(UI->getType()->getPointerAddressSpace()))
      return Plan;
    BufferType BufTy = DecodeBufferType(UI->getType()->getPointerAddressSpace());
    // Surfaces only; IsBindless excludes BINDLESS_SAMPLER.
    Plan.AssumeNoOffsetWrap &= IsBindless(BufTy) || IsSSHbindless(BufTy);
    MayBeSampler |= BufTy == BINDLESS_SAMPLER || BufTy == SAMPLER || BufTy == BUFFER_TYPE_UNKNOWN;
  }

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
  for (const Leaf &L : Plan.NonConstLeaves) {
    if (auto *LI = dyn_cast<Instruction>(L.V)) {
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
  //     and caps the sampler index at < 7. Only applies to pointers that may
  //     be samplers; known surfaces would otherwise lose valid folds.
  unsigned BSSize = Ctx->platform.GetBindlessSamplerSize();
  if ((MayBeSampler || !Plan.FoldZExt) && BSSize > 0 && Plan.ConstSum % BSSize == 0 && Plan.ConstSum / BSSize >= 7)
    return Plan;

  // Note: live-range/RP risk is throttled at the function-level instruction
  // count gate in runOnFunction, not per-leaf here.

  Plan.Viable = true;
  return Plan;
}

// Finds a user of V before InsertPt in the same block satisfying Pred, so
// chains can share one residual base. Skips instructions with poison-generating
// flags (nuw/nsw/nneg): they may be poison where the new value is not.
template <typename PredT> static Instruction *findDominatingUser(Value *V, Instruction *InsertPt, PredT Pred) {
  for (User *U : V->users()) {
    auto *UI = dyn_cast<Instruction>(U);
    if (UI && UI != InsertPt && UI->getParent() == InsertPt->getParent() && UI->comesBefore(InsertPt) &&
        !UI->hasPoisonGeneratingFlags() && Pred(UI))
      return UI;
  }
  return nullptr;
}

static Value *getOrCreateZExt(IRBuilder<> &B, Value *V, Type *Ty) {
  Instruction *InsertPt = &*B.GetInsertPoint();
  if (Instruction *Existing =
          findDominatingUser(V, InsertPt, [Ty](Instruction *I) { return isa<ZExtInst>(I) && I->getType() == Ty; }))
    return Existing;
  return B.CreateZExt(V, Ty);
}

static Value *getOrCreateAdd(IRBuilder<> &B, Value *L, Value *R) {
  Instruction *InsertPt = &*B.GetInsertPoint();
  if (Instruction *Existing = findDominatingUser(L, InsertPt, [L, R](Instruction *I) {
        auto *BO = dyn_cast<BinaryOperator>(I);
        return BO && BO->getOpcode() == Instruction::Add &&
               ((BO->getOperand(0) == L && BO->getOperand(1) == R) ||
                (BO->getOperand(0) == R && BO->getOperand(1) == L));
      }))
    return Existing;
  return B.CreateAdd(L, R);
}

// Builds the new chain at the old top's position and rewires every inttoptr
// user. Old chain becomes dead and is DCE'd.
static void commit(IntToPtrInst *I2P, const CanonPlan &Plan) {
  IGC_ASSERT(Plan.Viable);
  Value *Top = I2P->getOperand(0);
  Type *Ty = Top->getType();

  auto *TopI = cast<Instruction>(Top);
  IRBuilder<> B(TopI);
  B.SetCurrentDebugLocation(TopI->getDebugLoc());
  auto materialize = [&](const Leaf &L) -> Value * {
    if (!L.NeedsZExt)
      return L.V;
    return Plan.MergeBases ? getOrCreateZExt(B, L.V, Ty) : B.CreateZExt(L.V, Ty);
  };
  Value *Acc = materialize(Plan.NonConstLeaves[0]);
  for (size_t i = 1; i < Plan.NonConstLeaves.size(); ++i) {
    Value *R = materialize(Plan.NonConstLeaves[i]);
    Acc = Plan.MergeBases ? getOrCreateAdd(B, Acc, R) : B.CreateAdd(Acc, R);
  }
  Value *NewTop = B.CreateAdd(Acc, ConstantInt::get(Ty, Plan.ConstSum));

  Top->replaceAllUsesWith(NewTop);
  RecursivelyDeleteTriviallyDeadInstructions(Top);
}

// Base of a stateful address: X in inttoptr(add(X, C)), else the operand.
static Value *getAddrBase(IntToPtrInst *I2P) {
  Value *V = I2P->getOperand(0);
  if (auto *BO = dyn_cast<BinaryOperator>(V); BO && BO->getOpcode() == Instruction::Add) {
    if (isa<ConstantInt>(BO->getOperand(1)))
      return BO->getOperand(0);
    if (isa<ConstantInt>(BO->getOperand(0)))
      return BO->getOperand(1);
  }
  return V;
}

// `zext X` leaves compare by (X, type) so separate copies of one extension match.
using LeafKey = std::pair<Value *, Type *>;
using BaseKey = std::pair<LeafKey, LeafKey>;

static LeafKey getLeafKey(Value *V) {
  if (auto *ZE = dyn_cast<ZExtInst>(V); ZE && !ZE->hasPoisonGeneratingFlags())
    return {ZE->getOperand(0), ZE->getType()};
  return {V, nullptr};
}

// Key of a flag-free `add L, R`, independent of operand order.
static std::optional<BaseKey> getBaseKey(Value *V) {
  auto *BO = dyn_cast<BinaryOperator>(V);
  if (!BO || BO->getOpcode() != Instruction::Add || BO->hasPoisonGeneratingFlags())
    return {};
  LeafKey A = getLeafKey(BO->getOperand(0)), B = getLeafKey(BO->getOperand(1));
  auto Less = [](const LeafKey &X, const LeafKey &Y) {
    std::less<const void *> L;
    return X.first != Y.first ? L(X.first, Y.first) : L(X.second, Y.second);
  };
  if (Less(B, A))
    std::swap(A, B);
  return BaseKey{A, B};
}

// Merges equivalent bases of stateful addresses in BB, keeping the earliest.
// SinkPointerConstAdd and commit() can leave several copies of one base, and
// each copy takes its own s0 slot. Window bounds the distance to the previous
// use, so a far copy keeps its own short live range (0 = no limit).
static bool unifyBases(BasicBlock &BB, unsigned Window) {
  struct Rep {
    Instruction *I;
    unsigned LastPos;
  };
  DenseMap<BaseKey, Rep> Reps;
  bool Changed = false;
  unsigned Pos = 0;
  // Merged copies are always earlier than the current inttoptr, so
  // early-increment iteration stays valid.
  for (Instruction &I : make_early_inc_range(BB)) {
    ++Pos;
    auto *I2P = dyn_cast<IntToPtrInst>(&I);
    if (!I2P || !isStatefulAddrSpace(I2P->getType()->getPointerAddressSpace()))
      continue;
    auto *Base = dyn_cast<Instruction>(getAddrBase(I2P));
    if (!Base || Base->getParent() != &BB)
      continue;
    auto Key = getBaseKey(Base);
    if (!Key)
      continue;
    auto [It, Inserted] = Reps.try_emplace(*Key, Rep{Base, Pos});
    Rep &R = It->second;
    if (Inserted || R.I == Base || (Window && Pos - R.LastPos > Window)) {
      R = {Base, Pos};
      continue;
    }
    // The earlier instruction dominates every use of the later one.
    Instruction *Keep = R.I->comesBefore(Base) ? R.I : Base;
    Instruction *Drop = Keep == R.I ? Base : R.I;
    Drop->replaceAllUsesWith(Keep);
    RecursivelyDeleteTriviallyDeadInstructions(Drop);
    R = {Keep, Pos};
    Changed = true;
  }
  return Changed;
}

} // namespace

bool StateIndexAddrChainCanonicalize::runOnFunction(Function &F) {
  if (IGC_IS_FLAG_ENABLED(DisableStatefulFolding))
    return false;
  auto Mode = static_cast<StateIndexCanonMode>(IGC_GET_FLAG_VALUE(EnableStateIndexAddrChainCanonicalize));
  if (Mode == StateIndexCanonMode::Off)
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
      CanonPlan Plan = analyze(I2P, Ctx, Mode);
      if (!Plan.Viable)
        continue;
      commit(I2P, Plan);
      Changed = true;
    }
  }
  if (Mode < StateIndexCanonMode::MergeBases)
    return Changed;
  unsigned Window = IGC_GET_FLAG_VALUE(StateIndexUnifyBasesWindow);
  for (BasicBlock &BB : F)
    Changed |= unifyBases(BB, Window);
  return Changed;
}

FunctionPass *createStateIndexAddrChainCanonicalizePass() { return new StateIndexAddrChainCanonicalize(); }
