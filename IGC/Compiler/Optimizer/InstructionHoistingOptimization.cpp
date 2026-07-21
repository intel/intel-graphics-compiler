/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InstructionHoistingOptimization.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "IGC/Compiler/CodeGenPublic.h"
#include "common/igc_regkeys.hpp"
#include "LLVM3DBuilder/BuiltinsFrontend.hpp"
#include "Probe/Assertion.h"
#include "common/Types.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include "llvm/IR/CFG.h"
#include "llvm/IR/Metadata.h"
#include "llvm/PassInfo.h"
#include "llvm/PassRegistry.h"
#include "llvm/IR/InstVisitor.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
namespace IGC {
// clang-format off
/////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Hoisting instructions (sampler) to earlier place in code after loop
/// unroll The reason on doing this hoisting is because for sampler after LICM,
/// only instructions with loop-invariants can be hoisted, and sampler in the
/// loop is like:
/// 19:                                               ; preds = %281, %0
///    %20 = phi i32[0, %0], [%265, %281]
///    %21 = phi i32[0, %0], [%202, %281]
///    %22 = phi i32[0, %0], [%139, %281]
///    %23 = phi i32[0, %0], [%76,  %281]
///    %24 = phi i32[0, %0], [%283, %281]
///    %25 = phi i32[0, %0], [%284, %281]
///    %26 = phi i32[0, %0], [%285, %281]
///    %27 = add nsw i32 %26, %17
///    %28 = uitofp i32 %27 to float
///    %29 = uitofp i32 %18 to float
///    %30 = fadd fast float %28, 5.000000e-01
///    %31 = fadd fast float %29, 5.000000e-01
///    %32 = call fast <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904v4f32(<4 x float> addrspace(2555904)* %"b0,1", i32 832, i32 4, i1 false)
///    %33 = extractelement <4 x float> %32, i64 0
///    %34 = extractelement <4 x float> %32, i64 1
///    %35 = fmul fast float %30, %33
///    %36 = fmul fast float %34, %31
///    %37 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %36, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
/// In this loop, only ldrawvector can be hoisted, and sampleLptr has operands
/// %36, %36 have input dependency that LICM cannot do hoisting. So, after LICM
/// and loop unroll, only one sampleLptr is hoisted (offset 0 in phi), we can do
/// a custom pass to handle the rest sampleLptr hoist as below:
/// ===> Hoistable instruction case (%71 sampler below):
/// ......
///    %36 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %20, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
///    %37 = extractelement <4 x float> %36, i64 3
/// ......
/// NodeBlock27:                                      ; preds = %NewDefault, %64, %66, %68
///    %70 = phi i32[%69, %68], [%67, %66], [%65, %64], [%63, %NewDefault]
///    %71 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %24, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
///    %72 = extractelement <4 x float> %71, i64 3
/// ......
/// ===> Proposed optimization (the above %71 sampler is hoisted up to be %37 below):
/// ......
///    %36 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %20, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
///    %37 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %24, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
///......
///    %64 = extractelement <4 x float> %36, i64 3
///......
/// NodeBlock27:                                      ; preds = %NewDefault, %91, %93, %95
///    %97 = phi i32[%96, %95], [%94, %93], [%92, %91], [%90, %NewDefault]
///    %98 = extractelement <4 x float> %37, i64 3
///......
// clang-format on
class InstructionHoistingOptimization : public llvm::FunctionPass,
                                        public llvm::InstVisitor<InstructionHoistingOptimization> {
public:
  static char ID; ///< ID used by the llvm PassManager (the value is not important)

  InstructionHoistingOptimization();

  ////////////////////////////////////////////////////////////////////////
  virtual bool runOnFunction(llvm::Function &F);

  ////////////////////////////////////////////////////////////////////////
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

  ////////////////////////////////////////////////////////////////////////
  void visitCallInst(llvm::CallInst &CI);

private:
  ////////////////////////////////////////////////////////////////////////
  bool ProcessFunction(llvm::Function &F);

  ////////////////////////////////////////////////////////////////////////
  void InvalidateMembers();

  ////////////////////////////////////////////////////////////////////////
  void CollectHoistableInstructions();

  ////////////////////////////////////////////////////////////////////////
  bool ProcessHoistableInstructions();

  ////////////////////////////////////////////////////////////////////////
  // PS-only per-sampler bounded hoist (replaces the global single-point batch
  // hoist for pixel shaders). See definitions for the rationale.
  bool hoistSamplersPS(llvm::Function &F);
  llvm::BasicBlock *findBoundedHoistTarget(llvm::Instruction *sampler, unsigned maxBlocks) const;
  bool collectCluster(llvm::Instruction *root, llvm::BasicBlock *target,
                      llvm::SmallVectorImpl<llvm::Instruction *> &cluster,
                      llvm::SmallPtrSetImpl<llvm::Instruction *> &visited) const;
  void collectHoistRegion(llvm::BasicBlock *target, llvm::BasicBlock *sampBB,
                          llvm::SmallPtrSetImpl<llvm::BasicBlock *> &region) const;

  ////////////////////////////////////////////////////////////////////////
  CodeGenContext *m_pCGCtxt = nullptr;
  DominatorTree *m_pDT = nullptr;
  // PS-only: non-null when the function still has loops (survived unroll). Used
  // to keep the pass out of loop bodies instead of bailing the whole function.
  llvm::LoopInfo *m_pLI = nullptr;
  llvm::PostDominatorTree *m_pPDT = nullptr;
  IGCLivenessAnalysisRunner *m_pRPE = nullptr;
  // Uniformity, so the pressure estimate counts uniform values once (not at full
  // SIMD width). Without it the gate over-counts and rejects safe hoists on SIMD32.
  WIAnalysisRunner *m_pWI = nullptr;

  // Pixel shaders are opt-in and require the extra gating below; non-PS keeps
  // the original (unconditional) hoisting behavior.
  bool m_isPS = false;
  // Injected from the call site (PS-only effect):
  // allow hoisting a sampler across a conditional branch (speculative)...
  bool m_allowSpeculative = false;
  // ...and GRF slack added to the no-new-spills pressure budget.
  unsigned m_rpMargin = 12;

  std::vector<llvm::Instruction *> m_SamplerInstructions;
  std::vector<llvm::Instruction *> m_HoistableInstructions;

  llvm::Instruction *m_InsertHoistBack = nullptr;
  llvm::BasicBlock *m_HoistBasicBlock = nullptr;

  // PS-only (m_pLI is null otherwise, so this is inert on the non-PS path):
  // true if BB belongs to any loop that survived unroll.
  bool isInLoop(llvm::BasicBlock *BB) const { return m_pLI && m_pLI->getLoopFor(BB) != nullptr; }

  // True if `I` is a raw buffer load that is value-invariant AND backed by a
  // read-only constant buffer. Such a load may ride up with a hoisted sampler:
  // relocating it (even speculatively across a branch) changes neither the
  // loaded value nor the fault behavior, because a constant buffer is resident
  // on every path. UAV / writable / generic loads are excluded (aliasing and
  // fault risk). This lets a sampler whose coordinates are computed from
  // constant-buffer reads hoist together with those reads.
  bool isSpeculatableInvariantLoad(const llvm::Instruction *I) const {
    const auto *LR = llvm::dyn_cast<LdRawIntrinsic>(I);
    if (!LR || LR->isVolatile())
      return false;
    switch (DecodeBufferType(LR->getResourceValue()->getType()->getPointerAddressSpace())) {
    case CONSTANT_BUFFER:
    case BINDLESS_CONSTANT_BUFFER:
    case SSH_BINDLESS_CONSTANT_BUFFER:
    case STATELESS_READONLY:
      return true;
    default:
      return false;
    }
  }

  // True if `I` is a sample whose LOD/gradients come from implicit screen-space
  // derivatives across the pixel quad (SampleIntrinsic::IsDerivative). Such
  // samples are unsafe to hoist SPECULATIVELY: a divergent branch changes the
  // quad's active lanes and thus the derivatives. Explicit-LOD (sampleL*) and
  // explicit-gradient (sampleD*) samples are not derivative and stay speculatable.
  bool sampleNeedsImplicitDerivatives(const llvm::Instruction *I) const {
    const auto *SI = llvm::dyn_cast<SampleIntrinsic>(I);
    return SI && SI->IsDerivative();
  }

  inline bool traceOperandInHoistBB(Value *operand, BasicBlock *hoistBB) {
    if (Instruction *inst = dyn_cast<Instruction>(operand)) {
      for (auto &op : inst->operands()) {
        if (isPHINode(op) || !isHoistableInstruction(op, hoistBB)) {
          return false;
        }
      }
    }
    return true;
  }
  inline bool isPHINode(Value *op) { return isa<PHINode>(op); }
  inline bool isHoistableInstruction(Value *op, BasicBlock *hoistBB) {
    if (auto *I = dyn_cast<Instruction>(op)) {
      auto *currBB = I->getParent();
      if (isa<LoadInst, LdRawIntrinsic, SampleIntrinsic>(I)) {
        return false;
      }
      if (currBB != hoistBB) {
        if (m_pDT->dominates(hoistBB, currBB)) {
          if (!traceOperandInHoistBB(op, hoistBB)) {
            return false;
          }
          InsertHoistableInstruction(I);
        } else {
          return false;
        }
      } else if (m_InsertHoistBack->comesBefore(I)) {
        return false;
      }
    }
    return true;
  }

  inline void InsertHoistableInstruction(Instruction *hoistInstr) {
    auto isDuplicate = [](std::vector<Instruction *> hoistedInstr, Instruction *newInstr) -> bool {
      auto it = std::find(hoistedInstr.begin(), hoistedInstr.end(), newInstr);
      return it != hoistedInstr.end();
    };

    if (!isDuplicate(m_HoistableInstructions, hoistInstr)) {
      m_HoistableInstructions.push_back(hoistInstr);
    }
  }
};

char InstructionHoistingOptimization::ID = 0;

////////////////////////////////////////////////////////////////////////////
InstructionHoistingOptimization::InstructionHoistingOptimization() : llvm::FunctionPass(ID) {
  initializeInstructionHoistingOptimizationPass(*PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////
bool InstructionHoistingOptimization::runOnFunction(llvm::Function &F) {
  m_pCGCtxt = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_pDT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  m_isPS = (m_pCGCtxt->type == ShaderType::PIXEL_SHADER);

  // Non-PS keeps the original premise (all loops unrolled): bail if any loop
  // remains. PS may still hoist samplers that live in loop-free regions of a
  // function whose loops survived unroll (e.g. dynamic trip counts), so keep
  // LoopInfo and gate per-region below (isInLoop + loop-free hoist point).
  if (!m_isPS) {
    if (!LI.empty()) {
      return false;
    }
  } else {
    m_pLI = &LI;
    // PS path needs speculation control + register-pressure gating. The driver
    // injects these via the constructor; also honor the regkeys directly so the
    // pass is configurable in standalone tools (igc_opt / IGCStandalone), where
    // the pass is created with default ctor args. This is a no-op for the driver
    // path, where the ctor value already equals the regkey.
    m_pPDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    m_pRPE = &getAnalysis<IGCLivenessAnalysis>().getLivenessRunner();
    m_pWI = &getAnalysis<WIAnalysis>().Runner;
  }

  InvalidateMembers();

  return ProcessFunction(F);
}

////////////////////////////////////////////////////////////////////////
void InstructionHoistingOptimization::visitCallInst(llvm::CallInst &CI) {
  if (llvm::GenIntrinsicInst *pIntr = llvm::dyn_cast<llvm::GenIntrinsicInst>(&CI)) {
    if (auto *SI = dyn_cast<SampleIntrinsic>(pIntr)) {
      m_SamplerInstructions.push_back(SI);
    }
  }
}

////////////////////////////////////////////////////////////////////////
void InstructionHoistingOptimization::CollectHoistableInstructions() {
  if (m_SamplerInstructions.empty())
    return;

  // use hash.getAsmHash() to debug specific shader

  // To make the hoisted instruction up as early as possible, check the first
  // BB, and check each sample operand and determined the latest location to as
  // hoist point. For the rest BBs, to simplify the process to sample hoist,
  // only pick the hoist point in first BB with sample, and use it to determine
  // the rest sample hoistable choice.
  for (Instruction *I : m_SamplerInstructions) {
    for (auto &Op : I->operands()) {
      if (Instruction *Inst = dyn_cast<Instruction>(Op)) {
        // pick the first op (inst) as the initial hoist point
        if (!m_InsertHoistBack) {
          m_InsertHoistBack = Inst;
          continue;
        }

        if (m_InsertHoistBack->getParent() == Inst->getParent()) {
          if (m_InsertHoistBack->comesBefore(Inst)) {
            m_InsertHoistBack = Inst;
          }
        }
      }
    }
  }

  if (!m_InsertHoistBack)
    return;

  m_HoistBasicBlock = m_InsertHoistBack->getParent();

  // Note: this path runs for non-PS only (PS is routed to hoistSamplersPS before
  // this is reached), so no PS-specific gating is needed here.
  for (auto *pSI : m_SamplerInstructions) {
    bool canInstrHoisted = true;

    for (auto &Op : pSI->operands()) {
      if (isa<PHINode>(Op)) {
        canInstrHoisted = false;
        break;
      }

      if (Instruction *I = dyn_cast<Instruction>(Op)) {
        if (isa<LoadInst, LdRawIntrinsic, SampleIntrinsic>(I)) {
          canInstrHoisted = false;
          break;
        }

        if (m_HoistBasicBlock == I->getParent()) {
          // If m_HoistBasicBlock comes before any operand of pSI,
          // this pSI won't be hoisted in this BB
          if (m_InsertHoistBack->comesBefore(I)) {
            canInstrHoisted = false;
            break;
          }
        } else {
          // check if op can be traced back to hoistInBB
          if (traceOperandInHoistBB(Op, m_HoistBasicBlock)) {
            InsertHoistableInstruction(I);
          } else {
            canInstrHoisted = false;
            break;
          }
        }
      }
    }

    if (canInstrHoisted) {
      InsertHoistableInstruction(pSI);
    }
  }
}

////////////////////////////////////////////////////////////////////////
bool InstructionHoistingOptimization::ProcessHoistableInstructions() {
  if (m_HoistableInstructions.empty())
    return false;

  // Non-PS only: original unconditional hoisting to the global single point.
  // (PS is routed to hoistSamplersPS, which does per-sampler bounded hoisting.)
  auto *hoist_point = m_InsertHoistBack;
  for (auto *I : m_HoistableInstructions) {
    I->moveAfter(hoist_point);
    hoist_point = I;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////
// Walk up the dominator tree from the sampler's block, at most maxBlocks steps,
// and return the furthest ancestor that is a legal hoist target: loop-free and
// (unless speculative hoisting is allowed) post-dominated by the sampler's block
// so the sampler is not executed more often than today. nullptr if none qualify.
llvm::BasicBlock *InstructionHoistingOptimization::findBoundedHoistTarget(llvm::Instruction *sampler,
                                                                          unsigned maxBlocks) const {
  BasicBlock *SampBB = sampler->getParent();
  DomTreeNode *Node = m_pDT->getNode(SampBB);
  if (!Node)
    return nullptr;

  // Implicit-derivative samples must never be hoisted speculatively (a divergent
  // branch would change the quad's active lanes and thus the derivatives), so
  // confine them to non-speculative (control-equivalent) targets even when
  // speculative hoisting is enabled. Explicit-LOD/gradient samples are exempt.
  const bool AllowSpec = m_allowSpeculative && !sampleNeedsImplicitDerivatives(sampler);

  BasicBlock *Best = nullptr;
  for (unsigned step = 0; step < maxBlocks; ++step) {
    Node = Node->getIDom();
    if (!Node)
      break;
    BasicBlock *Cand = Node->getBlock();
    if (!Cand || isInLoop(Cand))
      break;
    // Speculation: hoisting to Cand runs the sampler whenever Cand runs, which is
    // safe (non-speculative) only if SampBB post-dominates Cand.
    if (!AllowSpec && !m_pPDT->dominates(SampBB, Cand))
      break;
    Best = Cand;
  }
  return Best;
}

////////////////////////////////////////////////////////////////////////
// Gather, in def-before-use order into `cluster`, the operand-chain instructions
// that must move together with `root` so it can be placed high in `target`.
// Returns false if `root` cannot be legally hoisted to `target`: a PHI operand, a
// memory/latency op that must stay put, a loop-body value, or an operand defined
// off the dominator path between `target` and `root`.
bool InstructionHoistingOptimization::collectCluster(llvm::Instruction *root, llvm::BasicBlock *target,
                                                     llvm::SmallVectorImpl<llvm::Instruction *> &cluster,
                                                     llvm::SmallPtrSetImpl<llvm::Instruction *> &visited) const {
  for (Value *Op : root->operands()) {
    if (isa<PHINode>(Op))
      return false;
    Instruction *I = dyn_cast<Instruction>(Op);
    if (!I)
      continue; // constant / argument / global -> available everywhere
    BasicBlock *Ibb = I->getParent();
    // Already available at the insertion point and doesn't need to move: defined
    // above target, or in target itself (we insert the cluster AFTER the latest
    // in-target operand, so it is visible).
    if (Ibb == target || m_pDT->dominates(Ibb, target))
      continue;
    // Otherwise it must sit strictly between target and root (target dominates it).
    if (!m_pDT->dominates(target, Ibb))
      return false;
    if (isInLoop(Ibb))
      return false;
    // Never relocate another sampler. A load may ride along ONLY when it is an
    // invariant read-only constant-buffer read (value- and fault-safe to move,
    // even speculatively); every other load stays put.
    if (isa<SampleIntrinsic>(I))
      return false;
    if (isa<LoadInst, LdRawIntrinsic>(I) && !isSpeculatableInvariantLoad(I))
      return false;
    if (!visited.insert(I).second)
      continue; // already queued
    if (!collectCluster(I, target, cluster, visited))
      return false;
    cluster.push_back(I); // pushed after its own operands -> def-before-use order
  }
  return true;
}

////////////////////////////////////////////////////////////////////////
// Collect the CFG region whose liveness a hoist from `sampBB` up to `target`
// can change. A def only ever moves UP to `target` (which dominates it) and SSA
// guarantees the def dominates all its uses, so the only blocks whose live sets
// change are those on a path from `target` to `sampBB`. A backward walk from
// `sampBB` (stopping at `target`, which dominates it) collects exactly those,
// letting the liveness recompute be scoped instead of whole-function.
void InstructionHoistingOptimization::collectHoistRegion(llvm::BasicBlock *target, llvm::BasicBlock *sampBB,
                                                         llvm::SmallPtrSetImpl<llvm::BasicBlock *> &region) const {
  region.insert(target);
  region.insert(sampBB);
  llvm::SmallVector<llvm::BasicBlock *, 8> worklist;
  worklist.push_back(sampBB);
  while (!worklist.empty()) {
    llvm::BasicBlock *bb = worklist.pop_back_val();
    if (bb == target)
      continue; // don't expand above the (dominating) target
    for (llvm::BasicBlock *pred : llvm::predecessors(bb))
      if (region.insert(pred).second)
        worklist.push_back(pred);
  }
}

////////////////////////////////////////////////////////////////////////
// PS latency hoist: move each sampler up by a bounded number of dominating
// blocks (enough to expose independent work between the send and its first use)
// rather than to a single function-wide point. Each sampler's cluster is moved
// and pressure re-measured at the widest PS SIMD (SIMD32); a move that would
// exceed the "no new spills" budget is rolled back individually, so one
// expensive sampler never blocks the others and no SIMD variant is spilled.
bool InstructionHoistingOptimization::hoistSamplersPS(llvm::Function &F) {
  if (m_SamplerInstructions.empty())
    return false;

  const unsigned SIMD = m_pCGCtxt->platform.getMaxSimdSize();

  // Baseline function-wide pressure at the widest SIMD. Computed first because it
  // also drives the dynamic budget below.
  unsigned Running = m_pRPE->getMaxRegCountForFunction(F, SIMD, m_pWI);

  // Dynamic, dispatch-mode-agnostic budget.
  const unsigned Default128 = m_pCGCtxt->getNumGRFPerThread(true);
  unsigned GrfCeiling = Default128;
  unsigned PayloadReserve = 24; // default
  if (m_pCGCtxt->platform.isCoreChildOf(IGFX_XE3P_CORE)) {
    if (Running > 192) {
      GrfCeiling = 256;
      PayloadReserve /= 2;
    } else if (Running > 128) {
      GrfCeiling = 192;
    }
  }
  const unsigned Reserved = PayloadReserve + m_rpMargin;
  const unsigned Budget = (Reserved < GrfCeiling) ? (GrfCeiling - Reserved) : (GrfCeiling / 2);

  if (Running >= Budget)
    return false;

  // How many dominating blocks a sampler may travel. Kept minimal (1) so the
  // hoisted result's live range stays short (bounded SIMD32 pressure) while
  // still crossing a block boundary to expose independent latency work. A future
  // refinement could try decreasing distances when the furthest overshoots budget.
  const unsigned MaxHoistBlocks = 1;

  bool Changed = false;

  for (auto *pSI : m_SamplerInstructions) {
    BasicBlock *SampBB = pSI->getParent();
    if (isInLoop(SampBB))
      continue;

    BasicBlock *Target = findBoundedHoistTarget(pSI, MaxHoistBlocks);
    if (!Target)
      continue;

    SmallVector<Instruction *, 16> cluster;
    SmallPtrSet<Instruction *, 16> visited;
    if (!collectCluster(pSI, Target, cluster, visited))
      continue;

    // Blocks whose liveness this hoist can change (target..sampBB); used to scope
    // the recompute below. Depends only on the CFG, which the moves don't alter.
    SmallPtrSet<BasicBlock *, 32> Region;
    collectHoistRegion(Target, SampBB, Region);

    // Insertion point in `target`: as early as possible (top, after PHIs) so
    // target's body runs AFTER the send and hides its latency, but never before
    // an operand that already lives in `target` (the send must see it). So pick
    // the point just after the latest in-target operand of the sampler/cluster,
    // or the top when there is none.
    Instruction *InsertPt = &*Target->getFirstInsertionPt();
    {
      Instruction *LatestDep = nullptr;
      auto considerUser = [&](Instruction *user) {
        for (Value *op : user->operands())
          if (auto *opI = dyn_cast<Instruction>(op))
            if (opI->getParent() == Target && (!LatestDep || LatestDep->comesBefore(opI)))
              LatestDep = opI;
      };
      considerUser(pSI);
      for (Instruction *I : cluster)
        considerUser(I);
      if (LatestDep)
        InsertPt = LatestDep->getNextNode();
    }

    // Move the operand cluster (def-before-use) then the sampler before insertPt.
    // Record originals for rollback.
    SmallVector<std::pair<Instruction *, Instruction *>, 16> Undo;
    Undo.reserve(cluster.size() + 1);
    for (Instruction *I : cluster) {
      Undo.push_back({I, I->getNextNode()});
      IGCLLVM::moveBefore(I, InsertPt);
    }
    Undo.push_back({pSI, pSI->getNextNode()});
    IGCLLVM::moveBefore(pSI, InsertPt);

    // Scoped liveness recompute: only `Region` In/Out sets can change, so this is
    // equivalent to a full rerun but skips the whole-function dataflow fixpoint.
    m_pRPE->rerunLivenessAnalysis(F, &Region);

    // Pressure changes only within Region (that's why the recompute above is
    // scoped there), and Running holds the full-function baseline, so the region
    // max is enough: non-region blocks are <= Running and can't trip the gate.
    // Equivalent to a full-function max here, but O(region) not O(function).
    unsigned AfterHoist = 0;
    for (BasicBlock *BB : Region)
      AfterHoist = std::max(AfterHoist, m_pRPE->getMaxRegCountForBB(*BB, SIMD, m_pWI));

    // Allow pressure to grow up to the GRF budget; never past the running peak
    // once that already exceeds the budget (don't worsen an over-budget shader).
    if (AfterHoist > std::max(Running, Budget)) {
      for (auto it = Undo.rbegin(); it != Undo.rend(); ++it)
        IGCLLVM::moveBefore(it->first, it->second);
      m_pRPE->rerunLivenessAnalysis(F, &Region);
      continue;
    }

    // Update Running to the post-hoist function-wide max (which may be the
    // region max or the original global max, whichever is larger).
    Running = std::max(Running, AfterHoist); // accept
    Changed = true;
    // Pin against CodeSinking: it runs again after OptimizeIR and would sink the
    // sample back toward its consumer to relieve pressure, undoing this latency
    // hoist. The marker tells CodeSinking to leave THIS sample where we put it
    // (targeted, unlike the global DisableCodeSinkingLongLatencyInsts key).
    pSI->setMetadata(MD_LATENCY_HOISTED_SAMPLE, llvm::MDNode::get(F.getContext(), {}));
  }

  return Changed;
}

////////////////////////////////////////////////////////////////////////
bool InstructionHoistingOptimization::ProcessFunction(llvm::Function &F) {
  visit(F);

  // PS uses a per-sampler bounded hoist with incremental SIMD32 pressure gating;
  // non-PS keeps the original global single-point batch hoist.
  if (m_isPS) {
    return hoistSamplersPS(F);
  }

  CollectHoistableInstructions();

  return ProcessHoistableInstructions();
}

////////////////////////////////////////////////////////////////////////
void InstructionHoistingOptimization::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  // Used by the PS path for speculation control and register-pressure gating.
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.addRequired<IGCLivenessAnalysis>();
  AU.addRequired<WIAnalysis>();
}

////////////////////////////////////////////////////////////////////////
void InstructionHoistingOptimization::InvalidateMembers() {
  m_SamplerInstructions.clear();
  m_HoistableInstructions.clear();
}

////////////////////////////////////////////////////////////////////////
llvm::Pass *createInstructionHoistingOptimization() { return new InstructionHoistingOptimization(); }

} // namespace IGC

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-instruction-hoisting-optimization"
#define PASS_DESCRIPTION "InstructionHoistingOptimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InstructionHoistingOptimization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_END(InstructionHoistingOptimization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
