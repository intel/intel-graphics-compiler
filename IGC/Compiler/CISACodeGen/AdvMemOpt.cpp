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
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/AdvMemOpt.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

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
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<WIAnalysis>();
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<PostDominatorTreeWrapperPass>();
            AU.addRequired<ScalarEvolutionWrapperPass>();
        }

        bool collectOperandInst(SmallPtrSetImpl<Instruction*>&,
            Instruction*) const;
        bool collectTrivialUser(SmallPtrSetImpl<Instruction*>&,
            Instruction*) const;
        bool hoistUniformLoad(LoadInst*, BasicBlock*) const;
        bool hoistUniformLoad(ArrayRef<BasicBlock*>) const;

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
    MetaDataUtils* MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
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
            if (L->empty())
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
        if (!LD || WI->whichDepend(LD) != WIAnalysis::UNIFORM)
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
    assert(DT->dominates(Entry, Exit) && PDT->dominates(Exit, Entry));
    RegionSubgraph RSG(Exit);
    for (auto SI = po_ext_begin(Entry, RSG),
        SE = po_ext_end(Entry, RSG); SI != SE; ++SI)
        if (*SI != Entry && hasMemoryWrite(*SI))
            return true;
    return false;
}

bool AdvMemOpt::collectOperandInst(SmallPtrSetImpl<Instruction*>& Set,
    Instruction* Inst) const {
    for (Value* V : Inst->operands()) {
        Instruction* I = dyn_cast<Instruction>(V);
        if (!I || I->getParent() != Inst->getParent())
            continue;
        if (isa<PHINode>(I) || collectOperandInst(Set, I))
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

bool AdvMemOpt::hoistUniformLoad(LoadInst* LD, BasicBlock* BB) const {
    SmallPtrSet<Instruction*, 32> ToHoist;
    if (collectOperandInst(ToHoist, LD))
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
                if (!LD || WI->whichDepend(LD) != WIAnalysis::UNIFORM)
                    continue;
                if (!hoistUniformLoad(LD, Lead))
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
