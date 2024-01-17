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
// TODO: use isMaskPacking for find out ispc masks
// TODO: now only matching for top-level region works good (for simple tests at
// least). Need to add matching nested and add transformation.
//
// NOTE: transformation is much harded in details than it is decribed above.
// Main problem is to make this pass and SimdCFConformace be compatible.

#include "vc/Utils/GenX/KernelInfo.h"
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
#include <llvm/Support/FormattedStream.h>

#define DEBUG_TYPE "simdcf-region"

using namespace llvm;

static cl::opt<bool>
    SimdCFRMLoopMask("simdcf-rm-loop-mask", cl::init(false), cl::Hidden,
                     cl::desc("Replace rm-mask for loop-pattern"));

static cl::opt<bool> SimdCFSkipSearchPreds(
    "simdcf-skip-search-preds", cl::init(false), cl::Hidden,
    cl::desc("Do not additionally search predicates for apply"));

namespace llvm::genx {
// Need declarations of this functions here because of declaration conflict
// beetween genx::Region from GenXUtils.h and llvm::Region
bool isPredicate(Instruction *Inst);
bool isPredNot(Instruction *Inst);

using RegionIterator = SmallVector<BasicBlock *, 8>;

class SimdCFRegion : public Region {
protected:
  Value *Mask = nullptr;
  enum CFRegionKind { RK_IfReg, RK_LoopReg, RK_BaseReg };
  const CFRegionKind Kind;
public:
  SimdCFRegion(BasicBlock *Entry, BasicBlock *Exit, RegionInfo *RI,
               DominatorTree *DT, SimdCFRegion *Parent, CFRegionKind K);
  virtual ~SimdCFRegion() = default;

  SimdCFRegion() = delete;
  SimdCFRegion(const SimdCFRegion &) = delete;
  SimdCFRegion &operator=(const SimdCFRegion &) = delete;
  virtual bool verify() const = 0;
  auto *getMask() const;
  CFRegionKind getKind() const;
  void print(raw_ostream &ROS);
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  virtual void dump();
#endif
};

class SimdCFRegionBase : public SimdCFRegion {
  RegionIterator BBlocks;

public:
  SimdCFRegionBase(BasicBlock *Entry, BasicBlock *Exit, RegionInfo *RI,
                   DominatorTree *DT, SimdCFRegion *Parent = nullptr,
                   CFRegionKind K = RK_BaseReg);
  virtual ~SimdCFRegionBase() = default;

  SimdCFRegionBase() = delete;
  SimdCFRegionBase(const SimdCFRegionBase &) = delete;
  SimdCFRegionBase &operator=(const SimdCFRegionBase &) = delete;

  virtual bool verify() const;
  virtual RegionIterator *Blocks();
  virtual const RegionIterator *Blocks() const;
  // llvm RTTY information
  static bool classof(const SimdCFRegion *S) {
    return S->getKind() == RK_BaseReg;
  }
};

using SimdCFRegionPtr = std::unique_ptr<SimdCFRegionBase>;
using SimdCFRegionsT = std::vector<SimdCFRegionPtr>;

class SimdCFIfRegion final : public SimdCFRegionBase {
  SimdCFRegionBase *IfThenRegion = nullptr;
  SimdCFRegionBase *IfElseRegion = nullptr;
  BranchInst *SimdIfBranch = nullptr;
  RegionIterator AllBlocks;

public:
  SimdCFIfRegion(BasicBlock *, SimdCFRegionBase *, SimdCFRegionBase *,
                 BasicBlock *, RegionInfo *, DominatorTree *, SimdCFRegion *);
  virtual ~SimdCFIfRegion() {
    delete IfThenRegion;
    delete IfElseRegion;
  };

  SimdCFIfRegion() = delete;
  SimdCFIfRegion(const SimdCFIfRegion &) = delete;
  SimdCFIfRegion &operator=(const SimdCFIfRegion &) = delete;

  bool hasElse() const;
  SimdCFRegionBase *getIfThenRegion() const;
  SimdCFRegionBase *getIfElseRegion() const;
  BranchInst *getIfSimdBranch() const;
  int getIdThen() const;
  CallInst *getIfSimdCondition() const;
  bool verify() const override;
  // Conformance-pass expect true edge to join-block - that's why here swap
  // successors
  bool needSwap();
  RegionIterator *Blocks() override;
  BasicBlock *SplitBlock(Instruction *);
  // llvm RTTY information
  static bool classof(const SimdCFRegion *S) {
    return S->getKind() == RK_IfReg;
  }
};

class SimdCFLoopRegion final : public SimdCFRegionBase {
  Loop *SimdLoop;
  BranchInst *SimdLoopBranch = nullptr;
  BasicBlock *LoopHead = nullptr;
public:
  SimdCFLoopRegion(BasicBlock *, Loop *, BasicBlock *, RegionInfo *,
                   DominatorTree *, SimdCFRegion *Parent = nullptr);
  virtual ~SimdCFLoopRegion() = default;

  SimdCFLoopRegion() = delete;
  SimdCFLoopRegion(const SimdCFLoopRegion &) = delete;
  SimdCFLoopRegion &operator=(const SimdCFLoopRegion &) = delete;

  BranchInst *getSimdBranch() const;
  CallInst *getSimdCondition() const;
  BasicBlock *getLoopHead() const;
  int getIdExit() const;
  bool verify() const override;
  // Conformance-pass expect true edge to join-block - that's why here swap
  // successors
  bool needSwap();
  // llvm RTTY information
  static bool classof(const SimdCFRegion *S) {
    return S->getKind() == RK_LoopReg;
  }
};

// Usefull getters:
static inline uint64_t getSimdSize(Type *Ty) {
  auto *SimdTy = cast<IGCLLVM::FixedVectorType>(Ty);
  return SimdTy->getNumElements();
}

static inline uint64_t getSimdSize(CallInst *AnyInst) {
  auto *Ty = AnyInst->getOperand(0)->getType();
  return getSimdSize(Ty);
}

// SimdCFRegion methods definition

SimdCFRegion::SimdCFRegion(BasicBlock *Entry, BasicBlock *Exit, RegionInfo *RI,
                           DominatorTree *DT, SimdCFRegion *Parent,
                           CFRegionKind K)
    : Region(Entry, Exit, RI, DT, Parent), Kind(K) {}

auto *SimdCFRegion::getMask() const { return Mask; }

SimdCFRegion::CFRegionKind SimdCFRegion::getKind() const { return Kind; }

// SimdCFRegionBase methods definition

SimdCFRegionBase::SimdCFRegionBase(BasicBlock *Entry, BasicBlock *Exit,
                                   RegionInfo *RI, DominatorTree *DT,
                                   SimdCFRegion *Parent, CFRegionKind K)
    : SimdCFRegion(Entry, Exit, RI, DT, Parent, K) {
  if (Entry == Exit)
    BBlocks.push_back(Entry);
  else
    for (auto *BB : blocks())
      BBlocks.push_back(BB);
}

bool SimdCFRegionBase::verify() const { return true; }
RegionIterator *SimdCFRegionBase::Blocks() { return &BBlocks; }
const RegionIterator *SimdCFRegionBase::Blocks() const { return &BBlocks; }

// SimdCFIfRegion methods definition

RegionIterator *SimdCFIfRegion::Blocks() { return &AllBlocks; }

SimdCFIfRegion::SimdCFIfRegion(BasicBlock *Entry, SimdCFRegionBase *IfThen,
                               SimdCFRegionBase *IfElse, BasicBlock *Exit,
                               RegionInfo *RI, DominatorTree *DT,
                               SimdCFRegion *Parent = nullptr)
    : SimdCFRegionBase(Entry, Exit, RI, DT, Parent, RK_IfReg),
      IfThenRegion(IfThen), IfElseRegion(IfElse),
      AllBlocks(*SimdCFRegionBase::Blocks()) {
  IGC_ASSERT(IfThenRegion);
  SimdIfBranch = cast<BranchInst>(Entry->getTerminator());
  CallInst *Cond = dyn_cast<CallInst>(SimdIfBranch->getCondition());
  IGC_ASSERT(Cond);
  Mask = Cond->getArgOperand(0);
  AllBlocks.append(IfThenRegion->Blocks()->begin(),
                   IfThenRegion->Blocks()->end());
  if (hasElse())
    AllBlocks.append(IfElseRegion->Blocks()->begin(),
                     IfElseRegion->Blocks()->end());
}

bool SimdCFIfRegion::hasElse() const { return IfElseRegion != nullptr; }

SimdCFRegionBase *SimdCFIfRegion::getIfThenRegion() const {
  return IfThenRegion;
}

SimdCFRegionBase *SimdCFIfRegion::getIfElseRegion() const {
  return IfElseRegion;
}

BranchInst *SimdCFIfRegion::getIfSimdBranch() const { return SimdIfBranch; }

int SimdCFIfRegion::getIdThen() const {
  IGC_ASSERT(getIfSimdBranch()->isConditional());
  IGC_ASSERT(getIfSimdBranch()->getNumSuccessors() == 2);
  if (getIfSimdBranch()->getSuccessor(0) == IfThenRegion->getEntry())
    return 0;
  IGC_ASSERT(getIfSimdBranch()->getSuccessor(1) == IfThenRegion->getEntry());
  return 1;
}

CallInst *SimdCFIfRegion::getIfSimdCondition() const {
  return dyn_cast<CallInst>(getIfSimdBranch()->getCondition());
}

// TODO: need to add algo to verify mask for simdcf region:
// 1. For each masked op: op-mask in submask of region mask;
// 2. For each induction variable: var is masked with region mask;
// 3. For each subregion: subregion-mask is submask of region mask.
// SindCFIfRegion is considered verified if each of conditions above are true.
bool SimdCFIfRegion::verify() const {
  // Current implementation do not support nested conditon branch in regions.
  // it is needed to check/replace it for simd-width branches
  // (error in finalizer - assertion failed: don't handle instruction in SIMD CF
  // for now)
  auto *RegionCheck = getIfThenRegion();
  auto CheckIf = [&](BasicBlock *BB) {
    if (BB == RegionCheck->getExit()) {
      LLVM_DEBUG(dbgs() << "First Exit\n");
      return false;
    }
    auto *BI = dyn_cast<BranchInst>(BB->getTerminator());
    if (BI && BI->getNumSuccessors() > 1) {
      LLVM_DEBUG(BI->print(dbgs()));
      LLVM_DEBUG(dbgs() << "Exit for branch with num succ "
                        << BI->getNumSuccessors() << " \n");
      return true;
    }
    LLVM_DEBUG(dbgs() << "Default exit\n");
    return false;
  };

  LLVM_DEBUG(dbgs() << " Dump blocks name: \n"; for (auto *BB
                                                     : *RegionCheck->Blocks()) {
    dbgs() << " BB = " << BB->getName() << "\n";
  });
  if (find_if(*RegionCheck->Blocks(), CheckIf) != RegionCheck->Blocks()->end())
    return false;
  if (!hasElse())
    return true;

  RegionCheck = getIfElseRegion();
  if (find_if(*RegionCheck->Blocks(), CheckIf) != RegionCheck->Blocks()->end())
    return false;
  return true;
}

bool SimdCFIfRegion::needSwap() { return getIdThen() == 0; }

BasicBlock *SimdCFIfRegion::SplitBlock(Instruction *OldCond) {
  auto *OldCondBB = OldCond->getParent();

  auto *JPSplit = OldCondBB->splitBasicBlock(OldCond, OldCondBB->getName());

  AllBlocks.push_back(JPSplit);
  return JPSplit;
}

// SimdCFLoopRegion methods definition

SimdCFLoopRegion::SimdCFLoopRegion(BasicBlock *Entry, Loop *L, BasicBlock *Exit,
                                   RegionInfo *RI, DominatorTree *DT,
                                   SimdCFRegion *Parent)
    : SimdCFRegionBase(Entry, Exit, RI, DT, Parent, RK_LoopReg), SimdLoop(L) {
  SimdLoopBranch =
      cast<BranchInst>(SimdLoop->getExitingBlock()->getTerminator());
  auto *Cond = cast<CallInst>(getSimdBranch()->getCondition());
  Mask = Cond->getArgOperand(0);
  LoopHead = SimdLoopBranch->getSuccessor(getIdExit());
}

BranchInst *SimdCFLoopRegion::getSimdBranch() const { return SimdLoopBranch; }

CallInst *SimdCFLoopRegion::getSimdCondition() const {
  return dyn_cast<CallInst>(getSimdBranch()->getCondition());
}

BasicBlock *SimdCFLoopRegion::getLoopHead() const { return LoopHead; }

int SimdCFLoopRegion::getIdExit() const {
  IGC_ASSERT(getSimdBranch()->isConditional());
  IGC_ASSERT(getSimdBranch()->getNumSuccessors() == 2);
  if (getSimdBranch()->getSuccessor(0) == getExit())
    return 0;
  IGC_ASSERT(getSimdBranch()->getSuccessor(1) == getExit());
  return 1;
}

bool SimdCFLoopRegion::verify() const {
  auto CheckLoop = [&](BasicBlock *BB) {
    if (BB == getEntry() || BB == SimdLoop->getExitingBlock())
      return false;

    auto *BI = dyn_cast<BranchInst>(BB->getTerminator());
    if (BI && BI->getNumSuccessors() > 1)
      return true;
    return false;
  };

  if (std::find_if(Blocks()->begin(), Blocks()->end(), CheckLoop) !=
      Blocks()->end())
    return false;

  return true;
}

bool SimdCFLoopRegion::needSwap() { return getIdExit() != 0; }

void SimdCFRegion::print(raw_ostream &ROS) {
  formatted_raw_ostream OS(ROS);
  if (auto *Loop = dyn_cast<SimdCFLoopRegion>(this)) {
    OS << "Print Loop blocks:\n";
    for (auto *BB : *Loop->Blocks())
      OS << "    loop block " << BB->getName() << "\n";
    Region::print(OS);
  } else if (auto *IfThen = dyn_cast<SimdCFIfRegion>(this)) {
    OS << "Print If-Then-Else blocks:\n";
    OS << "  curr empty: " << (IfThen->Blocks()->empty() ? "y" : "n") << "\n";

    for (auto *BB : *IfThen->Blocks()) {
      OS << "  curr blocks ";
      BB->printAsOperand(OS, false);
      OS << "\n";
    }
    OS << "Then-blocks:\n";
    IfThen->getIfThenRegion()->print(OS);
    if (IfThen->hasElse()) {
      OS << "Else-blocks:\n";
      IfThen->getIfElseRegion()->print(OS);
    }
    Region::print(OS);
  } else if (auto *Base = dyn_cast<SimdCFRegionBase>(this)) {
    OS << "\t\tPrint BaseRegion blocks:\n";
    auto *CurrBlocks = Base->Blocks();
    OS << "\t\tempty: " << (CurrBlocks->empty() ? "y" : "n") << "\n";
    for (auto *BB : *CurrBlocks) {
      OS << "\t\tblock ";
      BB->printAsOperand(OS, false);
      OS << "\n";
    }
  } else {
    Region::print(OS);
  }
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void SimdCFRegion::dump() { print(dbgs()); }
#endif

} // namespace llvm::genx

using namespace genx;

namespace {

class GenXPredToSimdCF final : public FunctionPass {
  // Created (existed) execution-mask addresses
  std::map<Function *, AllocaInst *> EMAddrs;
  // List of load-instructions for execution-mask
  std::map<BasicBlock *, LoadInst *> EMLoads;
  // Created (existed) resume-mask addresses
  std::map<BasicBlock *, AllocaInst *> RMAddrs;
  std::map<SimdCFRegion *, BasicBlock *> JoinBlocks;
  SmallPtrSet<Value *, 8> ToErase;
  PostDominatorTree *PDT = nullptr;
  // Variables, used during generation goto-joins
  CallInst *OldCond = nullptr;
  Value *Mask = nullptr;
  BasicBlock *JP = nullptr;
  BasicBlock *JPSplit = nullptr;
  BasicBlock *OldCondBB = nullptr;
  bool NeedSwap = false;
public:

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
  }

private:
  SimdCFRegionsT findSimdCFRegions(Function &F);
  void replaceUses(Use *Ui, bool BuildEM);
  void insertIfGotoJoin(SimdCFIfRegion *R);
  void removeMask(SimdCFIfRegion *R);
  void removeMask(SimdCFLoopRegion *R);
  void removeMachMask(SimdCFRegionBase *R);
  void fixPHIs(SimdCFIfRegion *R);
  void insertLoopGotoJoin(SimdCFLoopRegion *R);
  void fixPHIs(SimdCFLoopRegion *R);
  bool transform(SimdCFRegion *R);
  bool transform(SimdCFIfRegion *R);
  bool transform(SimdCFLoopRegion *R);

  void generateGoto(BranchInst *Br);
  void generateOppositeMask(Type *RmTy);
  void generateThenGoto(BranchInst *Br, BasicBlock *Join);
  Value *generateJoin(BasicBlock *JoinBlock);
  Value *getEMAddr(Function *F);
  //  Get or create EM-Load for basic-block
  Value *getEMValue(BasicBlock *BB);
  void saveGotoEMRM(CallInst *Goto);
  bool TryReplace(SimdCFRegionBase *Reg, Use *Ui, bool IsEm);

  Value *getRMAddr(unsigned Width = 0);
  bool isSimdCFCondition(Value *Cond);
  std::pair<BranchInst *, Value *> findSimdCFBranchAndCondition(BasicBlock &BB);

  bool isReduceAnd(const Value *V);
  bool isReduceOr(const Value *V);
  bool areOppositeSimdCFConditions(Value *IfCond, Value *ElseCond);

  BasicBlock *getIfExitFromIfBranch(BranchInst &BrInst);
  BasicBlock *getIfExitFromElseBranch(BranchInst &BrInst);
  BasicBlock *getIfThenEntry(BranchInst &BrInst);
  BasicBlock *getIfThenEnd(BranchInst &BrInst);
  BasicBlock *getIfElseEntry(BranchInst &BrInst);
  BasicBlock *getIfElseEnd(BranchInst &BrInst);

  // TODO: simd-conformance pass expect select, wrregion, shufflevector
  // and gather/scatter need support more complex examples
  template <class BBContainer>
  bool analizeInsts(BBContainer *Container, Value *Cond);

  SimdCFRegionPtr tryMatchIf(BasicBlock &BB);
  std::pair<BranchInst *, Value *>
  findSimdCFLoopBranchAndCondition(const Loop &L);
  SimdCFRegionPtr tryMatchLoop(BasicBlock &BB);
};

Value *GenXPredToSimdCF::getEMAddr(Function *F) {
  // TODO: replace to Module or function-group for support stack calls
  IGC_ASSERT(F);
  auto EM = EMAddrs.find(F);
  if (EM != EMAddrs.end())
    return EMAddrs[F];

  auto *M = F->getParent();
  auto *EMTy = IGCLLVM::FixedVectorType::get(Type::getInt1Ty(M->getContext()),
                                             MAX_SIMD_CF_WIDTH);
  auto *InsertBefore = &JP->getParent()->front().front();
  auto *EMAddr = new AllocaInst(EMTy, 0 /*AddrSpace*/,
                                Twine("EM.") + JP->getName(), InsertBefore);
  new StoreInst(Constant::getAllOnesValue(EMTy), EMAddr, false /* isVolatile */,
                InsertBefore);
  EMAddrs[F] = EMAddr;
  return EMAddr;
}

Value *GenXPredToSimdCF::getEMValue(BasicBlock *BB) {
  auto EM = EMLoads.find(BB);
  if (EM != EMLoads.end())
    return EMLoads[BB];

  auto *Inst = BB->getFirstNonPHI();
  IRBuilder<> Builder(Inst);

  Value *EMAddr = getEMAddr(Inst->getFunction());
  auto *EMVal =
      Builder.CreateLoad(IGCLLVM::getNonOpaquePtrEltTy(EMAddr->getType()),
                         EMAddr, false /*isVolatile*/, EMAddr->getName());
  EMLoads[BB] = EMVal;
  return EMVal;
}

Value *GenXPredToSimdCF::getRMAddr(unsigned Width) {
  auto RMAddr = RMAddrs.find(JP);
  if (RMAddr == RMAddrs.end()) {
    auto *RMTy =
        IGCLLVM::FixedVectorType::get(Type::getInt1Ty(JP->getContext()), Width);
    auto *InsertBefore = &JP->getParent()->front().front();
    auto *RMAddr = new AllocaInst(RMTy, 0 /*AddrSpace*/,
                                  Twine("RM.") + JP->getName(), InsertBefore);
    new StoreInst(Constant::getNullValue(RMTy), RMAddr, false /* isVolatile */,
                  InsertBefore);
    RMAddrs[JP] = RMAddr;
  }
  return RMAddrs[JP];
}

bool GenXPredToSimdCF::isSimdCFCondition(Value *Cond) {
  // TODO: Support case GenXIntrinsic::genx_all:
  switch (GenXIntrinsic::getGenXIntrinsicID(Cond)) {
  case GenXIntrinsic::genx_any:
    return true;
  default:
    return false;
  }
}

std::pair<BranchInst *, Value *>
GenXPredToSimdCF::findSimdCFBranchAndCondition(BasicBlock &BB) {
  LLVM_DEBUG(dbgs() << "Trying to find Simd branch and condition in:\n"
                    << BB.getName() << "\n");

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

  LLVM_DEBUG(dbgs() << "Condition isn't a Simd CF condition:\n";
             Cond->print(dbgs()));
  return std::make_pair(nullptr, nullptr);
}

bool GenXPredToSimdCF::isReduceAnd(const Value *V) {
  if (V)
    if (const CallInst *CI = dyn_cast<CallInst>(V))
      return GenXIntrinsic::getGenXIntrinsicID(V) == GenXIntrinsic::genx_all ||
             CI->getIntrinsicID() ==
#if LLVM_VERSION_MAJOR < 12
                 Intrinsic::experimental_vector_reduce_and;
#else
                 Intrinsic::vector_reduce_and;
#endif
  return false;
}

bool GenXPredToSimdCF::isReduceOr(const Value *V) {
  if (V)
    if (const CallInst *CI = dyn_cast<CallInst>(V))
      return GenXIntrinsic::getGenXIntrinsicID(V) == GenXIntrinsic::genx_any ||
             CI->getIntrinsicID() ==
#if LLVM_VERSION_MAJOR < 12
                 Intrinsic::experimental_vector_reduce_or;
#else
                 Intrinsic::vector_reduce_or;
#endif
  return false;
}

bool GenXPredToSimdCF::areOppositeSimdCFConditions(Value *IfCond,
                                                   Value *ElseCond) {
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
  LLVM_DEBUG(dbgs() << "IfMask: " << *IfMask << "\nElseMask: " << *ElseMask);

  if (isIfCondReduceOr && isElseCondReduceOr) {
    if (!genx::isPredicate(IfMask) || !genx::isPredNot(ElseMask))
      return false;
    LLVM_DEBUG(dbgs() << "Comparing " << *IfMask << "against   "
                      << *ElseMask->getOperand(0));
    return IfMask == ElseMask->getOperand(0);
  }
  if (isIfCondReduceOr && !isElseCondReduceOr)
    return false; // not implemented
  if (!isIfCondReduceOr && isElseCondReduceOr)
    return false; // not implemented
  // last variant - if (!isIfCondReduceOr && !isElseCondReduceOr)
  if (!genx::isPredicate(IfMask) || !genx::isPredNot(ElseMask))
    return false;
  LLVM_DEBUG(dbgs() << "Comparing " << *IfMask << "against   "
                    << *ElseMask->getOperand(0));
  return IfMask == ElseMask->getOperand(0);
}

BasicBlock *GenXPredToSimdCF::getIfExitFromIfBranch(BranchInst &BrInst) {
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

BasicBlock *GenXPredToSimdCF::getIfExitFromElseBranch(BranchInst &BrInst) {
  if (!BrInst.isConditional())
    return nullptr;
  auto *IfElseEntry = getIfElseEntry(BrInst);
  auto IfExit =
      std::find_if(BrInst.successors().begin(), BrInst.successors().end(),
                   [IfElseEntry, this](BasicBlock *BB) {
                     return BB != IfElseEntry;
                   });
  return IfExit != BrInst.successors().end() ? *IfExit : nullptr;
}

BasicBlock *GenXPredToSimdCF::getIfThenEntry(BranchInst &BrInst) {
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

BasicBlock *GenXPredToSimdCF::getIfThenEnd(BranchInst &BrInst) {
  if (!BrInst.isConditional())
    return nullptr;
  auto *IfExit = getIfExitFromIfBranch(BrInst);
  if (!IfExit)
    return nullptr;
  auto IfThenEnd = std::find_if(
      pred_begin(IfExit), pred_end(IfExit),
      [&BrInst](BasicBlock *BB) { return BB != BrInst.getParent(); });
  return IfThenEnd != pred_end(IfExit) ? *IfThenEnd : nullptr;
}

BasicBlock *GenXPredToSimdCF::getIfElseEntry(BranchInst &BrInst) {
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

BasicBlock *GenXPredToSimdCF::getIfElseEnd(BranchInst &BrInst) {
  if (!BrInst.isConditional())
    return nullptr;
  auto IfExitPos =
      std::find_if(BrInst.successors().begin(), BrInst.successors().end(),
                   [&BrInst, this](BasicBlock *BB) {
                     return BB != getIfElseEntry(BrInst);
                   });
  if (IfExitPos == BrInst.successors().end())
    return nullptr;
  auto *IfExit = *IfExitPos;
  auto IfElseEnd = std::find_if(
      pred_begin(IfExit), pred_end(IfExit),
      [&BrInst](BasicBlock *BB) { return BB != BrInst.getParent(); });
  return IfElseEnd != pred_end(IfExit) ? *IfElseEnd : nullptr;
}

// TODO: simd-conformance pass expect select, wrregion, shufflevector
// and gather/scatter need support more complex examples
template <class BBContainer>
bool GenXPredToSimdCF::analizeInsts(BBContainer *Container, Value *Cond) {
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
  TryToFindBlock = [&](User *Usr, Value *PredInst, MatcherType Matcher) {
    if (Matcher(Usr, PredInst)) {
      auto *Inst = cast<Instruction>(Usr);
      LLVM_DEBUG(dbgs() << Inst->getFunction()->getName() << " - " << Inst->getName() << "\n");
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
          LLVM_DEBUG(dbgs() << "Find existing SimdCF " << Call->getName());
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
    if constexpr (std::is_base_of<SimdCFRegionBase, BBContainer>::value) {
      for (auto *BB : *Container->Blocks()) {
        for (auto &Inst : *BB) {
          if (isa<SelectInst>(Inst) && Inst.getOperand(0) == Const) {
            LLVM_DEBUG(dbgs() << "Find const-pred inst " << Inst.getName());
            RetVal = true;
          }
        }
      }
    } else {
      for (auto *BB : Container->blocks()) {
        for (auto &Inst : *BB) {
          if (isa<SelectInst>(Inst) && Inst.getOperand(0) == Const) {
            LLVM_DEBUG(dbgs() << "Find const-pred inst " << Inst.getName());
            RetVal = true;
          }
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

  BBs.clear();

  if (SimdCFSkipSearchPreds)
    return true;

  // 3-d  case - lookup for selects in predicate-users
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

  if constexpr (std::is_base_of<SimdCFRegionBase, BBContainer>::value) {
    for (auto *BB : *Container->Blocks()) {
      if (BBs.count(BB))
        return true; // ---> Success exit
    }
  } else {
    for (auto *BB : Container->blocks()) {
      if (BBs.count(BB))
        return true; // ---> Success exit
    }
  }

  return false;
}

SimdCFRegionPtr GenXPredToSimdCF::tryMatchIf(BasicBlock &BB) {

  LLVM_DEBUG(dbgs() << "Trying match Simd CF If on BB '" << BB.getName()
                    << "'\n");
  auto [Branch, Cond] = findSimdCFBranchAndCondition(BB);
  if (!Branch || !Cond)
    return nullptr;

  LLVM_DEBUG(dbgs() << "Find Simd CF If branch and condition:\n"
                    << *Cond << "\n"
                    << *Branch);
  BasicBlock *Entry = &BB;
  BasicBlock *IfThenEntry = getIfThenEntry(*Branch);
  if (!IfThenEntry)
    return nullptr;
  LLVM_DEBUG(dbgs() << "Find Simd CF IfThenEntry BB: ";
             IfThenEntry->printAsOperand(dbgs(), false); dbgs() << '\n');
  BasicBlock *IfThenEnd = getIfThenEnd(*Branch);
  if (!IfThenEnd)
    return nullptr;
  // Find Simd CF IfThenEnd BB: if.then
  LLVM_DEBUG(dbgs() << "Find Simd CF IfThenEnd BB: ";
             IfThenEnd->printAsOperand(dbgs(), false); dbgs() << '\n');

  auto IfThenRegion = std::make_unique<SimdCFRegionBase>(
      IfThenEntry, IfThenEnd, &getAnalysis<RegionInfoPass>().getRegionInfo(),
      &getAnalysis<DominatorTreeWrapperPass>().getDomTree(), nullptr);
  IfThenRegion->verifyRegion();
  // Finded IfThenRegion 'if.then => if.then': [0] if.then => if.then
  LLVM_DEBUG(dbgs() << "Finded IfThenRegion '" << IfThenRegion->getNameStr()
                    << "':\n";
             IfThenRegion->print(dbgs()));

  auto [ElseBranch, ElseCond] = findSimdCFBranchAndCondition(*IfThenEnd);

  if (ElseBranch && ElseCond) {
    bool areOppositeConds = areOppositeSimdCFConditions(Cond, ElseCond);
    if (!areOppositeConds) {
      LLVM_DEBUG(dbgs() << "Finded Simd CF Else condition is not opposite to "
                           "the If condition:\n"
                        << *ElseCond << "\n"
                        << *ElseBranch);
      return nullptr;
    }
    LLVM_DEBUG(dbgs() << "Find Simd CF Else branch and condition:\n"
                      << *ElseCond << "\n"
                      << *ElseBranch);
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

    auto IfElseRegion = std::make_unique<SimdCFRegionBase>(
        IfElseEntry, IfElseEnd, &getAnalysis<RegionInfoPass>().getRegionInfo(),
        &getAnalysis<DominatorTreeWrapperPass>().getDomTree(), nullptr);
    IfElseRegion->verifyRegion();
    LLVM_DEBUG(dbgs() << "Finded IfElseRegion '" << IfElseRegion->getNameStr()
                      << "':\n";
               IfElseRegion->print(dbgs()));

    BasicBlock *Exit = getIfExitFromElseBranch(*ElseBranch);
    if (!Exit)
      return nullptr;

    // IfThen* post dominate Exit && Entry post dominate Exit
    if (!PDT || !PDT->dominates(Exit, IfThenEnd) ||
        !PDT->dominates(Exit, Entry) || !PDT->dominates(Exit, IfElseEnd))
      return nullptr;

    if (!(analizeInsts(IfThenRegion.get(), Cond) ||
          analizeInsts(IfElseRegion.get(), Cond))) {
      LLVM_DEBUG(dbgs() << "Filed to find predicated instructions for"
                        << *Cond);
      return nullptr;
    }

    if (getSimdSize(cast<CallInst>(Cond)) <= 1)
      return nullptr;

    LLVM_DEBUG(dbgs() << "Find Simd CF IfEnd BB: " << Exit->getName() << '\n');
    return std::make_unique<SimdCFIfRegion>(
        Entry, IfThenRegion.release(), IfElseRegion.release(), Exit,
        &getAnalysis<RegionInfoPass>().getRegionInfo(),
        &getAnalysis<DominatorTreeWrapperPass>().getDomTree());
  }

  BasicBlock *Exit = getIfExitFromIfBranch(*Branch);
  if (!Exit)
    return nullptr;

  // IfThenEnd post dominate exit && Entry post dominate exit
  if (!PDT || !PDT->dominates(Exit, Entry) || !PDT->dominates(Exit, IfThenEnd))
    return nullptr;

  // Check there is any predicated(select) instructions
  // in region found.
  if (!analizeInsts(IfThenRegion.get(), Cond)) {
    LLVM_DEBUG(dbgs() << "Filed to find predicated instructions for" << *Cond);
    return nullptr;
  }

  LLVM_DEBUG(dbgs() << "Find Simd CF IfEnd BB: " << Exit->getName() << '\n');
  return std::make_unique<SimdCFIfRegion>(
      Entry, IfThenRegion.release(), nullptr, Exit,
      &getAnalysis<RegionInfoPass>().getRegionInfo(),
      &getAnalysis<DominatorTreeWrapperPass>().getDomTree());
}

std::pair<BranchInst *, Value *>
GenXPredToSimdCF::findSimdCFLoopBranchAndCondition(const Loop &L) {
  LLVM_DEBUG(dbgs() << "Trying to find Simd CF branch and condition in:\n"
                    << L.getName() << "\n");
  auto *ExitingBB = L.getExitingBlock();
  if (!ExitingBB) {
    LLVM_DEBUG(dbgs() << "Didn't find exiting BB for this loop!\n");
    return std::make_pair(nullptr, nullptr);
  }
  LLVM_DEBUG(dbgs() << "Loop exiting BB: '" << ExitingBB->getName() << "'\n");
  return findSimdCFBranchAndCondition(*ExitingBB);
}

SimdCFRegionPtr GenXPredToSimdCF::tryMatchLoop(BasicBlock &BB) {
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
  LLVM_DEBUG(dbgs() << "Find Simd CF Loop branch and condition:\n"
                    << *Cond << "\n"
                    << *Branch);
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

// TODO: rewrite this algo to use Region as arg instead of Fuction to allow
// reuse it for matching subregions
SimdCFRegionsT GenXPredToSimdCF::findSimdCFRegions(Function &F) {
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
                          << "'\n";
                   Match->print(dbgs()));

        for (auto &MBB : *Match->Blocks()) {
          Visited.insert(MBB);
          LLVM_DEBUG(dbgs()
                     << "Marking BB '" << MBB->getName() << "' as visited\n");
        }
        LLVM_DEBUG(dbgs() << "Add -> SimdCFRegion:\n"; Match->print(dbgs()));
        Regions.push_back(std::move(Match));
      } else
        LLVM_DEBUG(dbgs() << "Couldn't match Simd CF on BB '" << BB.getName()
                          << "'\n");
    } else {
      if (SimdCFRegionPtr Match = tryMatchLoop(BB)) {
        LLVM_DEBUG(dbgs() << "Matched Simd CF Loop on BB '" << BB.getName()
                          << "'\n";
                   Match->print(dbgs()));
        for (auto &MBB : *Match->Blocks()) {
          Visited.insert(MBB);
          LLVM_DEBUG(dbgs()
                     << "Marking BB '" << MBB->getName() << "' as visited\n");
        }
        LLVM_DEBUG(dbgs() << "Add -> SimdCFRegion:\n"; Match->print(dbgs()));
        Regions.push_back(std::move(Match));
      } else
        LLVM_DEBUG(dbgs() << "Couldn't match Simd CF on BB '" << BB.getName()
                          << "'\n");
    }

    Visited.insert(&BB);
    LLVM_DEBUG(dbgs() << "Marking BB '" << BB.getName() << "' as visited\n");
  }

  return Regions;
}

Value *GenXPredToSimdCF::generateJoin(BasicBlock *JoinBlock) {
  auto *F = JoinBlock->getParent();
  Value *EM = getEMAddr(F);

  auto SimdWidth = getSimdSize(Mask->getType());
  IRBuilder<> Builder(JoinBlock, JoinBlock->begin());

  // Fix execution mask in after-then branch
  auto *OldEM = Builder.CreateLoad(IGCLLVM::getNonOpaquePtrEltTy(EM->getType()),
                                   EM, false /*isVolatile*/, EM->getName());

  auto *RMAddr = getRMAddr(SimdWidth);
  auto *RM =
      Builder.CreateLoad(IGCLLVM::getNonOpaquePtrEltTy(RMAddr->getType()),
                         RMAddr, false /*isVolatile*/, RMAddr->getName());

  Type *Tys[] = {OldEM->getType(), RM->getType()};
  auto *M = JoinBlock->getModule();
  auto *JoinDecl = GenXIntrinsic::getGenXDeclaration(
      M, GenXIntrinsic::genx_simdcf_join, Tys);
  Value *Args[] = {OldEM, RM};

  CallInst *Join = Builder.CreateCall(JoinDecl, Args, "join");
  Join->setTailCall();
  Join->setConvergent();
  auto *NewEM = Builder.CreateExtractValue(Join, 0, "join.extractem");
  Builder.CreateStore(NewEM, EM, false /*isVolatile*/);
  auto *JoinCond = Builder.CreateExtractValue(Join, 1, "join.extractcond");
  return JoinCond;
}

void GenXPredToSimdCF::saveGotoEMRM(CallInst *Goto) {
  auto SimdWidth = getSimdSize(Mask->getType());
  IRBuilder<> Builder(Goto->getParent()->getTerminator());

  // Save EM-value
  Value *EM = getEMAddr(Goto->getFunction());
  auto *NewGotoEM = Builder.CreateExtractValue(Goto, 0, "goto.extractem");
  Builder.CreateStore(NewGotoEM, EM, false /*isVolatile*/);

  // Save RM-value
  Value *GotoRMAddr = getRMAddr(SimdWidth);
  auto *RmTy = IGCLLVM::getNonOpaquePtrEltTy(GotoRMAddr->getType());
  auto *NewGotoRM = Builder.CreateExtractValue(Goto, 1, "goto.extractrm");
  IGC_ASSERT(GotoRMAddr->getType()->isPointerTy());

  if (RmTy != cast<Instruction>(NewGotoRM)->getType()) {
    NewGotoRM = Builder.CreateTruncOrBitCast(NewGotoRM, RmTy);
  }
  Builder.CreateStore(NewGotoRM, GotoRMAddr, false /*isVolatile*/);
}

// For goto it is needed to make negation of branch
// by-default for genx.any.
void GenXPredToSimdCF::generateOppositeMask(Type *RmTy) {
  //  If we already will swap it - do nothing
  if (NeedSwap)
    return;

  // TODO: search already exist not-mask
  // TODO: support genx.all here
  if (auto *CMP = dyn_cast<CmpInst>(Mask))
    CMP->swapOperands();
  else {
    IRBuilder<> Builder(cast<Instruction>(Mask));
    Mask = Builder.CreateXor(Mask, Constant::getAllOnesValue(RmTy),
                             Mask->getName() + ".not");
  }
}

// Called for if-then and also for loop patterns
void GenXPredToSimdCF::generateGoto(BranchInst *Br) {
  IRBuilder<> Builder(Br);

  auto SimdWidth = getSimdSize(Mask->getType());
  auto *F = Br->getFunction();
  Value *EM = getEMAddr(F);
  Instruction *OldGotoEM =
      Builder.CreateLoad(IGCLLVM::getNonOpaquePtrEltTy(EM->getType()), EM,
                         false /*isVolatile*/, EM->getName());

  Value *GotoRMAddr = getRMAddr(SimdWidth);
  auto *RmTy = IGCLLVM::getNonOpaquePtrEltTy(GotoRMAddr->getType());

  Instruction *OldGotoRM = Builder.CreateLoad(
      IGCLLVM::getNonOpaquePtrEltTy(GotoRMAddr->getType()), GotoRMAddr,
      false /*isVolatile*/, GotoRMAddr->getName());

  generateOppositeMask(RmTy);

  if (RmTy != Mask->getType()) {
    Mask = Builder.CreateTrunc(Mask, RmTy);
  }

  // Create Goto instructions
  auto *M = Br->getModule();
  Type *GotoTys[] = {OldGotoEM->getType(), OldGotoRM->getType()};
  auto *GotoDecl = GenXIntrinsic::getGenXDeclaration(
      M, GenXIntrinsic::genx_simdcf_goto, GotoTys);
  Value *GotoArgs[] = {OldGotoEM, OldGotoRM, Mask};
  CallInst *Goto = Builder.CreateCall(GotoDecl, GotoArgs, "goto");
  Goto->setTailCall();
  Goto->setConvergent();

  saveGotoEMRM(Goto);

  auto *LoweredGotoCond =
      Builder.CreateExtractValue(Goto, 2, "goto.extractcond");

  // Branch prepare
  Br->setCondition(LoweredGotoCond);
}

// generate goto(em, zero, zero) to ebable all disabled chanels
// * NewEM = OldEM & (zero) == zero
// * NewRM = OldRM | (OldEM & ~(zero & OldEM == zero)) == OldRM | (OldEM &
// allOne) == OldRM | OldEM == (enable all)
// * Cond = !any(zero) == !false == true
void GenXPredToSimdCF::generateThenGoto(BranchInst *Br, BasicBlock *Join) {
  IRBuilder<> Builder(Br);

  auto SimdWidth = getSimdSize(Mask->getType());
  auto *M = Br->getModule();
  Value *OldGotoEM = getEMValue(Br->getParent());

  auto *RmTy = IGCLLVM::getNonOpaquePtrEltTy(getRMAddr(SimdWidth)->getType());

  Type *GotoTys[] = {OldGotoEM->getType(), RmTy};
  auto *RMZeroinit = Constant::getNullValue(RmTy);
  auto *GotoDecl = GenXIntrinsic::getGenXDeclaration(
      M, GenXIntrinsic::genx_simdcf_goto, GotoTys);
  Value *GotoArgs[] = {OldGotoEM, RMZeroinit, RMZeroinit};
  CallInst *Goto = Builder.CreateCall(GotoDecl, GotoArgs, "goto");
  Goto->setTailCall();
  Goto->setConvergent();

  // Save em/rm
  saveGotoEMRM(Goto);

  auto *LoweredGotoCond =
      Builder.CreateExtractValue(Goto, 2, "goto.extractcond");

  Builder.CreateCondBr(LoweredGotoCond, Join, Join);
  Br->eraseFromParent();
}

void GenXPredToSimdCF::insertIfGotoJoin(SimdCFIfRegion *R) {
  auto *Br = R->getIfSimdBranch();
  auto *M = Br->getModule();
  // Will be removed:
  OldCond = R->getIfSimdCondition();
  JP = R->getIfThenRegion()->getEntry();
  NeedSwap = R->needSwap();

  generateGoto(Br);

  Function *F = Br->getFunction();
  auto *IfThenExit = R->getIfThenRegion()->getExit();
  StringRef JoinBlockPrefix = IfThenExit->getName();
  JoinBlockPrefix.consume_back(".then");
  Twine JoinBlockName = JoinBlockPrefix + ".afterthen";
  auto *JoinBlock = BasicBlock::Create(M->getContext(), JoinBlockName, F,
                                       IfThenExit->getNextNode());
  auto ThenId = R->getIdThen();
  Br->setSuccessor(1 - ThenId, JoinBlock);
  if (NeedSwap)
    Br->swapSuccessors();

  auto *IfThenExitBr = IfThenExit->getTerminator();
  IGC_ASSERT(isa<BranchInst>(IfThenExitBr));
  IfThenExitBr->setSuccessor(0, JoinBlock);

  // Additional fix for else-branch
  if (R->hasElse()) {
    auto *ElseExit = R->getIfThenRegion()->getExit();
    Br = cast<BranchInst>(ElseExit->getTerminator());
    auto *IfEndBB = IfThenExitBr->getSuccessor(1);
    // 1-th: generate goto(EM, zeroinitializer, zeroinitializer) in the end of
    // then-block
    generateThenGoto(Br, JoinBlock);

    for (auto &PHI : IfEndBB->phis()) {
      PHI.replaceIncomingBlockWith(IfThenExit, JoinBlock);
    }
  }
  JoinBlocks[R] = JoinBlock;

  auto *JoinCond = generateJoin(JoinBlock);

  IRBuilder<> Builder(JoinBlock, JoinBlock->end());
  if (R->hasElse()) {
    BasicBlock *JoinCondFalse =
        R->hasElse() ? R->getIfElseRegion()->getEntry() : R->getExit();
    Builder.CreateCondBr(JoinCond, R->getExit(), JoinCondFalse);
  } else
    Builder.CreateBr(R->getExit());
}

void GenXPredToSimdCF::insertLoopGotoJoin(SimdCFLoopRegion *R) {
  auto *Br = R->getSimdBranch();
  auto *M = Br->getModule();
  // Will be removed:
  OldCond = R->getSimdCondition();
  JP = R->getLoopHead();
  NeedSwap = R->needSwap();

  generateGoto(Br);

  Function *F = Br->getFunction();

  auto *LastBlock = R->getSimdBranch()->getParent();
  StringRef JoinBlockPrefix = LastBlock->getName();

  Twine JoinBlockName = JoinBlockPrefix + ".join";
  auto *JoinBlock = BasicBlock::Create(M->getContext(), JoinBlockName, F,
                                       LastBlock->getNextNode());

  IGC_ASSERT(isa<BranchInst>(Br));
  Br->setSuccessor(R->getIdExit(), JoinBlock);
  if (NeedSwap)
    Br->swapSuccessors();

  // Additional fix for else-branch
  JoinBlocks[R] = JoinBlock;
  generateJoin(JoinBlock);

  IRBuilder<> Builder(JoinBlock, JoinBlock->end());
  Builder.CreateBr(R->getExit());

  LLVM_DEBUG(dbgs() << "Join block\n" << *JoinBlock);
}

void GenXPredToSimdCF::replaceUses(Use *Ui, bool BuildEM) {
  auto *Use = Ui->getUser();
  auto *Inst = cast<Instruction>(Use);
  LLVM_DEBUG(dbgs() << "replaceUses: for instruction " << *Inst);
  auto OpNo = Ui->getOperandNo();
  auto *CondTy = Inst->getOperand(OpNo)->getType();
  IGC_ASSERT(CondTy == Ui->get()->getType());
  IRBuilder<> Builder(Inst);

  ToErase.insert(Inst->getOperand(OpNo));

  Value *CurrMask;
  if (BuildEM) {
    CurrMask = getEMValue(Inst->getParent());
  } else {
    auto SimdWidth = getSimdSize(Mask->getType());
    // And class-member JP
    auto *RMAddr = getRMAddr(SimdWidth);
    CurrMask =
        Builder.CreateLoad(IGCLLVM::getNonOpaquePtrEltTy(RMAddr->getType()),
                           RMAddr, false /*isVolatile*/, RMAddr->getName());
  }
  if (CondTy != CurrMask->getType()) {
    // if sizes are equal - creare bitcast or trunk
    auto *PredRgnTy = cast<IGCLLVM::FixedVectorType>(CurrMask->getType());
    auto *PredTy = cast<IGCLLVM::FixedVectorType>(CondTy);

    if (PredTy->getNumElements() == PredRgnTy->getNumElements()) {
      CurrMask = Builder.CreateSExtOrTrunc(CurrMask, PredTy,
                                           CurrMask->getName() + ".castpred");
    } else {
      // we may generate only trunk size, not extend
      IGC_ASSERT(PredTy->getNumElements() < PredRgnTy->getNumElements());
      auto *M = Inst->getModule();
      auto *OffsetTy = Builder.getInt16Ty();
      auto *RdPredRgnFunc = GenXIntrinsic::getAnyDeclaration(
          M, GenXIntrinsic::genx_rdpredregion, {PredTy, PredRgnTy, OffsetTy});
      Value *Args[] = {CurrMask, Builder.getInt32(0)};
      CurrMask = Builder.CreateCall(RdPredRgnFunc, Args);
    }
  }
  Inst->setOperand(OpNo, CurrMask);
}

bool GenXPredToSimdCF::TryReplace(SimdCFRegionBase *Reg, Use *Ui, bool IsEm) {
  if (!Reg)
    return false;
  auto *Inst = cast<Instruction>(Ui->getUser());
  auto *Par = Inst->getParent();
  if (isa<SelectInst>(Inst) && (Ui->getOperandNo() == 0)) {
    auto Exit = Reg->getExit();
    if (Reg->getEntry() == Exit && Exit == Par) {
      LLVM_DEBUG(dbgs() << "Replace Then-use in " << (IsEm ? "EM" : "RM")
                        << ": " << *Inst);
      replaceUses(&*Ui, IsEm);
      return true;
    } else {
      if (find(*Reg->Blocks(), Inst->getParent()) != Reg->Blocks()->end()) {
        LLVM_DEBUG(dbgs() << "Replace Then-use in " << (IsEm ? "EM" : "RM")
                          << ": " << *Inst);
        replaceUses(&*Ui, IsEm);
        return true;
      }
    }
  }
  LLVM_DEBUG(dbgs() << " Failed to replace Else-use in " << (IsEm ? "EM" : "RM")
                    << ": " << *Inst);
  return false;
}

void GenXPredToSimdCF::removeMask(SimdCFIfRegion *R) {
  auto *CurrMask = R->getMask();
  while (isa<CastInst>(CurrMask))
    CurrMask = cast<Instruction>(CurrMask)->getOperand(0);

  ToErase.insert(CurrMask);

  LLVM_DEBUG(dbgs() << "Trying to remove selects with mask: " << *CurrMask);
  auto *ThenReg = R->getIfThenRegion();
  auto *ElseReg = R->getIfElseRegion();
  if (NeedSwap)
    std::swap(ElseReg, ThenReg);

  SmallVector<Use *, 4> Uses;
  for (auto Ui = CurrMask->use_begin(), Ue = CurrMask->use_end(); Ui != Ue;
       ++Ui) {
    Uses.push_back(&*Ui);
  }

  // Iterate by index to have posibility to insert in back
  for (unsigned i = 0; i < Uses.size(); ++i) {
    // auto *Ui : Uses) {
    auto *Ui = Uses[i];
    Value *Use = Ui->getUser();
    auto *Inst = dyn_cast<Instruction>(Use);
    if (!Inst)
      continue;
    // TODO: Do not touch only join-phi's all another fits
    if (isa<PHINode>(Inst))
      continue;

    if (TryReplace(ThenReg, Ui, false))
      continue;

    if (TryReplace(ElseReg, Ui, true))
      continue;

    if (isa<CastInst>(Inst)) {
      for (auto CUi = Inst->use_begin(), CUe = Inst->use_end(); CUi != CUe;
           ++CUi)
        Uses.push_back(&*CUi);
      continue;
    }
  }
}

void GenXPredToSimdCF::removeMachMask(SimdCFRegionBase *R) {
  auto *CurrMask = R->getMask();
  auto *EBB = R->getExit();
  while (isa<CastInst>(CurrMask))
    CurrMask = cast<Instruction>(CurrMask)->getOperand(0);

  ToErase.insert(CurrMask);

  // Match pattern:
  //    goto (EM, RM, mask_any)
  //    mask_any = genx.any/all (condition)
  //    condition = cmp ne MASK, zero
  // ->
  //    ASSERT( MASK is PHI )
  // each cmp-successors of phi:
  //    NOT = cmp eq MASK
  // Replace all uses of NOT to load EM

  // Here real MASK, not genx.any 'mask'
  Instruction *FullMask = nullptr;
  if (auto *CMP = dyn_cast<CmpInst>(CurrMask)) {
    // is cmp && ne && getOperand(1) == zero) {
    if (CMP->getPredicate() == CmpInst::ICMP_NE &&
        isa<ConstantAggregateZero>(CMP->getOperand(1)))
      FullMask = dyn_cast<Instruction>(CMP->getOperand(0));
  }
  if (!FullMask || !dyn_cast<PHINode>(FullMask))
    return;

  LLVM_DEBUG(dbgs() << "Remove Mach mask for: \n" << *FullMask->getParent());

  SmallVector<Use *, 4> MaskUses;
  for (auto Ui = FullMask->use_begin(), Ue = FullMask->use_end(); Ui != Ue;
       ++Ui) {
    if (Ui->getUser() != CurrMask) {
      MaskUses.push_back(&*Ui);
      LLVM_DEBUG(dbgs() << "Find separate mask " << *Ui);
    }
  }

  // TODO! We must make an analisys, that current use/mask do not change after
  // replacement
  // Current implementation may generate bugs

  for (unsigned i = 0; i < MaskUses.size(); ++i) {
    auto *Ui = MaskUses[i];
    Value *Use = Ui->getUser();
    auto *Inst = dyn_cast<Instruction>(Use);
    if (!Inst)
      continue;
    auto IID = GenXIntrinsic::getGenXIntrinsicID(Inst);
    if (IID == GenXIntrinsic::genx_simdcf_goto ||
        IID == GenXIntrinsic::genx_simdcf_join)
      continue;
    // TODO: Do not touch only join-phi's all another fits
    if (Inst->getParent() == EBB)
      continue;
    if (auto *CMP = dyn_cast<CmpInst>(Inst)) {
      if (CMP->getPredicate() == CmpInst::ICMP_EQ) {
        for (auto CUi = Inst->use_begin(), CUe = Inst->use_end(); CUi != CUe;
             ++CUi)
          MaskUses.push_back(&*CUi);
      }
      continue;
    }
    if (isa<PHINode>(Inst)) {
      for (auto CUi = Inst->use_begin(), CUe = Inst->use_end(); CUi != CUe;
           ++CUi)
        MaskUses.push_back(&*CUi);
      continue;
    }
    if (Inst->isBitwiseLogicOp()) {
      continue;
    }

    if (llvm::find(*R->Blocks(), Inst->getParent()) != R->Blocks()->end()) {
      LLVM_DEBUG(dbgs() << "Replace Loop-use in (EM): " << *Inst);
      replaceUses(&*Ui, true);
      continue;
    }
    if (isa<CastInst>(Inst)) {
      for (auto CUi = Inst->use_begin(), CUe = Inst->use_end(); CUi != CUe;
           ++CUi)
        MaskUses.push_back(&*CUi);
      continue;
    }
  }
}

void GenXPredToSimdCF::removeMask(SimdCFLoopRegion *R) {
  auto *CurrMask = R->getMask();
  while (isa<CastInst>(CurrMask))
    CurrMask = cast<Instruction>(CurrMask)->getOperand(0);

  removeMachMask(R);

  LLVM_DEBUG(dbgs() << "Trying to remove selects with mask: " << *CurrMask);

  auto *EBB = R->getExit();

  SmallVector<Use *, 4> Uses;
  for (auto Ui = CurrMask->use_begin(), Ue = CurrMask->use_end(); Ui != Ue;
       ++Ui) {
    Uses.push_back(&*Ui);
  }
  // Additional check, that EM calculated just before goto,
  // otherwise it is not correct to replace this use to current
  // EM. Example:
  //     BB_
  //         calculation
  //         mask = ...
  //         result = select (mask) % 1, %2
  //         goto mask BB_
  //   must be transformed to
  //         mask_pre = ...
  //     BB_
  //         phi_mask = phi [in mask_pre, back_age mask]
  //         calculation
  //         result = select (phi_mask) %1, %2
  // or better llvm.genx.wrregioni.* (%1, %2, %phi_mask)
  //         mask = ...
  //         goto mask BB_
  // in this case we may predicate all select under goto's mask
  // otherwise it will executed in all lines under predicate, which drop perf
  //  wrregioni

  // Iterate by index to have posibility to insert in back
  for (unsigned i = 0; i < Uses.size(); ++i) {
    // auto *Ui : Uses) {
    auto *Ui = Uses[i];
    Value *Use = Ui->getUser();
    auto *Inst = dyn_cast<Instruction>(Use);
    if (!Inst)
      continue;

    auto IID = GenXIntrinsic::getGenXIntrinsicID(Inst);
    if (IID == GenXIntrinsic::genx_simdcf_goto ||
        IID == GenXIntrinsic::genx_simdcf_join)
      continue;
    // TODO: Do not touch only join-phi's all another fits
    if (isa<PHINode>(Inst) && Inst->getParent() == EBB)
      continue;
    if (isa<PHINode>(Inst)) {
      for (auto CUi = Inst->use_begin(), CUe = Inst->use_end(); CUi != CUe;
           ++CUi)
        Uses.push_back(&*CUi);
      continue;
    }
    if (Inst->isBitwiseLogicOp()) {
      continue;
    }

    // TODO: support here another instructions
    //    in select-inst -replace only predicate incomes
    if ((llvm::find(*R->Blocks(), Inst->getParent()) != R->Blocks()->end()) &&
        (isa<SelectInst>(Inst) && (Ui->getOperandNo() == 0))) {
      LLVM_DEBUG(dbgs() << "Replace Loop-use in (RM): " << *Inst);
      replaceUses(&*Ui, true);
      continue;
    }
    if (isa<CastInst>(Inst)) {
      for (auto CUi = Inst->use_begin(), CUe = Inst->use_end(); CUi != CUe;
           ++CUi)
        Uses.push_back(&*CUi);
      continue;
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
bool checkSelects(PHINode &PHINode, SimdCFIfRegion *R,
                  llvm::SelectInst *&ifThenSelect, Value *&data) {
  IGC_ASSERT(PHINode.getNumIncomingValues() == 2);
  auto *IV1 = PHINode.getIncomingValue(0);
  auto *IV2 = PHINode.getIncomingValue(1);
  // R.getIfThen vs R.getIfElse
  if (isa<SelectInst>(IV1) && isa<SelectInst>(IV2)) {
    auto *V1 = cast<SelectInst>(IV1);
    auto *V2 = cast<SelectInst>(IV2);

    // TODO: support check for each block in region
    IGC_ASSERT(R->getIfThenRegion()->getEntry() ==
               R->getIfThenRegion()->getExit());
    IGC_ASSERT(R->getIfElseRegion()->getEntry() ==
               R->getIfElseRegion()->getExit());

    // Just check that this selects from needed bb-s
    ifThenSelect =
        V1->getParent() == R->getIfThenRegion()->getEntry() ? V1 : V2;
    IGC_ASSERT(V1 != V2);
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
void GenXPredToSimdCF::fixPHIs(SimdCFIfRegion *R) {
  auto *EBB = R->getExit();
  if (!R->hasElse()) {
    LLVM_DEBUG(dbgs() << "Phi fixing for: " << EBB->getName());
    Instruction *InsertPoint = JoinBlocks[R]->getFirstNonPHI();
    IGC_ASSERT(JoinBlocks[R] != EBB);
    LLVM_DEBUG(dbgs() << "Insert to : " << InsertPoint->getName() << "\nEBB: "
                      << EBB->getName() << "  = " << JoinBlocks[R]->getName());

    SmallVector<PHINode *, 3> Phis;
    for (auto &PHINode : EBB->phis()) {
      Phis.push_back(&PHINode);
    }
    for (auto *PHINode : Phis) {
      PHINode->moveBefore(InsertPoint);
      LLVM_DEBUG(dbgs() << *PHINode);
      if (JPSplit)
        for (unsigned i = 0, e = PHINode->getNumIncomingValues(); i != e; i++) {
          auto *IB = PHINode->getIncomingBlock(i);
          if (IB == OldCondBB)
            PHINode->setIncomingBlock(i, JPSplit);
        }
    }
    return;
  }

  LLVM_DEBUG(dbgs() << "EBB: " << EBB->getName());
  for (auto &PHINode : EBB->phis()) {
    LLVM_DEBUG(dbgs() << "PHI in EBB: " << PHINode.getName());
    // Save all eges ids for re-fix them with new phi-node
    std::vector<unsigned> PhiIncomingID;
    for (unsigned i = 0, e = PHINode.getNumIncomingValues(); i != e; i++) {
      auto *IV = PHINode.getIncomingValue(i);
      auto *IB = PHINode.getIncomingBlock(i);
      if (R->getIfThenRegion()->getExit() == IB)
        PHINode.addIncoming(IV, JoinBlocks[R]);

      if (JPSplit && IB == OldCondBB)
        PHINode.setIncomingBlock(i, JPSplit);

      if (JoinBlocks[R] == PHINode.getIncomingBlock(i))
        PhiIncomingID.push_back(i);
    }

    SelectInst *ifThenSelect = nullptr;
    Value *data = nullptr;
    if (!checkSelects(PHINode, R, ifThenSelect, data)) {
      IGC_ASSERT_UNREACHABLE();
      return;
    }

    SelectInst *VForFix = dyn_cast<SelectInst>(ifThenSelect);
    // Find edge from after-then block

    if (Constant *ConstOperandV1 = dyn_cast<Constant>(data)) {
      // for all JoinBlocks[&R] create phi with const input in after-then block
      IRBuilder<> Builder(&JoinBlocks[R]->front());
      auto Ty = ConstOperandV1->getType();
      auto *PHI = Builder.CreatePHI(Ty, 2, "afterthenPHI");
      for (auto it = pred_begin(JoinBlocks[R]), et = pred_end(JoinBlocks[R]);
           it != et; ++it) {
        if (R->getIfThenRegion()->getExit() == *it)
          PHI->addIncoming(VForFix, R->getIfThenRegion()->getExit());
        else
          PHI->addIncoming(ConstOperandV1, *it);
      }
      // Restore incoming id
      for (auto &IncID : PhiIncomingID) {
        PHINode.setIncomingValue(IncID, PHI);
        PHINode.setIncomingBlock(IncID, JoinBlocks[R]);
      }
    } else {
      // TODO support non-consts
      LLVM_DEBUG(dbgs() << "Error! Not constant inst! Unsupported!\n");
      IGC_ASSERT_UNREACHABLE();
    }
  }
}

void GenXPredToSimdCF::fixPHIs(SimdCFLoopRegion *R) {
  auto *EBB = R->getExit();
  auto *LoopEnd = R->getSimdBranch()->getParent();

  LLVM_DEBUG(dbgs() << "Phi fixing for: " << EBB->getName() << "\n");
  IGC_ASSERT(JoinBlocks[R] != EBB);
  LLVM_DEBUG(dbgs() << "Insert to : "
                    << JoinBlocks[R]->getFirstNonPHI()->getName()
                    << "\n for: " << EBB->getName()
                    << "\n Join: " << JoinBlocks[R]->getName() << "\n");

  for (auto &PHINode : EBB->phis()) {
    for (unsigned i = 0, e = PHINode.getNumIncomingValues(); i != e; i++) {
      auto *IB = PHINode.getIncomingBlock(i);
      if (LoopEnd == IB)
        PHINode.setIncomingBlock(i, JoinBlocks[R]);
    }
    LLVM_DEBUG(dbgs() << PHINode);
  }
}

bool GenXPredToSimdCF::transform(SimdCFIfRegion *R) {
  LLVM_DEBUG(dbgs() << "IfRegion: Insert if goto\n");
  insertIfGotoJoin(R);

  OldCondBB = OldCond->getParent();

  LLVM_DEBUG(dbgs() << " For: " << OldCondBB->getParent()->getName() << "\n"
                    << *OldCondBB);

  JPSplit = R->SplitBlock(OldCond);

  LLVM_DEBUG(dbgs() << "splitting block\n : " << OldCondBB->getName()
                    << " created " << JPSplit->getName() << "\n");

  ToErase.insert(OldCond);

  LLVM_DEBUG(dbgs() << "IfRegion: Fixing PHIs\n");
  fixPHIs(R);

  LLVM_DEBUG(dbgs() << "IfRegion: Removing mask\n");
  removeMask(R);

  return true;
}

bool GenXPredToSimdCF::transform(SimdCFLoopRegion *R) {
  LLVM_DEBUG(dbgs() << "LoopRegion: Insert if goto\n");
  insertLoopGotoJoin(R);

  LLVM_DEBUG(dbgs() << "LoopRegion: Fixing PHIs\n");
  fixPHIs(R);

  if (SimdCFRMLoopMask) {
    LLVM_DEBUG(dbgs() << "LoopRegion: Removing mask\n");
    removeMask(R);
  }

  ToErase.insert(OldCond);

  return true;
}

bool GenXPredToSimdCF::transform(SimdCFRegion *R) {
  Mask = R->getMask();

  if (getSimdSize(Mask->getType()) <= 1)
    return false;

  if (dyn_cast_or_null<SimdCFIfRegion>(R))
    return transform(cast<SimdCFIfRegion>(R));

  if (dyn_cast_or_null<SimdCFLoopRegion>(R))
    return transform(cast<SimdCFLoopRegion>(R));

  return false;
}

static bool hasStackCall(const Module &M) {
  return std::any_of(M.begin(), M.end(),
                     [](const auto &F) { return vc::requiresStackCall(&F); });
}

bool GenXPredToSimdCF::runOnFunction(Function &F) {
  if (hasStackCall(*F.getParent()))
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
      LLVM_DEBUG(dbgs() << "Not verified SimdCFRegion:\n"; R->print(dbgs()));
    } else {
      LLVM_DEBUG(dbgs() << "Verified SimdCFRegion:\n"; R->print(dbgs()));
      ChangeCurrReg = transform(R.get());
    }
    // If apply - the pass changes dominate structure
    if (ChangeCurrReg) {
      DT->recalculate(F);
      PDT->recalculate(F);
    }
    for (auto *I : ToErase) {
      LLVM_DEBUG(dbgs() << *I);
      if (auto *Inst = dyn_cast<Instruction>(I); Inst && Inst->use_empty()) {
        LLVM_DEBUG(dbgs() << "Erased \n");
        Inst->eraseFromParent();
      } else {
        LLVM_DEBUG(dbgs() << "NOT Erased !\n");
      }
    }

    Changed |= ChangeCurrReg;
    Mask = nullptr;
    JP = nullptr;
    JPSplit = nullptr;
    OldCondBB = nullptr;
    NeedSwap = false;
    ToErase.clear();
  }

  LLVM_DEBUG(if (Regions.empty()) dbgs() << "Find no SimdCFRegions\n";
             else { dbgs() << "SimdCFRegions applyed\n"; } dbgs()
             << "For function " << F.getName() << "\n");

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
INITIALIZE_PASS_END(GenXPredToSimdCF, DEBUG_TYPE, DEBUG_TYPE, false, false)

namespace llvm {
FunctionPass *createGenXPredToSimdCFPass() {
  initializeGenXPredToSimdCFPass(*PassRegistry::getPassRegistry());
  return new GenXPredToSimdCF();
}
} // namespace llvm
