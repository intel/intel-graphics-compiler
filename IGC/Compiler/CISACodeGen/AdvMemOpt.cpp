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
#include <llvm/ADT/Optional.h>
#include "llvmWrapper/Analysis/TargetLibraryInfo.h"
#include "llvmWrapper/Transforms/Utils/LoopUtils.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/MemOpt2.h"
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

#define DEBUG_TYPE "AdvMemOpt"

namespace {

    class AdvMemOpt : public FunctionPass {
        DominatorTree* DT = nullptr;
        LoopInfo* LI = nullptr;
        PostDominatorTree* PDT = nullptr;
        WIAnalysis* WI = nullptr;
        TargetLibraryInfo* TLI = nullptr;

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
            AU.addRequired<TargetLibraryInfoWrapperPass>();
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

        MemInstCluster Cluster;
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
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass);
        IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
    IGC_INITIALIZE_PASS_END(AdvMemOpt, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
} // End namespace IGC

bool AdvMemOpt::runOnFunction(Function& F) {
    bool Changed = false;

    // Skip non-kernel function.
    MetaDataUtils* MDU = nullptr;
    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    WI = &getAnalysis<WIAnalysis>();
    TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

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
        Changed |= hoistUniformLoad(Line);
    }

    auto* Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (Ctx->platform.isProductChildOf(IGFX_DG2)) {
        // 1) split 64-bit uniform store into <2 x i32>, so it has better chance
        // to merge with other i32 stores in order to form 16-byte stores that
        // can use L1 cache.
        // 2) count the number of sample operations that use lane-varying
        // resource or sampler state. We need to apply mem-inst-clustering
        // because, once ballot-loop is added, vISA finalizer cannot schedule
        // those sample operations.
        auto& DL = F.getParent()->getDataLayout();
        IGCIRBuilder<> IRB(F.getContext());
        Cluster.init(Ctx, &DL, nullptr/*AA*/, TLI, 32);
        for (Function::iterator I = F.begin(), E = F.end(); I != E;
             ++I) {
            BasicBlock *BB = &*I;
            unsigned NumResourceVarying = 0;
            bool HasStore = false;
            for (BasicBlock::iterator II = BB->begin(), EI = BB->end();
                 II != EI;
                 /*empty*/) {
                Instruction *I = &*II++;
                if (I->mayWriteToMemory())
                    HasStore = true;
                if (auto *SI = dyn_cast<SampleIntrinsic>(I)) {
                    if (!WI->isUniform(SI->getTextureValue()) ||
                        !WI->isUniform(SI->getSamplerValue())) {
                        NumResourceVarying++;
                    }
                } else if (auto *GI = dyn_cast<SamplerGatherIntrinsic>(I)) {
                    if (!WI->isUniform(GI->getTextureValue()) ||
                        !WI->isUniform(GI->getSamplerValue())) {
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
                    if (AS != ADDRESS_SPACE_PRIVATE &&
                        AS != ADDRESS_SPACE_GLOBAL)
                      continue;

                    IRB.SetInsertPoint(SI->inst());

                    if (auto NewSI = expand64BitStore(IRB, DL, SI.value())) {
                      auto NewASI = AStoreInst::get(NewSI);
                      WI->incUpdateDepend(NewSI, WIAnalysis::UNIFORM_THREAD);
                      WI->incUpdateDepend(NewASI->getValueOperand(),
                                          WIAnalysis::UNIFORM_THREAD);
                      WI->incUpdateDepend(NewASI->getPointerOperand(),
                                          WIAnalysis::UNIFORM_THREAD);
                      SI->inst()->eraseFromParent();
                      Changed = true;
                    }
                }
            }
            // If a basic-block has lane-varying resource access
            if (NumResourceVarying) {
                Ctx->m_instrTypes.numSamplesVaryingResource +=
                    NumResourceVarying;
                // clustering method cannot handle memory dependence
                if (!HasStore)
                    Changed |= Cluster.runForGFX(BB);
            }
        }
    }
    return Changed;
}

bool AdvMemOpt::isLeadCandidate(BasicBlock* BB) const {
    // A candidate lead should have at least one uniform load. In addition,
    // there's no instruction might to write memory from the last uniform loads
    // to the end.
    LLVM_DEBUG(dbgs() << "Check lead candidate: " << BB->getName() << "\n");
    for (auto II = BB->rbegin(), IE = BB->rend(); II != IE; ++II) {
        if (II->mayWriteToMemory()) {
            LLVM_DEBUG(dbgs() <<" - May write to memory. Bail out: " << *II << "\n");
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
        BasicBlock* Exit;
        SmallPtrSet<BasicBlock*, 32> Visited;

    public:
        RegionSubgraph(BasicBlock* E) : Exit(E) {}

        bool preVisit(llvm::Optional<BasicBlock*> From, BasicBlock* To) {
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

        bool insertEdge(llvm::Optional<BasicBlock*> From, BasicBlock* To) {
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
    LLVM_DEBUG(dbgs() << "Find the lead BB where to hoist uniform load.\n");

    auto BI = Line.begin();
    auto BE = Line.end();

    while (BI != BE) {
        if (!isLeadCandidate(*BI)) {
            ++BI;
            continue;
        }

        // Found lead.
        BasicBlock* Lead = *BI++;
        LLVM_DEBUG(dbgs() << "Found lead to hoist to: " << Lead->getName() << "\n");

        for (; BI != BE; ++BI) {
            BasicBlock* Curr = *BI;
            LLVM_DEBUG(dbgs() << " - Try to hoist from: " << Curr->getName() << "\n");
            // Check whether it's safe to hoist uniform loads from Curr to Lead by
            // checking all blocks between Prev and Curr.
            if (hasMemoryWrite(Lead, Curr)) {
                LLVM_DEBUG(dbgs() << "- Memory write between Lead and Curr. Bail out.\n");
                break;
            }

            // Hoist uniform loads from Curr into Lead.
            for (auto II = Curr->getFirstNonPHI()->getIterator(),
                IE = Curr->end(); II != IE; /*EMPTY*/) {
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

                if (!hoistInst(LD->inst(), Lead)) {
                    LLVM_DEBUG(dbgs() << " - - Uniform load could not be hoisted safely. Bail out.\n");
                    break;
                }
                Changed = true;
                LLVM_DEBUG(dbgs() << " - - Hoisted!\n");

                // Reset iterator
                II = Curr->getFirstNonPHI()->getIterator();
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
