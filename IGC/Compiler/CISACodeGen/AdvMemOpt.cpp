/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/CFG.h>
#include <llvm/Pass.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/MemOpt2.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/AdvMemOpt.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include "Compiler/CISACodeGen/PrepareLoadsStoresUtils.h"
#include "Compiler/CISACodeGen/helper.h"
#include "llvmWrapper/Analysis/TargetLibraryInfo.h"
#include "llvmWrapper/Transforms/Utils/LoopUtils.h"
#include "llvmWrapper/ADT/Optional.h"
#include "Probe/Assertion.h"
#include "llvmWrapper/IR/Instructions.h"
#include <algorithm>
#include <limits>

using namespace llvm;
using namespace llvm::PatternMatch;
using namespace IGC;
using namespace IGC::IGCMD;

#define DEBUG_TYPE "AdvMemOpt"

namespace {

class AdvMemOpt : public FunctionPass {
  DominatorTree *DT = nullptr;
  LoopInfo *LI = nullptr;
  PostDominatorTree *PDT = nullptr;
  WIAnalysis *WI = nullptr;
  TargetLibraryInfo *TLI = nullptr;
  IGCLivenessAnalysisRunner *RPE = nullptr;
  unsigned SIMD = 0;
  // Fraction of the GRF file held back at the hoist destination, so hoisting
  // leaves register allocation something to work with. 16 GRFs at 128.
  static constexpr unsigned ReservedGRFDenominator = 8;

public:
  static char ID;

  AdvMemOpt() : FunctionPass(ID) { initializeAdvMemOptPass(*PassRegistry::getPassRegistry()); }

  bool runOnFunction(Function &F) override;

  StringRef getPassName() const override { return "Advanced MemOpt"; }

private:
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addPreservedID(WIAnalysis::ID);
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<WIAnalysis>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    // Required, not preserved: hoisting moves instructions, so the live sets go
    // stale for the remat passes downstream in AddLegalizationPasses.
    AU.addRequired<IGCLivenessAnalysis>();
  }

  bool collectOperandInst(SmallPtrSetImpl<Instruction *> &, Instruction *, BasicBlock *) const;
  bool collectTrivialUser(SmallPtrSetImpl<Instruction *> &, Instruction *) const;
  bool hoistUniformLoad(ArrayRef<BasicBlock *>) const;

  unsigned occupiedRegisters(BasicBlock &Lead) const;
  unsigned hoistBudgetInRegisters(BasicBlock *Lead) const;

  bool hoistInst(Instruction *inst, BasicBlock *, unsigned &BudgetInRegisters) const;

  bool isLeadCandidate(BasicBlock *) const;

  bool hasMemoryWrite(BasicBlock *BB) const;
  bool hasMemoryWrite(BasicBlock *Entry, BasicBlock *Exit) const;

  MemInstCluster Cluster;
};

char AdvMemOpt::ID = 0;

} // End anonymous namespace

FunctionPass *IGC::createAdvMemOptPass() { return new AdvMemOpt(); }

#define PASS_FLAG "igc-advmemopt"
#define PASS_DESC "Advanced Memory Optimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(AdvMemOpt, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass);
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
IGC_INITIALIZE_PASS_END(AdvMemOpt, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
} // End namespace IGC

bool AdvMemOpt::runOnFunction(Function &F) {
  bool Changed = false;

  // Skip non-kernel function.
  MetaDataUtils *MDU = nullptr;
  MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto FII = MDU->findFunctionsInfoItem(&F);
  if (FII == MDU->end_FunctionsInfo())
    return false;

  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  WI = &getAnalysis<WIAnalysis>();
  TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  RPE = &getAnalysis<IGCLivenessAnalysis>().getLivenessRunner();
  SIMD = numLanes(IGC::bestGuessSIMDSize(getAnalysis<CodeGenContextWrapper>().getCodeGenContext(), &F,
                                         getAnalysisIfAvailable<GenXFunctionGroupAnalysis>()));

  SmallVector<Loop *, 8> InnermostLoops;
  for (auto I = LI->begin(), E = LI->end(); I != E; ++I)
    for (auto DFI = df_begin(*I), DFE = df_end(*I); DFI != DFE; ++DFI) {
      Loop *L = *DFI;
      if (IGCLLVM::isInnermost(L))
        InnermostLoops.push_back(L);
    }

  const bool FollowPostDom = IGC_IS_FLAG_ENABLED(AdvMemOptAggressiveHoist);

  for (Loop *L : InnermostLoops) {
    SmallVector<BasicBlock *, 8> Line;
    SmallPtrSet<BasicBlock *, 8> Seen;
    BasicBlock *BB = L->getHeader();
    // The post-dominator tree is acyclic; 'Seen' only guards a degenerate CFG.
    while (BB && Seen.insert(BB).second) {
      Line.push_back(BB);
      BasicBlock *CurrBB = BB;
      BB = nullptr;
      if (FollowPostDom) {
        // Neither arm of an if/else diamond post-dominates CurrBB, so successor
        // scanning stalls there. The immediate post-dominator is the join past it.
        if (auto *PDTNode = PDT->getNode(CurrBB))
          if (auto *IPDomNode = PDTNode->getIDom())
            BB = IPDomNode->getBlock();
        // hasMemoryWrite() asserts each 'Line' block is dominated by, and
        // post-dominates, its predecessor.
        if (BB && (BB == CurrBB || !L->contains(BB) || !DT->dominates(CurrBB, BB) || !PDT->dominates(BB, CurrBB)))
          BB = nullptr;
      } else {
        for (auto BI = succ_begin(CurrBB), BE = succ_end(CurrBB); BI != BE; ++BI) {
          BasicBlock *OtherBB = *BI;
          if (CurrBB == OtherBB || !L->contains(OtherBB))
            continue;
          if (DT->dominates(CurrBB, OtherBB) && PDT->dominates(OtherBB, CurrBB)) {
            BB = OtherBB;
            break;
          }
        }
      }
    }
    Changed |= hoistUniformLoad(Line);
  }

  auto *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (Ctx->platform.isProductChildOf(IGFX_DG2)) {
    // 1) split 64-bit uniform store into <2 x i32>, so it has better chance
    // to merge with other i32 stores in order to form 16-byte stores that
    // can use L1 cache.
    // 2) count the number of sample operations that use lane-varying
    // resource or sampler state. We need to apply mem-inst-clustering
    // because, once ballot-loop is added, vISA finalizer cannot schedule
    // those sample operations.
    auto &DL = F.getParent()->getDataLayout();
    IGCIRBuilder<> IRB(F.getContext());
    Cluster.init(Ctx, &DL, nullptr /*AA*/, TLI, 32);
    for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {
      BasicBlock *BB = &*I;
      unsigned NumResourceVarying = 0;
      bool HasStore = false;
      for (BasicBlock::iterator II = BB->begin(), EI = BB->end(); II != EI;
           /*empty*/) {
        Instruction *I = &*II++;
        if (I->mayWriteToMemory())
          HasStore = true;
        if (auto *SI = dyn_cast<SampleIntrinsic>(I)) {
          if (!WI->isUniform(SI->getTextureValue()) || !WI->isUniform(SI->getSamplerValue())) {
            NumResourceVarying++;
          }
        } else if (auto *GI = dyn_cast<SamplerGatherIntrinsic>(I)) {
          if (!WI->isUniform(GI->getTextureValue()) || !WI->isUniform(GI->getSamplerValue())) {
            NumResourceVarying++;
          }
        } else if (auto *LI = dyn_cast<SamplerLoadIntrinsic>(I)) {
          if (!WI->isUniform(LI->getTextureValue())) {
            NumResourceVarying++;
          }
        } else if (auto *LI = dyn_cast<LdRawIntrinsic>(I)) {
          if (!WI->isUniform(LI->getResourceValue())) {
            NumResourceVarying++;
          }
        } else if (auto SI = AStoreInst::get(I); SI.has_value()) {
          if (!WI->isUniform(SI->inst()))
            continue;

          unsigned AS = SI->getPointerAddressSpace();
          if (AS != ADDRESS_SPACE_PRIVATE && AS != ADDRESS_SPACE_GLOBAL)
            continue;

          IRB.SetInsertPoint(SI->inst());

          if (auto NewSI = expand64BitStore(IRB, DL, SI.value())) {
            auto NewASI = AStoreInst::get(NewSI);
            WI->incUpdateDepend(NewSI, WIAnalysis::UNIFORM_THREAD);
            WI->incUpdateDepend(NewASI->getValueOperand(), WIAnalysis::UNIFORM_THREAD);
            WI->incUpdateDepend(NewASI->getPointerOperand(), WIAnalysis::UNIFORM_THREAD);
            SI->inst()->eraseFromParent();
            Changed = true;
          }
        }
      }
      // If a basic-block has lane-varying resource access
      if (NumResourceVarying) {
        Ctx->m_instrTypes.numSamplesVaryingResource += NumResourceVarying;
        // clustering method cannot handle memory dependence
        if (!HasStore)
          Changed |= Cluster.runForGFX(BB);
      }
    }
  }
  return Changed;
}

bool AdvMemOpt::isLeadCandidate(BasicBlock *BB) const {
  // A candidate lead should have at least one uniform load. In addition,
  // there's no instruction might to write memory from the last uniform loads
  // to the end.
  LLVM_DEBUG(dbgs() << "Check lead candidate: " << BB->getName() << "\n");
  for (auto II = BB->rbegin(), IE = BB->rend(); II != IE; ++II) {
    if (II->mayWriteToMemory()) {
      LLVM_DEBUG(dbgs() << " - May write to memory. Bail out: " << *II << "\n");
      return false;
    }
    std::optional<ALoadInst> LD = ALoadInst::get(&*II);
    if (!LD.has_value() || !WI->isUniform(LD->inst())) {
      LLVM_DEBUG(dbgs() << " - Not uniform load. Skip: " << *II << "\n");
      continue;
    }
    LLVM_DEBUG(dbgs() << "Found uniform loads.\n");
    return true;
  }
  return false;
}

namespace {
class RegionSubgraph {
  BasicBlock *Exit;
  SmallPtrSet<BasicBlock *, 32> Visited;

public:
  RegionSubgraph(BasicBlock *E) : Exit(E) {}

  // Visited-set for post_order_ext; false prunes the walk at Exit.
  std::pair<SmallPtrSet<BasicBlock *, 32>::iterator, bool> insert(BasicBlock *To) {
    if (To == Exit)
      return {Visited.end(), false};
    return Visited.insert(To);
  }
};
} // End anonymous namespace

bool AdvMemOpt::hasMemoryWrite(BasicBlock *BB) const {
  for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
    if (II->mayWriteToMemory())
      return true;
  return false;
}

bool AdvMemOpt::hasMemoryWrite(BasicBlock *Entry, BasicBlock *Exit) const {
  // Entry and Exit must be on line of code.
  IGC_ASSERT(nullptr != DT);
  IGC_ASSERT(DT->dominates(Entry, Exit));
  IGC_ASSERT(nullptr != PDT);
  IGC_ASSERT(PDT->dominates(Exit, Entry));

  RegionSubgraph RSG(Exit);
  for (auto *SI : post_order_ext(Entry, RSG))
    if (SI != Entry && hasMemoryWrite(SI))
      return true;
  return false;
}

bool AdvMemOpt::collectOperandInst(SmallPtrSetImpl<Instruction *> &Set, Instruction *Inst,
                                   BasicBlock *LeadingBlock) const {
  for (Value *V : Inst->operands()) {
    Instruction *I = dyn_cast<Instruction>(V);
    if (!I)
      continue;
    if (isa<PHINode>(I) && IGC_IS_FLAG_ENABLED(AdvMemOptAggressiveHoist)) {
      // A PHI cannot be moved, but need not be when it is already live at the destination.
      if (DT->dominates(I->getParent(), LeadingBlock))
        continue;
      return true;
    }
    if (isa<PHINode>(I) || I->mayHaveSideEffects() || I->mayReadOrWriteMemory())
      return true;
    if (I->getParent() != Inst->getParent()) {
      // moving load instruction can be done only if operands
      // comes from the same basic block or a dominator of
      // the destination basic block. The condition is required
      // to counteract using uninitialized or wrong filled registers
      if (DT->dominates(I->getParent(), LeadingBlock))
        continue;
      else
        return true;
    }
    if (collectOperandInst(Set, I, LeadingBlock))
      return true;
  }
  Set.insert(Inst);
  return false;
}

bool AdvMemOpt::collectTrivialUser(SmallPtrSetImpl<Instruction *> &Set, Instruction *Inst) const {
  for (auto *U : Inst->users()) {
    Instruction *I = dyn_cast<Instruction>(U);
    if (!I || I->getParent() != Inst->getParent())
      continue;
    if (!isa<BitCastInst>(I) && !isa<ExtractElementInst>(I))
      continue;
    if (collectTrivialUser(Set, I))
      return true;
  }
  Set.insert(Inst);
  return false;
}

// Registers held by the values live out of 'Lead', charging each value a whole
// GRF of its own.
//
// getMaxRegCountForBB() instead sums all live values in bytes and rounds once,
// which reads a crowd of uniform values as nearly empty: a uniform i32 counts 4
// bytes, so a GRF's worth of them looks like one register even though the
// allocator gives each its own. That crowd is what this pass leaves behind.
unsigned AdvMemOpt::occupiedRegisters(BasicBlock &Lead) const {
  auto OI = RPE->getOutSet().find(&Lead);
  if (OI == RPE->getOutSet().end())
    return 0;

  const DataLayout &DL = Lead.getModule()->getDataLayout();
  unsigned Registers = 0;
  for (Value *V : OI->second)
    Registers += RPE->bytesToRegisters(RPE->computeSizeInBytes(V, SIMD, &WI->Runner, DL));
  return Registers;
}

// Register headroom at 'Lead'. Hoisting stretches a live range up to 'Lead', so
// the cost lands on whatever is already live there.
//
// Occupancy is the worse of two readings: the byte estimate, which catches
// lane-varying pressure peaking mid-block, and the live-out count, which catches
// the uniform crowd the byte estimate reads as free. Measured once per lead; the
// caller then subtracts what it spends.
unsigned AdvMemOpt::hoistBudgetInRegisters(BasicBlock *Lead) const {
  auto *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  const unsigned GRFBudget = Ctx->getNumGRFPerThread(true, Lead->getParent());
  if (!GRFBudget)
    return std::numeric_limits<unsigned>::max();

  const unsigned Reserved = GRFBudget / ReservedGRFDenominator;
  const unsigned Used = std::max(RPE->getMaxRegCountForBB(*Lead, SIMD, &WI->Runner), occupiedRegisters(*Lead));
  if (Used + Reserved >= GRFBudget)
    return 0;

  return GRFBudget - Reserved - Used;
}

bool AdvMemOpt::hoistInst(Instruction *LD, BasicBlock *BB, unsigned &BudgetInRegisters) const {
  SmallPtrSet<Instruction *, 32> ToHoist;
  if (collectOperandInst(ToHoist, LD, BB))
    return false;
  if (collectTrivialUser(ToHoist, LD))
    return false;

  // 'ToHoist' is the complete move set, so charge the budget before moving
  // anything. Round each value up to a whole register, matching how occupancy is
  // counted at the lead.
  const DataLayout &DL = LD->getModule()->getDataLayout();
  unsigned CostInRegisters = 0;
  for (Instruction *I : ToHoist)
    CostInRegisters += RPE->bytesToRegisters(RPE->computeSizeInBytes(I, SIMD, &WI->Runner, DL));
  if (CostInRegisters > BudgetInRegisters) {
    LLVM_DEBUG(dbgs() << " - - Out of register headroom at the lead block. Bail out.\n");
    return false;
  }
  BudgetInRegisters -= CostInRegisters;

  BasicBlock *FromBB = LD->getParent();
  Instruction *Pos = BB->getTerminator();
  for (auto II = IGCLLVM::getFirstNonPHI(FromBB)->getIterator(), IE = FromBB->end(); II != IE; /*EMPTY*/) {
    Instruction *I = &*II++;
    if (ToHoist.count(I)) {
      IGCLLVM::moveBefore(I, Pos);
      ToHoist.erase(I);
      if (ToHoist.empty())
        break;
    }
  }
  return true;
}

bool AdvMemOpt::hoistUniformLoad(ArrayRef<BasicBlock *> Line) const {
  bool Changed = false;
  LLVM_DEBUG(dbgs() << "Find the lead BB where to hoist uniform load.\n");

  auto BI = Line.begin();
  auto BE = Line.end();

  while (BI != BE) {
    if (!isLeadCandidate(*BI)) {
      ++BI;
      continue;
    }

    // Found lead.
    BasicBlock *Lead = *BI++;
    unsigned BudgetInRegisters = hoistBudgetInRegisters(Lead);
    LLVM_DEBUG(dbgs() << "Found lead to hoist to: " << Lead->getName() << " (budget " << BudgetInRegisters
                      << " registers)\n");

    for (; BI != BE; ++BI) {
      BasicBlock *Curr = *BI;
      LLVM_DEBUG(dbgs() << " - Try to hoist from: " << Curr->getName() << "\n");
      // Check whether it's safe to hoist uniform loads from Curr to Lead by
      // checking all blocks between Prev and Curr.
      if (hasMemoryWrite(Lead, Curr)) {
        LLVM_DEBUG(dbgs() << "- Memory write between Lead and Curr. Bail out.\n");
        break;
      }

      // Hoist uniform loads from Curr into Lead.
      for (auto II = IGCLLVM::getFirstNonPHI(Curr)->getIterator(), IE = Curr->end(); II != IE; /*EMPTY*/) {
        LLVM_DEBUG(dbgs() << " - - Try hoisting: " << *II << "\n");

        if (II->mayWriteToMemory()) {
          LLVM_DEBUG(dbgs() << " - - May write to memory. Bail out.\n");
          break;
        }

        std::optional<ALoadInst> LD = ALoadInst::get(&*II++);
        if (!LD.has_value() || !WI->isUniform(LD->inst())) {
          LLVM_DEBUG(dbgs() << " - - Not uniform load. Skip.\n");
          continue;
        }

        if (!hoistInst(LD->inst(), Lead, BudgetInRegisters)) {
          LLVM_DEBUG(dbgs() << " - - Uniform load not hoisted (unsafe or out of budget). Bail out.\n");
          break;
        }
        Changed = true;
        LLVM_DEBUG(dbgs() << " - - Hoisted!\n");

        // Reset iterator
        II = IGCLLVM::getFirstNonPHI(Curr)->getIterator();
      }

      // After hoisting uniform loads safely, if Curr has memory write, stop
      // hoisting further.
      if (hasMemoryWrite(Curr)) {
        LLVM_DEBUG(dbgs() << "- Curr has memory write. Bail out.\n");
        break;
      }
    }
  }

  return Changed;
}
