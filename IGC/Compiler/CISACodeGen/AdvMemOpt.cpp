/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Local.h>
#include "llvmWrapper/Transforms/Utils/LoopUtils.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/AdvMemOpt.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/PrepareLoadsStoresUtils.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace llvm::PatternMatch;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

    class AdvMemOpt : public FunctionPass {
        DominatorTree* DT;
        LoopInfo* LI;
        PostDominatorTree* PDT;
        ScalarEvolution* SE;
        WIAnalysis* WI;

    public:
        static char ID;

        AdvMemOpt() : FunctionPass(ID) {
            initializeAdvMemOptPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function& F) override;

        StringRef getPassName() const override { return "Advanced MemOpt"; }

    private:
        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addPreservedID(WIAnalysis::ID);
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<WIAnalysis>();
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<PostDominatorTreeWrapperPass>();
            AU.addRequired<ScalarEvolutionWrapperPass>();
        }

        bool collectOperandInst(SmallPtrSetImpl<Instruction*>&,
            Instruction*, BasicBlock*) const;
        bool collectTrivialUser(SmallPtrSetImpl<Instruction*>&,
            Instruction*) const;
        bool hoistUniformLoad(ArrayRef<BasicBlock*>) const;

        bool hoistInst(Instruction *inst, BasicBlock*) const;

        bool isLeadCandidate(BasicBlock*) const;

        bool hasMemoryWrite(BasicBlock* BB) const;
        bool hasMemoryWrite(BasicBlock* Entry, BasicBlock* Exit) const;
    };

    char AdvMemOpt::ID = 0;

} // End anonymous namespace

FunctionPass* IGC::createAdvMemOptPass() {
    return new AdvMemOpt();
}

#define PASS_FLAG     "igc-advmemopt"
#define PASS_DESC     "Advanced Memory Optimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(AdvMemOpt, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass);
    IGC_INITIALIZE_PASS_END(AdvMemOpt, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
} // End namespace IGC

bool AdvMemOpt::runOnFunction(Function& F) {
    // Skip non-kernel function.
    MetaDataUtils* MDU = nullptr;
    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    WI = &getAnalysis<WIAnalysis>();

    SmallVector<Loop*, 8> InnermostLoops;
    for (auto I = LI->begin(), E = LI->end(); I != E; ++I)
        for (auto DFI = df_begin(*I), DFE = df_end(*I); DFI != DFE; ++DFI) {
            Loop* L = *DFI;
            if (IGCLLVM::isInnermost(L))
                InnermostLoops.push_back(L);
        }

    for (Loop* L : InnermostLoops) {
        SmallVector<BasicBlock*, 8> Line;
        BasicBlock* BB = L->getHeader();
        while (BB) {
            Line.push_back(BB);
            BasicBlock* CurrBB = BB;
            BB = nullptr;
            for (auto BI = succ_begin(CurrBB),
                BE = succ_end(CurrBB); BI != BE; ++BI) {
                BasicBlock* OtherBB = *BI;
                if (CurrBB == OtherBB || !L->contains(OtherBB))
                    continue;
                if (DT->dominates(CurrBB, OtherBB) && PDT->dominates(OtherBB, CurrBB)) {
                    BB = OtherBB;
                    break;
                }
            }
        }
        hoistUniformLoad(Line);
    }

    auto* Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (Ctx->platform.isProductChildOf(IGFX_DG2)) {
        // split 64-bit uniform store into <2 x i32>, so it has better chance
        // to merge with other i32 stores in order to form 16-byte stores that
        // can use L1 cache
        auto& DL = F.getParent()->getDataLayout();
        IRBuilder<> IRB(F.getContext());
        for (auto II = inst_begin(&F), EI = inst_end(&F); II != EI; /* empty */) {
            auto* I = &*II++;
            if (auto* SI = dyn_cast<StoreInst>(I)) {
                if (!WI->isUniform(SI))
                    continue;

                unsigned AS = SI->getPointerAddressSpace();
                if (AS != ADDRESS_SPACE_PRIVATE &&
                    AS != ADDRESS_SPACE_GLOBAL)
                    continue;

                IRB.SetInsertPoint(SI);

                if (auto NewSI = expand64BitStore(IRB, DL, SI)) {
                    WI->incUpdateDepend(NewSI, WIAnalysis::UNIFORM_THREAD);
                    WI->incUpdateDepend(NewSI->getValueOperand(), WIAnalysis::UNIFORM_THREAD);
                    WI->incUpdateDepend(NewSI->getPointerOperand(), WIAnalysis::UNIFORM_THREAD);
                    SI->eraseFromParent();
                }
            }
        }
    }

    // hoist input-based sample instructions in pixel shader into Entry-Block
    unsigned MaxSampleHoisted = IGC_GET_FLAG_VALUE(PixelSampleHoistingLimit);
    if (Ctx->type == ShaderType::PIXEL_SHADER && MaxSampleHoisted)
    {
        auto DL = F.getParent()->getDataLayout();
        BasicBlock& EntryBlk = F.getEntryBlock();
        unsigned LiveOutPressure = 0;
        unsigned NumSampleInsts = 0;
        unsigned NumOtherInsts = 0;
        unsigned MaxLevelDeps = 0;
        std::map<Instruction*, unsigned> InstDepLevel;
        // check several parameters as the indicator of sampler hoisting:
        // the number of sample instructions
        // the number of other instructions
        // the level of dependencies on sample instructions
        // the live-out registers in the entry-block
        for (auto BI = EntryBlk.begin(), BE = EntryBlk.end(); BI != BE; ++BI)
        {
            auto Inst = &*BI;
            // compute instruction's dependency level
            unsigned DepLevel = 0;
            for (Value* Opnd : Inst->operands())
            {
                if (auto SrcI = dyn_cast<Instruction>(Opnd))
                {
                    if (InstDepLevel.find(SrcI) != InstDepLevel.end())
                    {
                        DepLevel = max(DepLevel, InstDepLevel[SrcI]);
                    }
                }
            }
            if (isSampleLoadGather4InfoInstruction(Inst))
            {
                NumSampleInsts++;
                DepLevel++;  // bump up the dependency-level
            }
            else
                NumOtherInsts++;
            InstDepLevel[Inst] = DepLevel;
            MaxLevelDeps = max(MaxLevelDeps, DepLevel);
            if (Inst->isUsedOutsideOfBlock(&EntryBlk))
            {
                LiveOutPressure += (uint)(DL.getTypeAllocSize(Inst->getType()));
            }
        }
        // hoist sampler instructions from the blocks post-dominiating the entry block
        if ((NumSampleInsts * 33) >= NumOtherInsts &&
            MaxLevelDeps >= 2 && LiveOutPressure <= 96)
        {
            auto Node = PDT->getNode(&EntryBlk);
            unsigned NumSampleHoisted = 0;
            while (Node->getIDom() && NumSampleHoisted < MaxSampleHoisted)
            {
                Node = Node->getIDom();
                BasicBlock* SuccBlk = Node->getBlock();
                auto BI = SuccBlk->begin();
                auto BE = SuccBlk->end();
                auto Inst = &*BI;
                while (!isSampleLoadGather4InfoInstruction(Inst) && BI != BE)
                {
                    Inst = &*BI++;
                }
                while (BI != BE && NumSampleHoisted < MaxSampleHoisted)
                {
                    auto NextCand = &*BI++;
                    while (!isSampleLoadGather4InfoInstruction(NextCand) && BI != BE)
                    {
                        NextCand = &*BI++;
                    }
                    if (hoistInst(Inst, &EntryBlk))
                    {
                        NumSampleHoisted++;
                    }
                    if (BI != BE)
                        Inst = NextCand;
                }
            }
        }
    }
    return false;
}

bool AdvMemOpt::isLeadCandidate(BasicBlock* BB) const {
    // A candidate lead should have at least one uniform loads. In addition,
    // there's no instruction might to write memory from the last uniform loads
    // to the end.
    for (auto II = BB->rbegin(), IE = BB->rend(); II != IE; ++II) {
        if (II->mayWriteToMemory())
            return false;
        LoadInst* LD = dyn_cast<LoadInst>(&*II);
        if (!LD || !WI->isUniform(LD))
            continue;
        // Found uniform loads.
        return true;
    }
    return false;
}

namespace {
    class RegionSubgraph {
        BasicBlock* Exit;
        SmallPtrSet<BasicBlock*, 32> Visited;

    public:
        RegionSubgraph(BasicBlock* E) : Exit(E) {}

        bool preVisit(Optional<BasicBlock*> From, BasicBlock* To) {
            if (To == Exit)
                return false;
            return Visited.insert(To).second;
        }
    };
} // End anonymous namespace

namespace llvm {
    template<>
    class po_iterator_storage<RegionSubgraph, true> {
        RegionSubgraph& RSG;

    public:
        po_iterator_storage(RegionSubgraph& G) : RSG(G) {}

        bool insertEdge(Optional<BasicBlock*> From, BasicBlock* To) {
            return RSG.preVisit(From, To);
        }
        void finishPostorder(BasicBlock*) {}
    };
} // End llvm namespace

bool AdvMemOpt::hasMemoryWrite(BasicBlock* BB) const {
    for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
        if (II->mayWriteToMemory())
            return true;
    return false;
}

bool AdvMemOpt::hasMemoryWrite(BasicBlock* Entry, BasicBlock* Exit) const {
    // Entry and Exit must be on line of code.
    IGC_ASSERT(nullptr != DT);
    IGC_ASSERT(DT->dominates(Entry, Exit));
    IGC_ASSERT(nullptr != PDT);
    IGC_ASSERT(PDT->dominates(Exit, Entry));

    RegionSubgraph RSG(Exit);
    for (auto SI = po_ext_begin(Entry, RSG),
        SE = po_ext_end(Entry, RSG); SI != SE; ++SI)
        if (*SI != Entry && hasMemoryWrite(*SI))
            return true;
    return false;
}

bool AdvMemOpt::collectOperandInst(SmallPtrSetImpl<Instruction*>& Set,
    Instruction* Inst, BasicBlock* LeadingBlock) const {
    for (Value* V : Inst->operands()) {
        Instruction* I = dyn_cast<Instruction>(V);
        if (!I)
            continue;
        if (isa<PHINode>(I) ||
            I->mayHaveSideEffects() ||
            I->mayReadOrWriteMemory())
            return true;
        if (I->getParent() != Inst->getParent())
        {
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

bool AdvMemOpt::collectTrivialUser(SmallPtrSetImpl<Instruction*>& Set,
    Instruction* Inst) const {
    for (auto* U : Inst->users()) {
        Instruction* I = dyn_cast<Instruction>(U);
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

bool AdvMemOpt::hoistInst(Instruction* LD, BasicBlock* BB) const {
    SmallPtrSet<Instruction*, 32> ToHoist;
    if (collectOperandInst(ToHoist, LD, BB))
        return false;
    if (collectTrivialUser(ToHoist, LD))
        return false;
    BasicBlock* FromBB = LD->getParent();
    Instruction* Pos = BB->getTerminator();
    for (auto II = FromBB->getFirstNonPHI()->getIterator(),
        IE = FromBB->end(); II != IE; /*EMPTY*/) {
        Instruction* I = &*II++;
        if (ToHoist.count(I)) {
            I->moveBefore(Pos);
            ToHoist.erase(I);
            if (ToHoist.empty())
                break;
        }
    }
    return true;
}

bool AdvMemOpt::hoistUniformLoad(ArrayRef<BasicBlock*> Line) const {
    bool Changed = false;
    // Find the lead BB where to hoist uniform load.
    auto BI = Line.begin();
    auto BE = Line.end();
    while (BI != BE) {
        if (!isLeadCandidate(*BI)) {
            ++BI;
            continue;
        }
        // Found lead.
        BasicBlock* Lead = *BI++;
        BasicBlock* Prev = Lead;
        for (; BI != BE; ++BI) {
            BasicBlock* Curr = *BI;
            // Check whether it's safe to hoist uniform loads from Curr to Lead by
            // checking all blocks between Prev and Curr.
            if (hasMemoryWrite(Prev, Curr))
                break;
            // Hoist uniform loads from Curr into Lead.
            for (auto II = Curr->getFirstNonPHI()->getIterator(),
                IE = Curr->end(); II != IE; /*EMPTY*/) {
                if (II->mayWriteToMemory())
                    break;
                LoadInst* LD = dyn_cast<LoadInst>(&*II++);
                if (!LD || !WI->isUniform(LD))
                    continue;
                if (!hoistInst(LD, Lead))
                    break; // Bail out if any uniform load could not be hoisted safely.
                  // Reset iterator
                II = Curr->getFirstNonPHI()->getIterator();
                Changed = true;
            }
            // After hoisting uniform loads safely, if Curr has memory write, stop
            // hoisting further.
            if (hasMemoryWrite(Curr))
                break;
        }
    }
    return Changed;
}
