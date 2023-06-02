/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*
 * Copyright (C) 2022 Yuly Tarasov <ulius.cesarus@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/// GenXPredToSimdCF
/// ----------------
///
/// In this file a new Simd CF IR interface introduced. Need for this interface
/// is caused by the fact that vector backend has several frontends, for
/// example, ISPC, CM, DPC++ ESIMD, and this interface can allow make unified
/// Simd CF constructions in IR and, moreover, solve some problems of existing
/// Simd CF IR model. Main idea of this interface is to predicate all Simd CF
/// code. This will be discussed in more detail below.
///
/// GenXPredToSimdCF is a function pass that transforms such predicated code
/// into GenX Simd CF.
///
/// Since in GenX ISA specification we can see that Simd CF starts with `goto`
/// instruction and ends with `join` instruction, Simd CF code has one entry and
/// one exit basic block. So let's call block (not basic!) of Simd CF code
/// 'SimdCFRegion' since it has one entry and one exit basic block.
///
/// All SimdCFRegions can be divided into two groups: SimdCFIfRegions and
/// SimdCFLoopRegions. First one corresponds to SIMD CF if-else and second one
/// to SIMD CF do-while.
///
/// Control flow scheme for SIMD CF if-else is following:
///
///                          +-------------------+
///                          |       entry       |
///                          +-------------------+
///                             /             \
///                            /               |<[if any cond is true]
///                           /                V
///                          /             +----------------+
/// [if all cond are false]>/              |   region for   |
///                        /               |   simd cf if   |
///                  +----+                |      cond      |
///                 /                      +----------------+
///                /                          /      /
///                |  [if any cond is false]>/      /
///                V                        /      /
///         +----------------+             /      /
///         |   region for   |            /      /
///         |  simd cf else  |<----------+      /<[if all cond are true]
///         |      cond      |                 /
///         +----------------+                /
///                            \             /
///                             |            |
///                             V            V
///                          +-------------------+
///                          |        end        |
///                          +-------------------+
///
/// Control flow scheme for SIMD CF do-while is following:
///
///                          +-------------------+
///                          |       entry       |
///                          +-------------------+
///                                 |
///                                 |     +----------+
///                                 V     V          |
///                          +-------------------+   |
///                          |     loop body     |   |<[if any cond is true]
///                          +-------------------+   |
///                                 |     |          |
///         [if all cond are false]>|     +----------+
///                                 V
///                          +-------------------+
///                          |        end        |
///                          +-------------------+
///
/// In proposed interface Simd CF goto is replaced with a pair of instructions.
/// First instruction is and/or reduce for Simd condition. Second instruction
/// is scalar branch result of reduce as branch condition. If just replace gotos
/// IR will not be in consistent state and futher passes can break such IR. So
/// to escape this all side effects (mostly it is stores) must be masked. And
/// when all side effects in Simd CF region are predicated with Simd condition
/// for this region we can call this code as 'predicated code'.
///
/// Algorithm for matching for SimdCFRegions:
/// 1. Find condition branch that looks like Simd CF;
/// 2. Check if region CF matches CF for Simd CF if-else or Simd CF do-while;
/// 3. Verify masks;
/// 4. For if-else:
///      check if masks for if and else regions are opposite;
///    For do-while:
///      check PHIs for induction variables;
///      check mask recalculation.
/// After that recurcively find nesting SimdCFRegions.
///
/// Transformation for SimdCFIfRegion requires next steps:
/// 1. Replace and/or reductions with genx.goto/join;
/// 2. Remove side effect masking.
///
//===----------------------------------------------------------------------===//

// TODO: may be it is better to divide GenXPredToSimdCF into two pass: analysis
// and transformation
//
// TODO: now only matching for top-level region works good (for simple tests at
// least). Need to add matching nested and add transformation.
//
// NOTE: transformation is much harded in details than it is decribed above.
// Main problem is to make this pass and SimdCFConformace be compatible.

#include "vc/Support/BackendConfig.h"
#include "vc/Utils/General/Types.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/RegionInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/InitializePasses.h>
#include <llvm/Support/CommandLine.h>

#define DEBUG_TYPE "simdcf-region"

using namespace llvm;

static cl::opt<bool>
    EnableSimdCFTransform("enable-simdcf-transform", cl::init(false),
                          cl::Hidden,
                          cl::desc("Enable simd cf transformation."));
namespace llvm::genx {
// Need declarations of this functions here because of declaration conflict
// beetween genx::Region from GenXUtils.h and llvm::Region
bool isPredicate(Instruction *Inst);
bool isPredNot(Instruction *Inst);

class SimdCFRegion : public Region {
protected:
  std::set<SimdCFRegion *> SubRegions; // set because subregions are unique
  Value *Mask;

public:
  SimdCFRegion(BasicBlock *Entry, BasicBlock *Exit, RegionInfo *RI,
               DominatorTree *DT, SimdCFRegion *Parent = nullptr)
      : Region(Entry, Exit, RI, DT, Parent), Mask(nullptr) {}
  virtual ~SimdCFRegion() {}

  virtual bool isIfRegion() const = 0;
  virtual bool isLoopRegion() const = 0;

  virtual bool verify() const = 0;

  auto *getMask() const { return Mask; }
};

class SimdCFIfRegion final : public SimdCFRegion {
  Region *IfThenRegion = nullptr;
  Region *IfElseRegion = nullptr;
  BranchInst *SimdIfBranch = nullptr;

public:
  bool isIfRegion() const override { return true; }
  bool isLoopRegion() const override { return false; }

  bool hasElse() const { return IfElseRegion != nullptr; }

  Region *getIfThenRegion() const { return IfThenRegion; }
  Region *getIfElseRegion() const { return IfElseRegion; }

  auto *getIfSimdBranch() const { return SimdIfBranch; }

  auto getIdThen() const {
    IGC_ASSERT(getIfSimdBranch()->isConditional());
    IGC_ASSERT(getIfSimdBranch()->getNumSuccessors() == 2);
    if (getIfSimdBranch()->getSuccessor(0) == IfThenRegion->getEntry())
      return 0;
    IGC_ASSERT(getIfSimdBranch()->getSuccessor(1) == IfThenRegion->getEntry());
    return 1;
  }

  // Conformance-pass expect true edge to join-block - that's why here swap
  // successors
  bool needSwap() { return getIdThen() == 0; }

  auto *getIfSimdCondition() const {
    return dyn_cast<CallInst>(getIfSimdBranch()->getCondition());
  }

  // TODO: need to add algo to verify mask for simdcf region:
  // 1. For each masked op: op mask in submask of region mask;
  // 2. If there is else block: else mask is opposite to if mask in terms of
  // parent mask;
  // 3. For each subregion: subregion mask is submask of region mask.
  //
  // SindCFIfRegion is considered verified if each of conditions above are true.
  bool verify() const override { return true; }

  SimdCFIfRegion(BasicBlock *Entry, Region *IfThen, Region *IfElse,
                 BasicBlock *Exit, RegionInfo *RI, DominatorTree *DT,
                 SimdCFRegion *Parent = nullptr)
      : SimdCFRegion(Entry, Exit, RI, DT, Parent), IfThenRegion(IfThen),
        IfElseRegion(IfElse) {
    IGC_ASSERT(IfThenRegion);
    SimdIfBranch = cast<BranchInst>(Entry->getTerminator());
    IGC_ASSERT(SimdIfBranch);
    Mask = getIfSimdCondition()->getArgOperand(0);
  }
};

class SimdCFLoopRegion final : public SimdCFRegion {
  Loop *SimdLoop;
  BranchInst *SimdLoopBranch = nullptr;
  BasicBlock *LoopHead = nullptr;

public:
  bool isIfRegion() const override { return false; }
  bool isLoopRegion() const override { return true; }

  BranchInst *getSimdBranch() const { return SimdLoopBranch; }

  auto *getSimdCondition() const {
    return dyn_cast<CallInst>(getSimdBranch()->getCondition());
  }

  auto *getLoopHead() const { return LoopHead; }

  auto getIdExit() const {
    IGC_ASSERT(getSimdBranch()->isConditional());
    IGC_ASSERT(getSimdBranch()->getNumSuccessors() == 2);
    if (getSimdBranch()->getSuccessor(0) == getExit())
      return 0;
    IGC_ASSERT(getSimdBranch()->getSuccessor(1) == getExit());
    return 1;
  }

  // TODO: need to add algo to verify mask for simdcf region:
  // 1. For each masked op: op mask in submask of region mask;
  // 2. For each induction variable: var is masked with region mask;
  // 3. For each subregion: subregion mask is submask of region mask.
  //
  // SindCFIfRegion is considered verified if each of conditions above are true.
  bool verify() const override { return true; }

  // Conformance-pass expect true edge to join-block - that's why here swap
  // successors
  bool needSwap() { return getIdExit() != 0; }

  SimdCFLoopRegion(BasicBlock *Entry, Loop *L, BasicBlock *Exit, RegionInfo *RI,
                   DominatorTree *DT, SimdCFRegion *Parent = nullptr)
      : SimdCFRegion(Entry, Exit, RI, DT, Parent), SimdLoop(L) {
    SimdLoopBranch =
        cast<BranchInst>(SimdLoop->getExitingBlock()->getTerminator());
    IGC_ASSERT(SimdLoopBranch);
    Mask = getSimdCondition()->getArgOperand(0);
    LoopHead = SimdLoopBranch->getSuccessor(getIdExit());
  }
};
} // namespace llvm::genx

using namespace genx;

namespace {
class GenXPredToSimdCF final : public FunctionPass {
  // Created (existed) execution-mask addresses
  std::map<Function *, AllocaInst *> EMAddrs;
  // Created (existed) resume-mask addresses
  std::map<BasicBlock *, AllocaInst *> RMAddrs;
  std::map<SimdCFRegion *, BasicBlock *> JoinBlocks;

  BranchInst *Br = nullptr; // TODO - remove it
  CallInst *OldCond = nullptr;
  Value *Mask = nullptr;
  BasicBlock *JP = nullptr;
  BasicBlock *JPSplit = nullptr;
  BasicBlock *OldCondBB = nullptr;
  bool NeedSwap = false;

  PostDominatorTree *PDT = nullptr;

public:
  using SimdCFRegionPtr = std::unique_ptr<SimdCFRegion>;
  using SimdCFRegionsT = std::vector<SimdCFRegionPtr>;

  static constexpr unsigned MAX_SIMD_CF_WIDTH = 32;

  static char ID;

  explicit GenXPredToSimdCF() : FunctionPass(ID) {}
  StringRef getPassName() const override {
    return "GenX transforming predicated code into Simd CF";
  }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<RegionInfoPass>();
    AU.addRequired<GenXBackendConfig>();
  }

private:
  SimdCFRegionsT findSimdCFRegions(Function &F);

  void insertIfGotoJoin(SimdCFIfRegion &R);
  void removeMask(SimdCFIfRegion &R);
  void fixPHIs(SimdCFIfRegion &R);

  void insertLoopGotoJoin(SimdCFLoopRegion &R);
  void fixPHIs(SimdCFLoopRegion &R);

  bool transform(SimdCFRegion &R);
  bool transform(SimdCFIfRegion &R);
  bool transform(SimdCFLoopRegion &R);

  void generateGoto(BranchInst *Br);
  Value *generateJoin(BasicBlock *JoinBlock);

  Value *getEM(Function *F) {
    // TODO: replace to Module or function-group for support stack calls
    IGC_ASSERT(F);
    auto EM = EMAddrs.find(F);
    if (EM == EMAddrs.end()) {
      auto *M = F->getParent();
      auto *EMTy = IGCLLVM::FixedVectorType::get(
          Type::getInt1Ty(M->getContext()), MAX_SIMD_CF_WIDTH);
      auto *InsertBefore = &JP->getParent()->front().front();
      auto *EMAddr = new AllocaInst(EMTy, 0 /*AddrSpace*/,
                                    Twine("EM.") + JP->getName(), InsertBefore);
      new StoreInst(Constant::getAllOnesValue(EMTy), EMAddr,
                    false /* isVolatile */, InsertBefore);
      EMAddrs[F] = EMAddr;
    }
    return EMAddrs[F];
  }

  Value *getRMAddr(BasicBlock *JP, unsigned Width = 0) {
    auto RMAddr = RMAddrs.find(JP);
    if (RMAddr == RMAddrs.end()) {
      auto *RMTy = IGCLLVM::FixedVectorType::get(
          Type::getInt1Ty(JP->getContext()), Width);
      auto *InsertBefore = &JP->getParent()->front().front();
      auto *RMAddr = new AllocaInst(RMTy, 0 /*AddrSpace*/,
                                    Twine("RM.") + JP->getName(), InsertBefore);
      new StoreInst(Constant::getNullValue(RMTy), RMAddr,
                    false /* isVolatile */, InsertBefore);
      RMAddrs[JP] = RMAddr;
    }
    return RMAddrs[JP];
  }

  bool isSimdCFCondition(Value *Cond) {
    switch (GenXIntrinsic::getGenXIntrinsicID(Cond)) {
    case GenXIntrinsic::genx_all:
    case GenXIntrinsic::genx_any:
      return true;
    default:
      return false;
    }
  }

  std::pair<BranchInst *, Value *>
  findSimdCFBranchAndCondition(BasicBlock &BB) {
    LLVM_DEBUG(dbgs() << "Trying to find Simd branch and condition in:\n");
    LLVM_DEBUG(BB.getName());

    auto *Terminator = BB.getTerminator();
    if (!Terminator) {
      LLVM_DEBUG(dbgs() << "Didn't find terminator for BB '" << BB.getName()
                        << "'\n");
      return std::make_pair(nullptr, nullptr);
    }

    if (!isa<BranchInst>(Terminator)) {
      LLVM_DEBUG(dbgs() << "Terminator isn't a branch for BB '" << BB.getName()
                        << "'\n");
      return std::make_pair(nullptr, nullptr);
    }

    auto *BrInst = cast<BranchInst>(Terminator);
    if (!BrInst) {
      LLVM_DEBUG(dbgs() << "Terminator isn't a branch for BB '" << BB.getName()
                        << "'\n");
      return std::make_pair(nullptr, nullptr);
    }

    if (BrInst->isUnconditional()) {
      LLVM_DEBUG(dbgs() << "Branch is unconditional for BB '" << BB.getName()
                        << "'\n");
      return std::make_pair(nullptr, nullptr);
    }

    auto *Cond = BrInst->getCondition();
    if (isSimdCFCondition(Cond))
      return std::make_pair(BrInst, Cond);

    LLVM_DEBUG(dbgs() << "Condition isn't a Simd CF condition:\n");
    LLVM_DEBUG(Cond->dump());
    return std::make_pair(nullptr, nullptr);
  }

  bool isReduceAnd(const Value *V) {
    if (V)
      if (const CallInst *CI = dyn_cast<CallInst>(V))
        return GenXIntrinsic::getGenXIntrinsicID(V) ==
                   GenXIntrinsic::genx_all ||
               CI->getIntrinsicID() ==
#if LLVM_VERSION_MAJOR < 12
                   Intrinsic::experimental_vector_reduce_and;
#else
                   Intrinsic::vector_reduce_and;
#endif
    return false;
  }

  bool isReduceOr(const Value *V) {
    if (V)
      if (const CallInst *CI = dyn_cast<CallInst>(V))
        return GenXIntrinsic::getGenXIntrinsicID(V) ==
                   GenXIntrinsic::genx_any ||
               CI->getIntrinsicID() ==
#if LLVM_VERSION_MAJOR < 12
                   Intrinsic::experimental_vector_reduce_or;
#else
                   Intrinsic::vector_reduce_or;
#endif
    return false;
  }

  bool areOppositeSimdCFConditions(Value *IfCond, Value *ElseCond) {
    if (!IfCond || !ElseCond)
      return false;

    bool isIfCondReduceOr = isReduceOr(IfCond);
    if (!isIfCondReduceOr && !isReduceAnd(IfCond))
      return false;

    bool isElseCondReduceOr = isReduceOr(ElseCond);
    if (!isElseCondReduceOr && !isReduceAnd(ElseCond))
      return false;

    auto *IfCondCall = cast<CallInst>(IfCond);
    auto *ElseCondCall = cast<CallInst>(ElseCond);
    if (!IfCondCall || !ElseCondCall)
      return false;
    // TODO: Support const (not casted to Instruction)
    auto *IfMask = dyn_cast<Instruction>(IfCondCall->getArgOperand(0));
    auto *ElseMask = dyn_cast<Instruction>(ElseCondCall->getArgOperand(0));
    if (!IfMask || !ElseMask)
      return false;
    LLVM_DEBUG(dbgs() << "IfMask: "; IfMask->dump());
    LLVM_DEBUG(dbgs() << "ElseMask: "; ElseMask->dump());

    if (isIfCondReduceOr && isElseCondReduceOr) {
      if (!genx::isPredicate(IfMask) || !genx::isPredNot(ElseMask))
        return false;
      LLVM_DEBUG(dbgs() << "Comparing "; IfMask->dump());
      LLVM_DEBUG(dbgs() << "against   "; ElseMask->getOperand(0)->dump());
      return IfMask == ElseMask->getOperand(0);
    } else if (isIfCondReduceOr && !isElseCondReduceOr) {
      return false; // not implemented
    } else if (!isIfCondReduceOr && isElseCondReduceOr) {
      return false; // not implemented
    } else if (!isIfCondReduceOr && !isElseCondReduceOr) {
      if (!genx::isPredicate(IfMask) || !genx::isPredNot(ElseMask))
        return false;
      LLVM_DEBUG(dbgs() << "Comparing "; IfMask->dump());
      LLVM_DEBUG(dbgs() << "against   "; ElseMask->getOperand(0)->dump());
      return IfMask == ElseMask->getOperand(0);
    }

    return false;
  }

  BasicBlock *getIfExitFromIfBranch(BranchInst &BrInst) {
    if (!BrInst.isConditional())
      return nullptr;
    enum { N_IF_EXIT_PREDECESSORS = 2 };
    auto *FirstSucc = BrInst.getSuccessor(0);
    if (FirstSucc->hasNPredecessors(N_IF_EXIT_PREDECESSORS))
      return FirstSucc;
    auto *SecondSucc = BrInst.getSuccessor(1);
    if (SecondSucc->hasNPredecessors(N_IF_EXIT_PREDECESSORS))
      return SecondSucc;
    return nullptr;
  }

  BasicBlock *getIfExitFromElseBranch(BranchInst &BrInst) {
    if (!BrInst.isConditional())
      return nullptr;
    auto *IfElseEntry = getIfElseEntry(BrInst);
    auto IfExit =
        std::find_if(BrInst.successors().begin(), BrInst.successors().end(),
                     [IfElseEntry, this](BasicBlock *BB) -> bool {
                       return BB != IfElseEntry;
                     });
    return IfExit != BrInst.successors().end() ? *IfExit : nullptr;
  }

  BasicBlock *getIfThenEntry(BranchInst &BrInst) {
    if (!BrInst.isConditional())
      return nullptr;
    enum { N_IF_THEN_ENTRY_PREDECESSORS = 1 };
    auto *FirstSucc = BrInst.getSuccessor(0);
    if (FirstSucc->hasNPredecessors(N_IF_THEN_ENTRY_PREDECESSORS))
      return FirstSucc;
    auto *SecondSucc = BrInst.getSuccessor(1);
    if (SecondSucc->hasNPredecessors(N_IF_THEN_ENTRY_PREDECESSORS))
      return SecondSucc;
    return nullptr;
  }

  BasicBlock *getIfThenEnd(BranchInst &BrInst) {
    if (!BrInst.isConditional())
      return nullptr;
    auto *IfExit = getIfExitFromIfBranch(BrInst);
    if (!IfExit)
      return nullptr;
    auto IfThenEnd = std::find_if(
        pred_begin(IfExit), pred_end(IfExit),
        [&BrInst](BasicBlock *BB) -> bool { return BB != BrInst.getParent(); });
    return IfThenEnd != pred_end(IfExit) ? *IfThenEnd : nullptr;
  }

  BasicBlock *getIfElseEntry(BranchInst &BrInst) {
    if (!BrInst.isConditional())
      return nullptr;
    enum { N_IF_ELSE_ENTRY_PREDECESSORS = 2 };
    auto *FirstSucc = BrInst.getSuccessor(0);
    if (FirstSucc->hasNPredecessors(N_IF_ELSE_ENTRY_PREDECESSORS))
      return FirstSucc;
    auto *SecondSucc = BrInst.getSuccessor(1);
    if (SecondSucc->hasNPredecessors(N_IF_ELSE_ENTRY_PREDECESSORS))
      return SecondSucc;
    return nullptr;
  }

  BasicBlock *getIfElseEnd(BranchInst &BrInst) {
    if (!BrInst.isConditional())
      return nullptr;
    auto IfExitPos =
        std::find_if(BrInst.successors().begin(), BrInst.successors().end(),
                     [&BrInst, this](BasicBlock *BB) -> bool {
                       return BB != getIfElseEntry(BrInst);
                     });
    if (IfExitPos == BrInst.successors().end())
      return nullptr;
    auto *IfExit = *IfExitPos;
    auto IfElseEnd = std::find_if(
        pred_begin(IfExit), pred_end(IfExit),
        [&BrInst](BasicBlock *BB) -> bool { return BB != BrInst.getParent(); });
    return IfElseEnd != pred_end(IfExit) ? *IfElseEnd : nullptr;
  }

  // TODO: simd-conformance pass expect select, wrregion, shufflevector
  // and gather/scatter need support more complex examples
  template <class BBContainer>
  bool analizeInsts(BBContainer *Container, Value *Cond) {
    // genx_any/genx_all (pred)
    if (!isSimdCFCondition(Cond))
      return false;
    bool RetVal = false;
    auto *Pred = (cast<Instruction>(Cond))->getOperand(0);
    if (auto *Cast = dyn_cast<CastInst>(Pred))
      Pred = dyn_cast<Instruction>(Cast->getOperand(0));

    LLVM_DEBUG(dbgs() << " ----- '    Node loop "
                      << (cast<Instruction>(Cond))->getFunction()->getName()
                      << "    ' ----- \n");

    llvm::SmallPtrSet<BasicBlock *, 8> BBs;
    // Helper function to propagate `matched`-insts through phi-nodes
    // and fill affected BB's
    using MatcherType = std::function<bool(User *, Value *)>;
    std::function<void(User *, Value *, MatcherType)> TryToFindBlock;
    TryToFindBlock = [&](User *Usr, Value* PredInst, MatcherType Matcher) {
      if (Matcher(Usr, PredInst)) {
        auto *Inst = dyn_cast<Instruction>(Usr);
        LLVM_DEBUG(dbgs() << Inst->getFunction()->getName() << " - "
                          << Inst->getName() << "\n");
        BBs.insert(Inst->getParent());
      }
      if (auto *Inst = dyn_cast<PHINode>(Usr))
        for (auto *PhiUsr : Inst->users()) {
          TryToFindBlock(PhiUsr, Inst, Matcher);
        }
    };

    // 1-st case - check successors for Cond:
    //  if there is already existing simd-cf - do not apply
    for (auto *Usr : Cond->users()) {
      TryToFindBlock(Usr, Pred, [](User *Usr, Value *) {
        if (auto *Call = dyn_cast<CallInst>(Usr))
          switch (GenXIntrinsic::getGenXIntrinsicID(Call)) {
          case GenXIntrinsic::genx_simdcf_goto:
          case GenXIntrinsic::genx_simdcf_join:
            LLVM_DEBUG(dbgs() << "Find existing SimdCF " << Call->getName(););
            return true;
          default:
            return false;
          }
        return false;
      });
    }
    // If instructions was found - that means simd cf already exist
    // TODO: Check, that blocks intersect
    if (BBs.size())
      return false;

    // 2-st case - if predicate is constant:
    //    lookup all instructions, is there same constant?
    if (auto *Const = dyn_cast<Constant>(Pred)) {
      for (auto *BB : Container->blocks()) {
        for (auto &Inst : *BB) {
          if (isa<SelectInst>(Inst) && Inst.getOperand(0) == Const) {
            LLVM_DEBUG(dbgs() << "Find const-pred inst " << Inst.getName(););
            RetVal = true;
          }
        }
      }
      if (RetVal)
        return true; // ---> Success exit
      // If not found any `select`-inst than can't apply
      return false;
    }
    LLVM_DEBUG(dbgs() << " ----- '    BB loop "
                      << (cast<Instruction>(Cond))->getFunction()->getName()
                      << "    ' ----- \n");

    // 3-d  case - lookup for selects in predicate-users
    BBs.clear();

    for (auto *Usr : Pred->users()) {
      TryToFindBlock(Usr, Pred, [&](User *Usr, Value *PredI) {
        if (auto *Inst = dyn_cast<SelectInst>(Usr)) {
          if (Inst->getCondition() == PredI)
            return true;
          if (auto *Cast = dyn_cast<CastInst>(Inst->getCondition()))
            return Cast->getOperand(0) == PredI;
        }
        return false;
      });
    }
    // Not found any selects
    if (!BBs.size())
      return false;

    for (auto *BB : Container->blocks()) {
      if (BBs.count(BB))
        return true; // ---> Success exit
    }
    return false;
  }

  SimdCFRegionPtr tryMatchIf(BasicBlock &BB) {

    LLVM_DEBUG(dbgs() << "Trying match Simd CF If on BB '" << BB.getName()
                      << "'\n");
    auto [Branch, Cond] = findSimdCFBranchAndCondition(BB);
    if (!Branch || !Cond)
      return nullptr;

    LLVM_DEBUG(dbgs() << "Find Simd CF If branch and condition:\n";
               Cond->dump(); Branch->dump());
    BasicBlock *Entry = &BB;
    BasicBlock *IfThenEntry = getIfThenEntry(*Branch);
    if (!IfThenEntry)
      return nullptr;
    LLVM_DEBUG(dbgs() << "Find Simd CF IfThenEntry BB: "
                      << IfThenEntry->getName() << '\n');
    BasicBlock *IfThenEnd = getIfThenEnd(*Branch);
    if (!IfThenEnd)
      return nullptr;
    // Find Simd CF IfThenEnd BB: if.then
    LLVM_DEBUG(dbgs() << "Find Simd CF IfThenEnd BB: " << IfThenEnd->getName()
                      << '\n');

    auto IfThenRegion = std::make_unique<Region>(
        IfThenEntry, IfThenEnd, &getAnalysis<RegionInfoPass>().getRegionInfo(),
        &getAnalysis<DominatorTreeWrapperPass>().getDomTree());
    IfThenRegion->verifyRegion();
    // Finded IfThenRegion 'if.then => if.then': [0] if.then => if.then
    LLVM_DEBUG(dbgs() << "Finded IfThenRegion '" << IfThenRegion->getNameStr()
                      << "':\n";
               IfThenRegion->dump(););

    auto [ElseBranch, ElseCond] = findSimdCFBranchAndCondition(*IfThenEnd);

    if (ElseBranch && ElseCond) {
      bool areOppositeConds = areOppositeSimdCFConditions(Cond, ElseCond);
      if (!areOppositeConds) {
        LLVM_DEBUG(dbgs() << "Finded Simd CF Else condition is not opposite to "
                             "the If condition:\n";
                   ElseCond->dump(); ElseBranch->dump());
        return nullptr;
      }
      LLVM_DEBUG(dbgs() << "Find Simd CF Else branch and condition:\n";
                 ElseCond->dump(); ElseBranch->dump());
      BasicBlock *IfElseEntry = getIfElseEntry(*ElseBranch);
      if (!IfElseEntry)
        return nullptr;
      LLVM_DEBUG(dbgs() << "Find Simd CF IfElseEntry BB: "
                        << IfElseEntry->getName() << '\n');
      BasicBlock *IfElseEnd = getIfElseEnd(*ElseBranch);
      if (!IfElseEnd)
        return nullptr;
      LLVM_DEBUG(dbgs() << "Find Simd CF IfElseEnd BB: " << IfElseEnd->getName()
                        << '\n');

      auto IfElseRegion = std::make_unique<Region>(
          IfElseEntry, IfElseEnd,
          &getAnalysis<RegionInfoPass>().getRegionInfo(),
          &getAnalysis<DominatorTreeWrapperPass>().getDomTree());
      IfElseRegion->verifyRegion();
      LLVM_DEBUG(dbgs() << "Finded IfElseRegion '" << IfElseRegion->getNameStr()
                        << "':\n";
                 IfElseRegion->dump(););

      BasicBlock *Exit = getIfExitFromElseBranch(*ElseBranch);
      if (!Exit)
        return nullptr;

      // IfThen* post dominate Exit && Entry post dominate Exit
      if (!PDT || !PDT->dominates(Exit, IfThenEnd) ||
          !PDT->dominates(Exit, Entry) || !PDT->dominates(Exit, IfElseEnd))
        return nullptr;

      if (!(analizeInsts(IfThenRegion.get(), Cond) ||
            analizeInsts(IfElseRegion.get(), Cond))) {
        LLVM_DEBUG(dbgs() << "Filed to find predicated instructions for";
                   Cond->dump());
        return nullptr;
      }

      LLVM_DEBUG(dbgs() << "Find Simd CF IfEnd BB: " << Exit->getName()
                        << '\n');
      return std::make_unique<SimdCFIfRegion>(
          Entry, IfThenRegion.release(), IfElseRegion.release(), Exit,
          &getAnalysis<RegionInfoPass>().getRegionInfo(),
          &getAnalysis<DominatorTreeWrapperPass>().getDomTree());
    }

    BasicBlock *Exit = getIfExitFromIfBranch(*Branch);
    if (!Exit)
      return nullptr;

    // IfThenEnd post dominate exit && Entry post dominate exit
    if (!PDT || !PDT->dominates(Exit, Entry) ||
        !PDT->dominates(Exit, IfThenEnd))
      return nullptr;

    // Check there is any predicated(select) instructions
    // in region found.
    if (!analizeInsts(IfThenRegion.get(), Cond)) {
      LLVM_DEBUG(dbgs() << "Filed to find predicated instructions for";
                 Cond->dump());
      return nullptr;
    }

    LLVM_DEBUG(dbgs() << "Find Simd CF IfEnd BB: " << Exit->getName() << '\n');
    return std::make_unique<SimdCFIfRegion>(
        Entry, IfThenRegion.release(), nullptr, Exit,
        &getAnalysis<RegionInfoPass>().getRegionInfo(),
        &getAnalysis<DominatorTreeWrapperPass>().getDomTree());
  }

  std::pair<BranchInst *, Value *>
  findSimdCFLoopBranchAndCondition(const Loop &L) {
    LLVM_DEBUG(dbgs() << "Trying to find Simd CF branch and condition in:\n");
    LLVM_DEBUG(L.getName());
    auto *ExitingBB = L.getExitingBlock();
    if (!ExitingBB) {
      LLVM_DEBUG(dbgs() << "Didn't find exiting BB for this loop!\n");
      return std::make_pair(nullptr, nullptr);
    }
    LLVM_DEBUG(dbgs() << "Loop exiting BB: '" << ExitingBB->getName() << "'\n");
    return findSimdCFBranchAndCondition(*ExitingBB);
  }

  SimdCFRegionPtr tryMatchLoop(BasicBlock &BB) {
    LLVM_DEBUG(dbgs() << "Trying match Simd CF Loop on BB '" << BB.getName()
                      << "'\n");

    auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    auto *Loop = LI.getLoopFor(&BB);
    if (!Loop) {
      LLVM_DEBUG(dbgs() << "Didn't find Loop on BB '" << BB.getName() << "'\n");
      return nullptr;
    }
    auto [Branch, Cond] = findSimdCFLoopBranchAndCondition(*Loop);
    if (!Branch || !Cond) {
      LLVM_DEBUG(dbgs() << "Didn't find Simd CF Loop branch and condition\n");
      return nullptr;
    }
    LLVM_DEBUG(dbgs() << "Find Simd CF Loop branch and condition:\n";
               Cond->dump(); Branch->dump());
    auto *Entry = Loop->getLoopPredecessor();
    if (!Entry)
      return nullptr;
    LLVM_DEBUG(dbgs() << "Find Simd CF Loop Entry BB: " << Entry->getName()
                      << '\n');
    auto *Exit = Loop->getExitBlock();
    if (!Exit)
      return nullptr;

    if (!analizeInsts(Loop, Cond))
      return nullptr;

    LLVM_DEBUG(dbgs() << "Find Simd CF Loop Exit BB: " << Exit->getName()
                      << '\n');

    return std::make_unique<SimdCFLoopRegion>(
        Entry, Loop, Exit, &getAnalysis<RegionInfoPass>().getRegionInfo(),
        &getAnalysis<DominatorTreeWrapperPass>().getDomTree());
  }
};

// TODO: rewrite this algo to use Region as arg instead of Fuction to allow
// reuse it for matching subregions
GenXPredToSimdCF::SimdCFRegionsT
GenXPredToSimdCF::findSimdCFRegions(Function &F) {
  SimdCFRegionsT Regions;
  std::set<BasicBlock *> Visited; // set because we need unique values
  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  for (auto &BB : F.getBasicBlockList()) {
    if (Visited.count(&BB) != 0) {
      LLVM_DEBUG(dbgs() << "Skipping already visited BB " << BB.getName()
                        << '\n');
      continue;
    }

    auto *Loop = LI.getLoopFor(&BB);
    if (!Loop) {
      if (SimdCFRegionPtr Match = tryMatchIf(BB)) {
        LLVM_DEBUG(dbgs() << "Matched Simd CF If on BB '" << BB.getName()
                          << "'\n");
        LLVM_DEBUG(Match->dump());
        for (auto *MBB : Match->blocks()) {
          Visited.insert(MBB);
          LLVM_DEBUG(dbgs()
                     << "Marking BB '" << MBB->getName() << "' as visited\n");
        }
        Regions.push_back(std::move(Match));
      } else {
        LLVM_DEBUG(dbgs() << "Couldn't match Simd CF on BB '" << BB.getName()
                          << "'\n");
      }
    } else {
      if (SimdCFRegionPtr Match = tryMatchLoop(BB)) {
        LLVM_DEBUG(dbgs() << "Matched Simd CF Loop on BB '" << BB.getName()
                          << "'\n");
        LLVM_DEBUG(Match->dump());
        for (auto *MBB : Match->blocks()) {
          Visited.insert(MBB);
          LLVM_DEBUG(dbgs()
                     << "Marking BB '" << MBB->getName() << "' as visited\n");
        }
        Regions.push_back(std::move(Match));
      } else {
        LLVM_DEBUG(dbgs() << "Couldn't match Simd CF on BB '" << BB.getName()
                          << "'\n");
      }
    }

    Visited.insert(&BB);
    LLVM_DEBUG(dbgs() << "Marking BB '" << BB.getName() << "' as visited\n");
  }

  return Regions;
}

Value *GenXPredToSimdCF::generateJoin(BasicBlock *JoinBlock) {
  auto *F = JoinBlock->getParent();
  Value *EM = getEM(F);

  auto SimdTy = cast<IGCLLVM::FixedVectorType>(Mask->getType());
  auto SimdWidth = SimdTy->getNumElements();
  IRBuilder<> Builder(JoinBlock, JoinBlock->begin());

  // Fix execution mask in after-then branch
  auto *OldEM = Builder.CreateLoad(IGCLLVM::getNonOpaquePtrEltTy(EM->getType()),
                                   EM, false /*isVolatile*/, EM->getName());

  auto *RMAddr = getRMAddr(JP, SimdWidth);
  auto *RM =
      Builder.CreateLoad(IGCLLVM::getNonOpaquePtrEltTy(RMAddr->getType()),
                         RMAddr, false /*isVolatile*/, RMAddr->getName());

  Type *Tys[] = {OldEM->getType(), RM->getType()};
  auto *M = JoinBlock->getModule();
  auto *JoinDecl = GenXIntrinsic::getGenXDeclaration(
      M, GenXIntrinsic::genx_simdcf_join, Tys);
  Value *Args[] = {OldEM, RM};

  CallInst *Join = Builder.CreateCall(JoinDecl, Args, "join");
  Join->setConvergent();
  auto *NewEM = Builder.CreateExtractValue(Join, 0, "join.extractem");
  Builder.CreateStore(NewEM, EM, false /*isVolatile*/);
  auto *JoinCond = Builder.CreateExtractValue(Join, 1, "join.extractcond");
  return JoinCond;
}

// Called for if-then and also for loop patterns
void GenXPredToSimdCF::generateGoto(BranchInst *Br) {
  IRBuilder<> Builder(Br);

  auto SimdTy = cast<IGCLLVM::FixedVectorType>(Mask->getType());
  auto SimdWidth = SimdTy->getNumElements();

  auto *F = Br->getFunction();
  Value *EM = getEM(F);
  Instruction *OldGotoEM =
      Builder.CreateLoad(IGCLLVM::getNonOpaquePtrEltTy(EM->getType()), EM,
                         false /*isVolatile*/, EM->getName());

  Value *GotoRMAddr = getRMAddr(JP, SimdWidth);
  auto *RmTy = IGCLLVM::getNonOpaquePtrEltTy(GotoRMAddr->getType());

  Instruction *OldGotoRM = Builder.CreateLoad(
      IGCLLVM::getNonOpaquePtrEltTy(GotoRMAddr->getType()), GotoRMAddr,
      false /*isVolatile*/, GotoRMAddr->getName());

  if (RmTy != Mask->getType()) {
    Mask = Builder.CreateTrunc(Mask, RmTy);
  }

  // TODO search already exist not-mask
  if (!NeedSwap)
    Mask = Builder.CreateXor(Mask, Constant::getAllOnesValue(RmTy),
                                    Mask->getName() + ".not");
  // Create Goto instructions
  auto *M = Br->getModule();
  Type *GotoTys[] = {OldGotoEM->getType(), OldGotoRM->getType()};
  auto *GotoDecl = GenXIntrinsic::getGenXDeclaration(
      M, GenXIntrinsic::genx_simdcf_goto, GotoTys);
  Value *GotoArgs[] = {OldGotoEM, OldGotoRM, Mask};
  CallInst *Goto = Builder.CreateCall(GotoDecl, GotoArgs, "goto");
  Goto->setConvergent();

  // Create calculation with goto
  auto *NewGotoEM = Builder.CreateExtractValue(Goto, 0, "goto.extractem");
  Builder.CreateStore(NewGotoEM, EM, false /*isVolatile*/);

  Instruction *NewGotoRM = dyn_cast<Instruction>(
      Builder.CreateExtractValue(Goto, 1, "goto.extractrm"));
  IGC_ASSERT(GotoRMAddr->getType()->isPointerTy());

  if (RmTy != NewGotoRM->getType()) { // SimdTy) {
    // TODO use builder
    NewGotoRM = CastInst::CreateTruncOrBitCast(
        NewGotoRM, RmTy, NewGotoRM->getName() + ".simdcast",
        NewGotoRM->getNextNonDebugInstruction());
  }
  Builder.CreateStore(NewGotoRM, GotoRMAddr, false /*isVolatile*/);
  auto *LoweredGotoCond =
      Builder.CreateExtractValue(Goto, 2, "goto.extractcond");

  // Branch prepare
  Br->setCondition(LoweredGotoCond);
}

void GenXPredToSimdCF::insertIfGotoJoin(SimdCFIfRegion &R) {
  Br = R.getIfSimdBranch(); // TODO : auto *
  auto *M = Br->getModule();
  // Will be removed:
  OldCond = R.getIfSimdCondition();
  JP = R.getIfThenRegion()->getEntry();
  NeedSwap = R.needSwap();

  generateGoto(Br);

  Function *F = Br->getFunction();
  auto *IfThenExit = R.getIfThenRegion()->getExit();
  StringRef JoinBlockPrefix = IfThenExit->getName();
  JoinBlockPrefix.consume_back(".then");
  Twine JoinBlockName = JoinBlockPrefix + ".afterthen";
  auto *JoinBlock = BasicBlock::Create(M->getContext(), JoinBlockName, F,
                                       IfThenExit->getNextNode());
  auto ThenId = R.getIdThen();
  Br->setSuccessor(1 - ThenId, JoinBlock);
  if (NeedSwap)
    Br->swapSuccessors();

  auto *IfThenExitBr = IfThenExit->getTerminator();
  IGC_ASSERT(isa<BranchInst>(IfThenExitBr));
  IfThenExitBr->setSuccessor(0, JoinBlock);

  // Additional fix for else-branch
  if (R.hasElse()) {
    auto *IfEndBB = IfThenExitBr->getSuccessor(1);
    IfThenExitBr->setSuccessor(1, JoinBlock);
    for (auto &PHI : IfEndBB->phis()) {
      PHI.replaceIncomingBlockWith(IfThenExit, JoinBlock);
    }
  }
  JoinBlocks[&R] = JoinBlock;

  auto *JoinCond = generateJoin(JoinBlock);

  IRBuilder<> Builder(JoinBlock, JoinBlock->end());
  if (R.hasElse()) {
    BasicBlock *JoinCondFalse =
        R.hasElse() ? R.getIfElseRegion()->getEntry() : R.getExit();
    Builder.CreateCondBr(JoinCond, R.getExit(), JoinCondFalse);
  } else
    Builder.CreateBr(R.getExit());
}

void GenXPredToSimdCF::insertLoopGotoJoin(SimdCFLoopRegion &R) {
  Br = R.getSimdBranch(); // TODO : auto *
  auto *M = Br->getModule();
  // Will be removed:
  OldCond = R.getSimdCondition();
  JP = R.getLoopHead();
  NeedSwap = R.needSwap();

  generateGoto(Br);

  Function *F = Br->getFunction();

  auto *LastBlock = R.getSimdBranch()->getParent();
  StringRef JoinBlockPrefix = LastBlock->getName();

  Twine JoinBlockName = JoinBlockPrefix + ".join";
  auto *JoinBlock = BasicBlock::Create(M->getContext(), JoinBlockName, F,
                                       LastBlock->getNextNode());

  IGC_ASSERT(isa<BranchInst>(Br));
  Br->setSuccessor(R.getIdExit(), JoinBlock);
  if (NeedSwap)
    Br->swapSuccessors();

  // Additional fix for else-branch
  JoinBlocks[&R] = JoinBlock;
  generateJoin(JoinBlock);

  IRBuilder<> Builder(JoinBlock, JoinBlock->end());
  Builder.CreateBr(R.getExit());

  LLVM_DEBUG(dbgs() << "Join block\n"; JoinBlock->dump());
}

void GenXPredToSimdCF::removeMask(SimdCFIfRegion &R) {
  auto *Mask = R.getMask();
  while (isa<CastInst>(Mask))
    Mask = cast<Instruction>(Mask)->getOperand(0);

  LLVM_DEBUG(dbgs() << "Trying to remove selects with mask: "; Mask->dump());
  auto *ThenReg = R.getIfThenRegion();
  auto *ElseReg = R.getIfElseRegion();
  if (NeedSwap) {
    ElseReg = ThenReg;
    ThenReg = R.getIfElseRegion();
  }
  if (ThenReg)
    for (BasicBlock *BB : ThenReg->blocks()) {
      for (auto &I : *BB) {
        if (isa<SelectInst>(I)) {
          auto *Select = cast<SelectInst>(&I);
          LLVM_DEBUG(dbgs() << "Find select: "; Select->dump());
          if (Mask == Select->getCondition()) {
            LLVM_DEBUG(dbgs() << "Removing select: "; Select->dump());
            // TODO: Really need replace to RM-mask
            // Select->replaceAllUsesWith(Select->getTrueValue());
          }
        }
      }
    }

  // Replace Mask for EM's
  if (ElseReg)
    for (BasicBlock *BB : ElseReg->blocks()) {
      for (auto &I : *BB) {
        if (isa<SelectInst>(I)) {
          auto *Select = cast<SelectInst>(&I);

          LLVM_DEBUG(dbgs() << "Find select: "; Select->dump());
          auto *Cond = Select->getCondition();
          auto *CondTy = Cond->getType();
          while (isa<CastInst>(Cond))
            Cond = cast<Instruction>(Cond)->getOperand(0);

          if (Mask == Cond) {
            IRBuilder<> Builder(Select);
            Value *EM = getEM(Select->getFunction());
            Value *NewEM =
                Builder.CreateLoad(IGCLLVM::getNonOpaquePtrEltTy(EM->getType()),
                                   EM, false /*isVolatile*/, EM->getName());
            if (CondTy != NewEM->getType()) {
              auto *PredRgnTy =
                  cast<IGCLLVM::FixedVectorType>(NewEM->getType());
              auto *PredTy = cast<IGCLLVM::FixedVectorType>(CondTy);
              auto *M = Select->getModule();
              auto *OffsetTy = Builder.getInt16Ty();
              auto *RdPredRgnFunc = GenXIntrinsic::getAnyDeclaration(
                  M, GenXIntrinsic::genx_rdpredregion,
                  {PredTy, PredRgnTy, OffsetTy});

              Value *Args[] = {NewEM, Builder.getInt32(0)};

              NewEM = Builder.CreateCall(RdPredRgnFunc, Args);
            }
            Select->setCondition(NewEM);
          }
        }
      }
    }
}

// Check that two instruction has equal incoming value and return it
Value *findGeneralIncomeValue(SelectInst *V1, SelectInst *V2) {
  Constant *ConstOperandV1;
  if (auto *C1 = dyn_cast<Constant>(V1->getTrueValue()))
    ConstOperandV1 = C1;
  else
    ConstOperandV1 = dyn_cast<Constant>(V1->getFalseValue());

  if (ConstOperandV1) {
    Constant *ConstOperandV2;
    if (auto *C2 = dyn_cast<Constant>(V2->getTrueValue()))
      ConstOperandV2 = C2;
    else
      ConstOperandV2 = dyn_cast<Constant>(V2->getFalseValue());

    IGC_ASSERT(ConstOperandV1 && ConstOperandV2);
    if (dyn_cast<Constant>(ConstOperandV1) !=
        dyn_cast<Constant>(ConstOperandV2))
      return nullptr;
    return ConstOperandV1;
  }
  // TODO support non-consts
  IGC_ASSERT_UNREACHABLE();
  return nullptr;
}

// Check Phi-node for fixing:
//       if.then            if.else
//     %a  ...XX...       %b  XXX..XXX
//      for  phi [%a if.then], [%b if.else]
//   %a and %b must have general incoming data for merge (`X`)
// TODO - it really may be `undef`, if we sure, that all lines are used
// incoming parameters: PHINode, SimdCFIfRegion
// results:             SelectInst, Value
bool checkSelects(PHINode &PHINode, SimdCFIfRegion &R,
                  llvm::SelectInst *&ifThenSelect, Value *&data) {
  IGC_ASSERT(PHINode.getNumIncomingValues() == 2);
  auto *IV1 = PHINode.getIncomingValue(0);
  auto *IV2 = PHINode.getIncomingValue(1);
  // R.getIfThen vs R.getIfElse
  if (isa<SelectInst>(IV1) && isa<SelectInst>(IV2)) {
    SelectInst *V1 = dyn_cast<SelectInst>(IV1);
    SelectInst *V2 = dyn_cast<SelectInst>(IV2);

    // TODO: support check for each block in region
    IGC_ASSERT(R.getIfThenRegion()->getEntry() ==
               R.getIfThenRegion()->getExit());
    IGC_ASSERT(R.getIfElseRegion()->getEntry() ==
               R.getIfElseRegion()->getExit());

    // Just check that this selects from needed bb-s
    ifThenSelect = V1->getParent() == R.getIfThenRegion()->getEntry() ? V1 : V2;
    auto *ifElseSelect =
        V2->getParent() == R.getIfElseRegion()->getEntry() ? V2 : V1;
    IGC_ASSERT(ifThenSelect != ifElseSelect);
    // Here create additional phi in if-after-then
    // Check is operand are constants?

    data = findGeneralIncomeValue(V1, V2);
    if (!data)
      return false;
    return true;
  } else {
    LLVM_DEBUG(dbgs() << "Error! Not select inst! Unsupported!\n");
    return false;
  }
}

// Fix PHIs after goto/join were inserted
void GenXPredToSimdCF::fixPHIs(SimdCFIfRegion &R) {
  auto *EBB = R.getExit();
  if (!R.hasElse()) {
    LLVM_DEBUG(dbgs() << "Phi fixing for: "; EBB->getName());
    Instruction *InsertPoint = JoinBlocks[&R]->getFirstNonPHI();
    IGC_ASSERT(JoinBlocks[&R] != EBB);
    LLVM_DEBUG(dbgs() << "Insert to : "; InsertPoint->getName());
    LLVM_DEBUG(EBB->getName());
    LLVM_DEBUG(JoinBlocks[&R]->getName());

    SmallVector<PHINode *, 3> Phis;
    for (auto &PHINode : EBB->phis()) {
      Phis.push_back(&PHINode);
    }
    for (auto *PHINode : Phis) {
      PHINode->moveBefore(InsertPoint);
      LLVM_DEBUG(PHINode->dump());
      if (JPSplit)
        for (unsigned i = 0, e = PHINode->getNumIncomingValues(); i != e; i++) {
          auto *IB = PHINode->getIncomingBlock(i);
          if (IB == OldCondBB)
            PHINode->setIncomingBlock(i, JPSplit);
        }
    }
    return;
  }

  LLVM_DEBUG(dbgs() << "EBB: "; EBB->getName());
  for (auto &PHINode : EBB->phis()) {
    LLVM_DEBUG(dbgs() << "PHI in EBB: "; PHINode.getName());
    // Save all eges ids for re-fix them with new phi-node
    std::vector<unsigned> PhiIncomingID;
    for (unsigned i = 0, e = PHINode.getNumIncomingValues(); i != e; i++) {
      auto *IV = PHINode.getIncomingValue(i);
      auto *IB = PHINode.getIncomingBlock(i);
      if (R.getIfThenRegion()->getExit() == IB)
        PHINode.addIncoming(IV, JoinBlocks[&R]);

      if (JPSplit && IB == OldCondBB)
        PHINode.setIncomingBlock(i, JPSplit);

      if (JoinBlocks[&R] == PHINode.getIncomingBlock(i))
        PhiIncomingID.push_back(i);
    }
    if (R.isIfRegion()) {
      SelectInst *ifThenSelect = nullptr;
      Value *data = nullptr;
      if (!checkSelects(PHINode, R, ifThenSelect, data)) {
        IGC_ASSERT_UNREACHABLE();
        return;
      }

      SelectInst *VForFix = dyn_cast<SelectInst>(ifThenSelect);
      // Find edge from after-then block

      if (Constant *ConstOperandV1 = dyn_cast<Constant>(data)) {
        // for (JoinBlocks[&R]->)
        //  Create phi with const input in after-then block
        IRBuilder<> Builder(&JoinBlocks[&R]->front());
        auto Ty = ConstOperandV1->getType();
        auto *PHI = Builder.CreatePHI(Ty, 2, "afterthenPHI");
        for (auto it = pred_begin(JoinBlocks[&R]),
                  et = pred_end(JoinBlocks[&R]);
             it != et; ++it) {
          if (R.getIfThenRegion()->getExit() == *it)
            PHI->addIncoming(VForFix, R.getIfThenRegion()->getExit());
          else
            PHI->addIncoming(ConstOperandV1, *it);
        }
        // Restore incoming id
        for (auto &IncID : PhiIncomingID) {
          PHINode.setIncomingValue(IncID, PHI);
          PHINode.setIncomingBlock(IncID, JoinBlocks[&R]);
        }
      } else {
        // TODO support non-consts
        LLVM_DEBUG(dbgs() << "Error! Not constant inst! Unsupported!\n");
        IGC_ASSERT_UNREACHABLE();
      }
    } else
      IGC_ASSERT_UNREACHABLE();
    // TODO support while-loop
  }
}

void GenXPredToSimdCF::fixPHIs(SimdCFLoopRegion &R) {
  auto *EBB = R.getExit();
  auto *LoopEnd = R.getSimdBranch()->getParent();

  LLVM_DEBUG(dbgs() << "Phi fixing for: "; EBB->getName());
  Instruction *InsertPoint = JoinBlocks[&R]->getFirstNonPHI();
  IGC_ASSERT(JoinBlocks[&R] != EBB);
  LLVM_DEBUG(dbgs() << "Insert to : "; InsertPoint->getName());
  LLVM_DEBUG(EBB->getName());
  LLVM_DEBUG(JoinBlocks[&R]->getName());

  for (auto &PHINode : EBB->phis()) {
    // PHINode->moveBefore(InsertPoint);
    for (unsigned i = 0, e = PHINode.getNumIncomingValues(); i != e; i++) {
      auto *IB = PHINode.getIncomingBlock(i);
      if (LoopEnd == IB)
        PHINode.setIncomingBlock(i, JoinBlocks[&R]);
    }
    LLVM_DEBUG(PHINode.dump());
  }
}

bool GenXPredToSimdCF::transform(SimdCFIfRegion &R) {
  LLVM_DEBUG(dbgs() << "IfRegion: Insert if goto\n");
  insertIfGotoJoin(R);

  OldCondBB = OldCond->getParent();
  LLVM_DEBUG(dbgs() << " For: " << OldCondBB->getParent()->getName() << "\n");
  JPSplit =
      OldCondBB->splitBasicBlock(OldCond->getPrevNode(), OldCondBB->getName());
  LLVM_DEBUG(dbgs() << "splitting block\n : " << OldCondBB->getName()
                    << " created " << JPSplit->getName() << "\n");

  if (OldCond->use_empty())
    OldCond->eraseFromParent();

  LLVM_DEBUG(dbgs() << "IfRegion: Fixing PHIs\n");
  fixPHIs(R);

  LLVM_DEBUG(dbgs() << "IfRegion: Removing mask\n");
  removeMask(R);

  return true;
}

bool GenXPredToSimdCF::transform(SimdCFLoopRegion &R) {
  LLVM_DEBUG(dbgs() << "LoopRegion: Insert if goto\n");
  insertLoopGotoJoin(R);

  LLVM_DEBUG(dbgs() << "LoopRegion: Fixing PHIs\n");
  fixPHIs(R);

  return true;
}

bool GenXPredToSimdCF::transform(SimdCFRegion &R) {

  Mask = R.getMask();
  auto SimdTy = cast<IGCLLVM::FixedVectorType>(Mask->getType());
  IGC_ASSERT(SimdTy);
  auto SimdWidth = SimdTy->getNumElements();
  if (SimdWidth <= 1)
    return false;

  if (R.isIfRegion())
    return transform(static_cast<SimdCFIfRegion &>(R));

  if (R.isLoopRegion())
    return transform(static_cast<SimdCFLoopRegion &>(R));

  return false;
}

bool GenXPredToSimdCF::runOnFunction(Function &F) {
  const auto &BackendConfig = getAnalysis<GenXBackendConfig>();

  if (!EnableSimdCFTransform && !BackendConfig.isBiFEmulationCompilation())
    return false;

  auto *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  bool Changed = false;

  LLVM_DEBUG(dbgs() << "Run GenXPredToSimdCF on function " << F.getName()
                    << '\n');

  auto Regions = findSimdCFRegions(F);

  for (auto &R : Regions) {
    bool ChangeCurrReg = false;
    if (!R->verify()) {
      LLVM_DEBUG(dbgs() << "Not verified SimdCFRegion:\n"; R->dump());
    } else {
      LLVM_DEBUG(dbgs() << "Verified SimdCFRegion:\n"; R->dump());
      ChangeCurrReg = transform(*R);
    }
    // If apply - the pass changes dominate structure
    if (ChangeCurrReg) {
      DT->recalculate(F);
      PDT->recalculate(F);
    }
    Changed |= ChangeCurrReg;
    Br = nullptr;
    Mask = nullptr;
    JP = nullptr;
    JPSplit = nullptr;
    OldCondBB = nullptr;
    NeedSwap = false;
  }

  LLVM_DEBUG(if (Regions.empty()) dbgs() << "Find no SimdCFRegions\n";
             else { dbgs() << "SimdCFRegions applyed\n"; } dbgs()
             << "For function " << F.getName() << "\n";);

  IGC_ASSERT(DT->verify());
  PDT = nullptr;

  return Changed;
}

} // namespace

char GenXPredToSimdCF::ID = 0;

namespace llvm {
void initializeGenXPredToSimdCFPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXPredToSimdCF, DEBUG_TYPE, DEBUG_TYPE, false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(RegionInfoPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXPredToSimdCF, DEBUG_TYPE, DEBUG_TYPE, false, false)

namespace llvm {
FunctionPass *createGenXPredToSimdCFPass() {
  initializeGenXPredToSimdCFPass(*PassRegistry::getPassRegistry());
  return new GenXPredToSimdCF();
}
} // namespace llvm
