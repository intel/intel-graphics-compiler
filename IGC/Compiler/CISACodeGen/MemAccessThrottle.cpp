/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/MemAccessThrottle.hpp"

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/MemOptUtils.h"
#include "Compiler/CISACodeGen/TranslationTable.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "llvmWrapper/IR/Instructions.h"
#include "common/igc_regkeys.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

#include <optional>
#include <string>

using namespace llvm;
using namespace IGC;

static constexpr uint64_t MinAccessBytes = 16;   // one d32x4 message
static constexpr uint32_t LeaderPauseUnits = 32; // 32 saturates the tm0.4 field at ~992 clocks

// Limits the rate of outgoing writes on CRI, where two devices writing to each
// other's memory at the same time collapses the write bandwidth.
//
// Matches a store whose value was just loaded back from an earlier store in the
// same block, i.e. a staged copy out to a second buffer, and appends the
// in-kernel throttle from the CRI peer2peer write guide so that one pause
// throttles the whole group:
//
//   if (localThreadId == 0) pause(32);
//   barrier();
//
// This cannot be a default: the compiler does not know whether a global store
// targets the own device, a peer or the host.
class MemAccessThrottle : public FunctionPass {
public:
  static char ID;
  MemAccessThrottle();

  bool runOnFunction(Function &F) override;

  StringRef getPassName() const override { return "MemAccessThrottle"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  void collectSites(BasicBlock &BB, SmallVectorImpl<Instruction *> &Sites);

  std::optional<AStoreInst> findReloadedStore(const ALoadInst &L);

  Value *createLeaderPredicate(Function &F);

  // The nearest point at or after Site that the whole work-group reaches the
  // same number of times, or null if there is none.
  Instruction *findThrottlePoint(Instruction *Site, WIAnalysisRunner &WI, PostDominatorTree &PDT, LoopInfo &LI) const;

  bool hasWorkgroupUniformTripCount(Loop *L, WIAnalysisRunner &WI) const;

  // Every thread of the work-group reaches BB the same number of times per
  // iteration of its loop.
  bool isWorkgroupUniformlyReached(BasicBlock *BB, WIAnalysisRunner &WI, PostDominatorTree &PDT) const;

  bool isWorkgroupUniform(Value *V, WIAnalysisRunner &WI) const;
  bool isWorkgroupUniform(Value *V, WIAnalysisRunner &WI, SmallPtrSetImpl<Value *> &Visited, unsigned Depth) const;

  bool isR0GroupId(const Value *V) const;

  uint64_t accessBytes(Type *Ty) const { return m_DL->getTypeStoreSize(Ty); }

  const DataLayout *m_DL = nullptr;
  Argument *m_R0 = nullptr;
  SymbolicEvaluation m_symEval;
};

#define PASS_FLAG "igc-mem-access-throttle"
#define PASS_DESCRIPTION "IGC Memory Access Throttle"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MemAccessThrottle, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(MemAccessThrottle, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char MemAccessThrottle::ID = 0;

MemAccessThrottle::MemAccessThrottle() : FunctionPass(ID) {
  initializeMemAccessThrottlePass(*PassRegistry::getPassRegistry());
}

void MemAccessThrottle::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<MetaDataUtilsWrapper>();
}

bool IGC::doMemAccessThrottle(const CodeGenContext *CGC) {
  if (CGC->type != ShaderType::OPENCL_SHADER)
    return false;

  if (CGC->platform.getPlatformInfo().eProductFamily != IGFX_CRI)
    return false;

  auto *OCLCtx = static_cast<const OpenCLProgramContext *>(CGC);
  const TriboolFlag Override = static_cast<TriboolFlag>(IGC_GET_FLAG_VALUE(MemAccessThrottle));
  return Override == TriboolFlag::Default ? OCLCtx->m_InternalOptions.MemAccessThrottle
                                          : Override == TriboolFlag::Enabled;
}

static Value *stripBitCasts(Value *V) {
  while (auto *BC = dyn_cast<BitCastInst>(V))
    V = BC->getOperand(0);
  return V;
}

static bool isGlobalAccess(unsigned AS) { return AS == ADDRESS_SPACE_GLOBAL || AS == ADDRESS_SPACE_GENERIC; }

static bool isThrottleable(const ALoadInst &L) {
  auto *LI = dyn_cast<LoadInst>(L.inst());
  return LI ? !LI->isAtomic() : L.isSimple();
}

static bool isThrottleable(const AStoreInst &S) {
  auto *SI = dyn_cast<StoreInst>(S.inst());
  return SI ? !SI->isAtomic() : S.isSimple();
}

// The throttle was applied by hand, as in the CRI peer2peer write guide.
static bool hasManualThrottle(Function &F) {
  for (Instruction &I : instructions(F))
    if (auto *GII = dyn_cast<GenIntrinsicInst>(&I))
      if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_eu_thread_pause)
        return true;
  return false;
}

// The pass is opt-in, so a request that ends up doing nothing is worth saying
// out loud rather than leaving the caller to wonder. EmitWarning already names
// the kernel.
static void warn(CodeGenContext *CGC, Value *Ctx, const char *Reason) {
  std::string Msg = std::string("mem-access-throttle: ") + Reason;
  CGC->EmitWarning(Msg.c_str(), Ctx);
}

std::optional<AStoreInst> MemAccessThrottle::findReloadedStore(const ALoadInst &L) {
  const uint64_t LoadBytes = accessBytes(L.getType());

  for (Instruction *I = L.inst()->getPrevNode(); I; I = I->getPrevNode()) {
    auto S = AStoreInst::get(I);
    if (!S || !isThrottleable(*S))
      continue;
    if (!isGlobalAccess(S->getPointerAddressSpace()))
      continue;
    if (accessBytes(S->getValueOperand()->getType()) != LoadBytes)
      continue;
    // A constant difference means a shared base, i.e. not a peer buffer.
    int64_t Diff = 0;
    if (getDiffIfConstant(S->getPointerOperand(), L.getPointerOperand(), Diff, m_symEval) && Diff == 0)
      return S;
  }
  return std::nullopt;
}

void MemAccessThrottle::collectSites(BasicBlock &BB, SmallVectorImpl<Instruction *> &Sites) {
  m_symEval.clear();
  m_symEval.setDataLayout(m_DL);

  for (Instruction &I : BB) {
    auto S2 = AStoreInst::get(&I);
    if (!S2 || !isThrottleable(*S2))
      continue;
    if (!isGlobalAccess(S2->getPointerAddressSpace()))
      continue;
    if (accessBytes(S2->getValueOperand()->getType()) < MinAccessBytes)
      continue;

    auto L3 = ALoadInst::get(stripBitCasts(S2->getValueOperand()));
    if (!L3 || !isThrottleable(*L3) || L3->inst()->getParent() != &BB)
      continue;

    auto S1 = findReloadedStore(*L3);
    if (!S1)
      continue;
    // A constant difference means a shared base, i.e. not a peer buffer.
    int64_t Diff = 0;
    if (getDiffIfConstant(S1->getPointerOperand(), S2->getPointerOperand(), Diff, m_symEval))
      continue;

    Sites.push_back(&I);
  }
}

Value *MemAccessThrottle::createLeaderPredicate(Function &F) {
  IRBuilder<> B(&*F.getEntryBlock().getFirstInsertionPt());

  auto &MDW = getAnalysis<MetaDataUtilsWrapper>();
  IGCMD::MetaDataUtils *MDUtils = MDW.getMetaDataUtils();
  ImplicitArgs IAS(F, MDUtils, MDW.getModuleMetaData());
  Value *R0 = IAS.getImplicitArgValue(F, ImplicitArg::R0, MDUtils);

  Value *Tid = B.CreateExtractElement(R0, B.getInt32(2), "localThreadId");
  Tid = B.CreateAnd(Tid, B.getInt32(0xff));
  return B.CreateICmpEQ(Tid, B.getInt32(0), "throttle.isleader");
}

// get_group_id lowers to an extract from R0 (WIFuncResolution::getGroupId) and
// R0 is UNIFORM_THREAD, because it also carries per-thread fields. The group id
// lanes themselves are the same in every thread of the group, so recognise
// those and nothing else.
bool MemAccessThrottle::isR0GroupId(const Value *V) const {
  auto *EE = dyn_cast<ExtractElementInst>(V);
  if (!EE)
    return false;

  auto *Idx = dyn_cast<ConstantInt>(EE->getIndexOperand());
  if (!Idx)
    return false;
  const uint64_t Lane = Idx->getZExtValue();
  if (Lane != 1 && Lane != 6 && Lane != 7) // group id X, Y, Z
    return false;

  const Value *R0 = EE->getVectorOperand();
  if (auto *GII = dyn_cast<GenIntrinsicInst>(R0))
    return GII->getIntrinsicID() == GenISAIntrinsic::GenISA_getR0;
  return m_R0 && R0 == m_R0;
}

bool MemAccessThrottle::isWorkgroupUniform(Value *V, WIAnalysisRunner &WI) const {
  SmallPtrSet<Value *, 16> Visited;
  return isWorkgroupUniform(V, WI, Visited, 0);
}

bool MemAccessThrottle::isWorkgroupUniform(Value *V, WIAnalysisRunner &WI, SmallPtrSetImpl<Value *> &Visited,
                                           unsigned Depth) const {
  // UNIFORM_THREAD is not enough on its own: it is scalar within a thread but
  // may still differ between the threads of the group, as hw_thread_id does.
  if (isa<Constant>(V) || WI.isWorkGroupOrGlobalUniform(V))
    return true;
  if (isR0GroupId(V))
    return true;

  // Past the group id, uniformity can only be recovered by looking through the
  // arithmetic that WIAnalysis already gave up on.
  constexpr unsigned MaxDepth = 32;
  if (Depth >= MaxDepth)
    return false;

  auto *I = dyn_cast<Instruction>(V);
  if (!I)
    return false;
  // Side-effect-free arithmetic only. Anything that reads memory or calls out
  // can differ per thread for reasons its operands do not show.
  if (!isa<BinaryOperator>(I) && !isa<CastInst>(I) && !isa<CmpInst>(I) && !isa<SelectInst>(I) && !isa<PHINode>(I) &&
      !isa<ExtractElementInst>(I))
    return false;

  // A phi reached again is a loop back edge; the values entering the loop decide.
  if (!Visited.insert(V).second)
    return true;

  for (Value *Op : I->operands())
    if (!isWorkgroupUniform(Op, WI, Visited, Depth + 1))
      return false;
  return true;
}

bool MemAccessThrottle::hasWorkgroupUniformTripCount(Loop *L, WIAnalysisRunner &WI) const {
  SmallVector<BasicBlock *, 4> Exiting;
  L->getExitingBlocks(Exiting);
  if (Exiting.empty())
    return false;

  for (BasicBlock *BB : Exiting) {
    // IGCLLVM::CondBrInst, because LLVM 23 deprecates BranchInst in favour of
    // CondBrInst/UncondBrInst.
    auto *BI = dyn_cast<IGCLLVM::CondBrInst>(BB->getTerminator());
    if (!BI)
      return false;

    if (!isWorkgroupUniform(BI->getCondition(), WI))
      return false;
  }
  return true;
}

// BB is control dependent on a branch when one successor of that branch always
// leads to BB and another does not. Part of the group would skip the barrier if
// any such branch could go different ways within the group.
bool MemAccessThrottle::isWorkgroupUniformlyReached(BasicBlock *BB, WIAnalysisRunner &WI,
                                                    PostDominatorTree &PDT) const {
  for (BasicBlock &A : *BB->getParent()) {
    auto *BI = dyn_cast<IGCLLVM::CondBrInst>(A.getTerminator());
    if (!BI || PDT.dominates(BB, &A))
      continue;

    bool AnyReaches = false, AllReach = true;
    for (BasicBlock *Succ : successors(&A)) {
      if (PDT.dominates(BB, Succ))
        AnyReaches = true;
      else
        AllReach = false;
    }

    if (AnyReaches && !AllReach && !isWorkgroupUniform(BI->getCondition(), WI))
      return false;
  }
  return true;
}

Instruction *MemAccessThrottle::findThrottlePoint(Instruction *Site, WIAnalysisRunner &WI, PostDominatorTree &PDT,
                                                  LoopInfo &LI) const {
  BasicBlock *SiteBB = Site->getParent();
  Loop *SiteLoop = LI.getLoopFor(SiteBB);

  // A thread-varying trip count makes every point inside the loop unsafe.
  for (Loop *L = SiteLoop; L; L = L->getParentLoop())
    if (!hasWorkgroupUniformTripCount(L, WI))
      return nullptr;

  for (auto *N = PDT.getNode(SiteBB); N; N = N->getIDom()) {
    BasicBlock *BB = N->getBlock();
    if (!BB)
      continue;
    // Leaving the loop would stop the throttle from pacing each iteration.
    if (LI.getLoopFor(BB) != SiteLoop)
      return nullptr;
    if (isWorkgroupUniformlyReached(BB, WI, PDT))
      return BB->getTerminator();
  }
  return nullptr;
}

bool MemAccessThrottle::runOnFunction(Function &F) {
  if (F.isDeclaration() || F.hasOptNone())
    return false;
  if (!doMemAccessThrottle(getAnalysis<CodeGenContextWrapper>().getCodeGenContext()))
    return false;

  // A barrier needs every thread in the group to reach it the same number of
  // times, which a callee cannot guarantee.
  auto &MDW = getAnalysis<MetaDataUtilsWrapper>();
  if (!isEntryFunc(MDW.getMetaDataUtils(), &F))
    return false;

  m_DL = &F.getParent()->getDataLayout();

  SmallVector<Instruction *, 4> Sites;
  for (BasicBlock &BB : F)
    collectSites(BB, Sites);

  if (Sites.empty())
    return false;

  CodeGenContext *CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  // A hand-written throttle is already tuned to the kernel, and the rate comes
  // from the global size, so a second one would only change that tuning.
  if (hasManualThrottle(F)) {
    warn(CGC, Sites.front(), "kernel already throttles itself with eu_thread_pause, leaving it alone");
    return false;
  }

  // Built here rather than required as analyses, so that a kernel with no match
  // pays nothing.
  DominatorTree DT(F);
  PostDominatorTree PDT(F);
  LoopInfo LI(DT);
  TranslationTable TT;
  TT.run(F);
  WIAnalysisRunner WI(&F, &LI, &DT, &PDT, MDW.getMetaDataUtils(), CGC, MDW.getModuleMetaData(), &TT);
  WI.run();

  // Looked up rather than created: a kernel without the argument simply has no
  // group id to recognise.
  ImplicitArgs IAS(F, MDW.getMetaDataUtils(), MDW.getModuleMetaData());
  m_R0 = IAS.getImplicitArg(F, ImplicitArg::R0);

  // Several stores in one uniformly executed region share a single throttle, as
  // in the hand-written kernel. Choose all points before splitting anything,
  // since a split invalidates the analyses above.
  SmallVector<Instruction *, 4> Points;
  SmallPtrSet<Instruction *, 4> Seen;
  for (Instruction *Site : Sites)
    if (Instruction *P = findThrottlePoint(Site, WI, PDT, LI))
      if (Seen.insert(P).second)
        Points.push_back(P);

  if (Points.empty()) {
    warn(CGC, Sites.front(), "no point reached the same number of times by the whole work-group, no throttle inserted");
    return false;
  }

  Value *IsLeader = createLeaderPredicate(F);
  Constant *Amount = ConstantInt::get(Type::getInt32Ty(F.getContext()), LeaderPauseUnits);
  Function *PauseFn = GenISAIntrinsic::getDeclaration(F.getParent(), GenISAIntrinsic::GenISA_eu_thread_pause);
  Function *BarrierFn = GenISAIntrinsic::getDeclaration(F.getParent(), GenISAIntrinsic::GenISA_threadgroupbarrier);

  for (Instruction *Point : Points) {
    Instruction *ThenTerm = SplitBlockAndInsertIfThen(IsLeader, Point, false);
    IRBuilder<>(ThenTerm).CreateCall(PauseFn, {Amount});

    // Top of the tail block, so both the leader and the waiters reach it.
    IRBuilder<>(Point).CreateCall(BarrierFn);
  }
  return true;
}

namespace IGC {
FunctionPass *createMemAccessThrottlePass() { return new MemAccessThrottle(); }
} // namespace IGC
