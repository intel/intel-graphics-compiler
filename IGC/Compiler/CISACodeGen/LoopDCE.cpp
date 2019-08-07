#define DEBUG_TYPE "slm-blocking"
#include "Compiler/CISACodeGen/LoopDCE.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsics.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

    class LoopDeadCodeElimination : public FunctionPass {
        CodeGenContext* CGC;
        const DataLayout* DL;
        LoopInfo* LI;
        PostDominatorTree* PDT;

    public:
        static char ID;

        LoopDeadCodeElimination() : FunctionPass(ID),
            CGC(nullptr), DL(nullptr), LI(nullptr), PDT(nullptr) {
            initializeLoopDeadCodeEliminationPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<PostDominatorTreeWrapperPass>();
            AU.addPreserved<PostDominatorTreeWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addPreserved<LoopInfoWrapperPass>();
        }

    private:
        bool processLoop(Loop* L);
    };

    /// This is to remove any recursive PHINode. For example,
    ///   Bx:
    ///     x = phi [] ...[y, By]
    ///   By:
    ///     y = phi [] ... [x, Bx]
    /// Both x and y are only used by PHINode, thus they can be
    /// removed.  This recursive PHINodes happens only if there
    /// are loops, and could be introduced in SROA.
    ///
    /// This pass detects the cases above and remove those PHINodes
    class DeadPHINodeElimination : public FunctionPass {
    public:
        static char ID;

        DeadPHINodeElimination() : FunctionPass(ID)
        {
            initializeDeadPHINodeEliminationPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }
    };

} // End anonymous namespace

char LoopDeadCodeElimination::ID = 0;

#define PASS_FLAG     "igc-loop-dce"
#define PASS_DESC     "Advanced DCE on loop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(LoopDeadCodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_END(LoopDeadCodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

FunctionPass* IGC::createLoopDeadCodeEliminationPass() {
    return new LoopDeadCodeElimination();
}

bool LoopDeadCodeElimination::runOnFunction(Function& F) {
    // Skip non-kernel function.
    MetaDataUtils* MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    DL = &F.getParent()->getDataLayout();
    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

    bool Changed = false;
    // DFT all loops.
    for (auto I = LI->begin(), E = LI->end(); I != E; ++I)
        for (auto L = df_begin(*I), F = df_end(*I); L != F; ++L)
            Changed |= processLoop(*L);

    return Changed;
}

static Instruction* getSingleUserInLoop(Value* V, Loop* L) {
    Instruction* UserInLoop = nullptr;
    for (auto U : V->users()) {
        auto I = dyn_cast<Instruction>(U);
        if (!I)
            return nullptr;
        if (!L->contains(I->getParent()))
            continue;
        if (UserInLoop)
            return nullptr;
        UserInLoop = I;
    }
    return UserInLoop;
}

bool LoopDeadCodeElimination::processLoop(Loop* L) {
    SmallVector<BasicBlock*, 8> ExitingBlocks;
    L->getExitingBlocks(ExitingBlocks);
    if (ExitingBlocks.empty())
        return false;

    bool Changed = false;
    for (auto BB : ExitingBlocks) {
        auto BI = dyn_cast<BranchInst>(BB->getTerminator());
        // Skip exiting block with non-conditional branch.
        if (!BI || !BI->isConditional())
            continue;
        bool ExitingOnTrue = !L->contains(BI->getSuccessor(0));
        auto Cond = BI->getCondition();
        for (auto U : Cond->users()) {

            auto SI = dyn_cast<SelectInst>(U);

            //Check that 'select' instruction is within the loop and also that the 
            //Branch condition is used as the select's condition and not as the select's
            //true or false value
            if (!SI || !L->contains(SI->getParent()) || SI->getCondition() != Cond)
                continue;
            // TODO: Handle the trivial case where 'select' is used as a loop-carried
            // value.
            auto I = getSingleUserInLoop(SI, L);
            if (!I)
                continue;
            auto PN = dyn_cast<PHINode>(I);
            if (!PN || L->getHeader() != PN->getParent())
                continue;
            // v2 := phi(v0/bb0, v1/bb1)
            // ...
            // bb1:
            // v1 := select(cond, va, vb);
            // br cond, out_of_loop
            //
            // replace all uses of v1 in loop with vb
            // replace all uses of v1 out of loop with va
            Value* NewValInLoop = SI->getFalseValue();
            Value* NewValOutLoop = SI->getTrueValue();
            if (!ExitingOnTrue)
                std::swap(NewValInLoop, NewValOutLoop);
            for (auto UI = SI->use_begin(), UE = SI->use_end(); UI != UE; /*EMPTY*/) {
                auto& Use = *UI++;
                auto I = cast<Instruction>(Use.getUser());
                auto NewVal =
                    L->contains(I->getParent()) ? NewValInLoop : NewValOutLoop;
                Use.set(NewVal);
            }
        }
    }
    return Changed;
}


/// DeadPHINodeElimination
char DeadPHINodeElimination::ID = 0;

#undef PASS_FLAG
#undef PASS_DESC
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

#define PASS_FLAG     "igc-phielimination"
#define PASS_DESC     "Remove dead recurisive PHINode"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(DeadPHINodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_END(DeadPHINodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

FunctionPass* IGC::createDeadPHINodeEliminationPass() {
    return new DeadPHINodeElimination();
}

bool DeadPHINodeElimination::runOnFunction(Function& F)
{
    // This is to eliminate potential recurive phi like the following:
    //  Bx:   x = phi [y0, B0], ...[z, Bz]
    //  Bz:   z = phi [x, Bx], ...[]
    //  x and z are recursive phi's that are not used anywhere but in those phi's.
    //  Thus, they can be eliminated.
    auto phiUsedOnlyByPhi = [](PHINode* P) {
        bool ret = true;
        for (auto U : P->users())
        {
            Instruction* Inst = dyn_cast<Instruction>(U);
            if (!Inst || !isa<PHINode>(Inst)) {
                ret = false;
                break;
            }
        }
        return ret;
    };

    DenseMap<PHINode*, int> candidates;
    for (auto& BI : F)
    {
        BasicBlock* BB = &BI;
        for (auto& II : *BB)
        {
            PHINode* Phi = dyn_cast<PHINode>(&II);
            if (Phi && phiUsedOnlyByPhi(Phi)) {
                candidates[Phi] = 1;
            }
            else if (!Phi) {
                // No more phi, stop looping.
                break;
            }
        }
    }

    /// Iteratively removing non-candidate PHINode by
    /// setting its map value to zero.
    bool changed;
    do
    {
        changed = false;
        for (auto MI = candidates.begin(), ME = candidates.end(); MI != ME; ++MI)
        {
            PHINode* P = MI->first;
            if (MI->second == 0)
                continue;

            for (auto U : P->users()) {
                PHINode* phiUser = dyn_cast<PHINode>(U);
                assert(phiUser && "ICE: all candidates should have phi as its users!");
                auto iter = candidates.find(phiUser);
                if (iter == candidates.end())
                {
                    // not candidate as its user is not in the map.
                    MI->second = 0;
                    changed = true;
                    break;
                }

                // If it is the user of itself, skip.
                if (iter->first == MI->first) {
                    continue;
                }

                if (iter->second == 0)
                {
                    // not candidate as being used by a non-candidate phi
                    MI->second = 0;
                    changed = true;
                    break;
                }
            }
        }
    } while (changed);

    // Prepare phi for deletion by setting its operands to null
    SmallVector<PHINode*, 8> toBeDeleted;
    for (auto& MI : candidates)
    {
        PHINode* P = MI.first;
        if (MI.second == 0)
            continue;
        // reset its operands to zero, which will eventually
        // make all dead Phi's uses to be empty!
        Value* nilVal = Constant::getNullValue(P->getType());
        for (int i = 0, e = (int)P->getNumIncomingValues(); i < e; ++i)
        {
            P->setIncomingValue(i, nilVal);
        }
        toBeDeleted.push_back(P);
    }

    // Actually delete them.
    for (int i = 0, e = (int)toBeDeleted.size(); i < e; ++i)
    {
        PHINode* P = toBeDeleted[i];
        P->eraseFromParent();
    }

    return (toBeDeleted.size() > 0);
}
