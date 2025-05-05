/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "pre-ra-remat-flag"
#include "Compiler/CISACodeGen/PreRARematFlag.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvmWrapper/ADT/Optional.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include <optional>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {
    typedef DominatorTreeBase<BasicBlock, false> DominatorTreeBasicBlock;

    class DominatedSubgraph {
        DominatorTreeBasicBlock* DT;
        BasicBlock* Entry;
        SmallPtrSet<BasicBlock*, 32> Visited;

    public:
        DominatedSubgraph(DominatorTreeBasicBlock* D, BasicBlock* N) : DT(D), Entry(N) {}

        bool preVisit(std::optional<BasicBlock*> From, BasicBlock* To) {
            // Skip BB not dominated by the specified entry.
            if (!DT->dominates(Entry, To))
                return false;
            return Visited.insert(To).second;
        }
        void postVisit(BasicBlock*) {}
    };
} // End anonymous namespace

namespace llvm {
    template<>
    class po_iterator_storage<DominatedSubgraph, true> {
        DominatedSubgraph& DSG;

    public:
        po_iterator_storage(DominatedSubgraph& G) : DSG(G) {}

        bool insertEdge(llvm::Optional<BasicBlock*> From, BasicBlock* To) {
            return DSG.preVisit(IGCLLVM::makeOptional(From), To);
        }
        void finishPostorder(BasicBlock* BB) { DSG.postVisit(BB); }
    };
} // End llvm namespace

namespace {

    typedef IRBuilder<> BuilderType;

    class PreRARematFlag : public FunctionPass {
        class LivenessChecker;

        DominatorTree* DT;
        LoopInfo* LI;
        LivenessChecker* LC;

        BuilderType* IRB;

    public:
        static char ID;

        PreRARematFlag() : FunctionPass(ID), DT(nullptr), LI(nullptr), LC(nullptr),
            IRB(nullptr) {
            initializePreRARematFlagPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

    private:
        bool reMaterialize(Instruction*, User*) const;

        /// FIXME: Add checking of reducibility of CFG as we rely on that. Otherwise,
        /// we need complex algorithm to handle irreducible SCC.
        ///
        /// Online liveness checker based on
        /// @inproceedings{Boissinot:2008:FLC:1356058.1356064,
        ///  author = {Boissinot, Benoit and Hack, Sebastian and Grund, Daniel and Dupont de Dine hin, Beno\^{\i}t and Rastello, Fabri e},
        ///  title = {Fast Liveness Checking for Ssa-form Programs},
        ///  booktitle = {Proceedings of the 6th Annual IEEE/ACM International Symposium on Code Generation and Optimization},
        ///  series = {CGO '08},
        ///  year = {2008},
        ///  isbn = {978-1-59593-978-4},
        ///  location = {Boston, MA, USA},
        ///  pages = {35--44},
        ///  numpages = {10},
        ///  url = {http://doi.acm.org/10.1145/1356058.1356064},
        ///  doi = {10.1145/1356058.1356064},
        ///  acmid = {1356064},
        ///  publisher = {ACM},
        ///  address = {New York, NY, USA},
        ///  keywords = {compilers, dominance, jit-compilation, liveness analysis, ssa form},
        /// }
        /// Different from the origin paper without assumption of CFG reducibility,
        /// the algorithm here assumes the reducibility of CFG to simplify the liveness
        /// checking further by the simplied loop nesting forest.
        ///
        class LivenessChecker {
            DominatorTree* DT;
            LoopInfo* LI;

            /// Find the outermost loop which contains `To` but not `Def`. Return
            /// nullptr if there's no such loop.
            Loop* findOuterMostLoop(BasicBlock* To, BasicBlock* Def) const {
                Loop* PrevL = nullptr;
                Loop* L = LI->getLoopFor(To);
                while (L && !L->contains(Def)) {
                    PrevL = L;
                    L = L->getParentLoop();
                }
                if (PrevL && PrevL->contains(Def))
                    return nullptr;
                return PrevL;
            }

            /// Is `To` reachable from `From` under the dominance of `Def`?
            bool isReachableUnderDominance(BasicBlock* From, BasicBlock* To,
                BasicBlock* Def) const {
                if (!DT->dominates(Def, From) || !DT->dominates(Def, To))
                    return false;

                // Adjust `From`/`To` blocks to loop header if there is such outermost
                // loop.
                if (Loop * L = findOuterMostLoop(To, Def))
                    To = L->getHeader();
                if (Loop * L = findOuterMostLoop(From, Def))
                    From = L->getHeader();

                // Check reachability under the dominance of `Def`.
                DominatedSubgraph DSG(DT, Def);
                for (auto SI = po_ext_begin(From, DSG),
                    SE = po_ext_end(From, DSG); SI != SE; ++SI)
                    if (*SI == To)
                        return true;

                return false;
            }

        public:
            LivenessChecker(DominatorTree* D, LoopInfo* L) : DT(D), LI(L) {}

            bool isLive(Instruction* I, BasicBlock* BB) const {
                BasicBlock* DefBB = I->getParent();

                if (!DT->dominates(DefBB, BB))
                    return false;

                for (auto* U : I->users())
                    if (isReachableUnderDominance(BB, cast<Instruction>(U)->getParent(),
                        DefBB))
                        return true;

                return false;
            }
        };
    };

} // End anonymous namespace

FunctionPass* IGC::createPreRARematFlagPass() {
    return new PreRARematFlag();
}

char PreRARematFlag::ID = 0;

#define PASS_FLAG     "igc-pre-ra-remat-flag"
#define PASS_DESC     "PreRA rematerialize flag"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(PreRARematFlag, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_END(PreRARematFlag, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

bool PreRARematFlag::runOnFunction(Function& F) {
    // Skip non-kernel function.
    MetaDataUtils* MDU = nullptr;
    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    LivenessChecker TheChecker(DT, LI);
    LC = &TheChecker;

    BuilderType TheBuilder(F.getContext());
    IRB = &TheBuilder;

    bool Changed = false;
    for (auto& BB : F) {
        BranchInst* BI = dyn_cast<BranchInst>(BB.getTerminator());
        if (!BI || !BI->isConditional())
            continue;

        // DFS traverse logic expression to remove multi-use boolean values.
        SmallVector<std::pair<Value*, User*>, 16> WorkList;
        WorkList.push_back(std::make_pair(BI->getCondition(), nullptr));
        while (!WorkList.empty()) {
            auto [V, LocalUser] = WorkList.back();
            WorkList.pop_back();
            if (CmpInst * Cmp = dyn_cast<CmpInst>(V)) {
                Changed |= reMaterialize(Cmp, LocalUser);
            }
            else if (BinaryOperator * BO = dyn_cast<BinaryOperator>(V)) {
                switch (BO->getOpcode()) {
                default:
                    break;
                case Instruction::And:
                case Instruction::Or:
                case Instruction::Xor: {
                    Changed |= reMaterialize(BO, LocalUser);
                    Value* LHS = BO->getOperand(0);
                    Value* RHS = BO->getOperand(1);
                    if (!isa<Constant>(LHS))
                        WorkList.push_back(std::make_pair(LHS, BO));
                    if (!isa<Constant>(RHS))
                        WorkList.push_back(std::make_pair(RHS, BO));
                    break;
                }
                }
            }
        }
    }

    return Changed;
}

bool PreRARematFlag::reMaterialize(Instruction* I, User* LocalUser) const {
    if (!LocalUser)
        return false;

    // Skip if the instruction is already single-used.
    if (I->hasOneUse())
        return false;

    if (BinaryOperator * BO = dyn_cast<BinaryOperator>(I))
        switch (BO->getOpcode()) {
        default:
            return false;
        case Instruction::And:
        case Instruction::Or:
        case Instruction::Xor:
            break;
        }
    else if (!isa<CmpInst>(I))
        return false;

    IGC_ASSERT(I->getNumOperands() == 2);

    Instruction* Op0 = dyn_cast<Instruction>(I->getOperand(0));
    Instruction* Op1 = dyn_cast<Instruction>(I->getOperand(1));

    bool Changed = false;
    for (auto UI = I->use_begin(),
        UE = I->use_end(); UI != UE; /* EMPTY */) {
        Use& U = *UI++;
        if (U.getUser() == LocalUser)
            continue;
        BasicBlock* BB = cast<Instruction>(U.getUser())->getParent();
        if ((Op0 && !LC->isLive(Op0, BB)) &&
            (Op1 && !LC->isLive(Op1, BB)))
            continue;
        // Clone this use.
        BuilderType::InsertPointGuard Guard(*IRB);
        Instruction* InsertPt = cast<Instruction>(U.getUser());
        if (PHINode * PN = dyn_cast<PHINode>(InsertPt)) {
            BasicBlock* PredBB = PN->getIncomingBlock(U);
            IGC_ASSERT(U != PredBB->getTerminator());
            InsertPt = PredBB->getTerminator();
        }
        Instruction* Clone = I->clone();
        Clone->insertBefore(InsertPt);
        U.set(Clone);
        Changed = true;
    }

    return Changed;
}
