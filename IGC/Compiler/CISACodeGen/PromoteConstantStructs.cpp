/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/PromoteConstantStructs.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/PtrUseVisitor.h"
#include "llvm/Analysis/CFG.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

namespace {

    // This class intends to promote constants that are saved into large structs
    // e.g. specialization constants
    //
    // This task is not done by SROA because this structs can be used(loaded) with
    // non-constant indices if they have an array inside
    //
    // It is not done by GVN either because GVN uses MemoryDependenceAnalysis which
    // does not look deeper than 100 instructions to get a memdep which does not
    // work for large structures. This parameter can not be tweaked inside compiler
    //
    // Restrictions to this implementation to keep it simple and save compile time:
    // -- The structure does not escape
    // -- We promote only constant values of the same type and store->load only
    //
    // How it works:
    // -- iterate over allocas to check stores and collect potential loads
    // -- iterate over loads and use MemDep to find defining store

    class PromoteConstantStructs : public FunctionPass {

    public:
        static char ID;

        PromoteConstantStructs() : FunctionPass(ID) {
            initializePromoteConstantStructsPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function& F) override;

    private:

        const unsigned int InstructionsLimit = 1000;

        MemoryDependenceResults *MD = nullptr;
        DominatorTree *DT = nullptr;
        LoopInfo* LPI = nullptr;

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.addRequired<MemoryDependenceWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.setPreservesCFG();
            AU.addPreserved<DominatorTreeWrapperPass>();
        }

        bool processAlloca(AllocaInst &AI);

        bool processLoad(LoadInst *LI, SetVector<BasicBlock*>& StoreBBs);

    };

    char PromoteConstantStructs::ID = 0;

} // End anonymous namespace

llvm::FunctionPass* createPromoteConstantStructsPass() {
    return new PromoteConstantStructs();
}

#define PASS_FLAG     "igc-promote-constant-structs"
#define PASS_DESC     "Promote constant structs"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PromoteConstantStructs, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
INITIALIZE_PASS_DEPENDENCY(MemoryDependenceWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(PromoteConstantStructs, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

// This class visits all alloca uses to check that
// -- it does not escape
// and collect all loads with constant offsets from alloca

class AllocaChecker : public PtrUseVisitor<AllocaChecker> {
    friend class PtrUseVisitor<AllocaChecker>;
    friend class InstVisitor<AllocaChecker>;

public:
    AllocaChecker(const DataLayout &DL)
        : PtrUseVisitor<AllocaChecker>(DL), StoreBBs() {}

    SmallVector<LoadInst*, 8>& getPotentialLoads() {
        return Loads;
    }

    SetVector<BasicBlock*>& getStoreBBs() {
        return StoreBBs;
    }

private:
    SmallVector<LoadInst*, 8> Loads;

    SetVector<BasicBlock*> StoreBBs;

    void visitMemIntrinsic(MemIntrinsic& I) {
        StoreBBs.insert(I.getParent());
    }

    void visitIntrinsicInst(IntrinsicInst& II) {
        auto IID = II.getIntrinsicID();
        if (IID == Intrinsic::lifetime_start || IID == Intrinsic::lifetime_end) {
            return;
        }

        if (!II.onlyReadsMemory()) {
            StoreBBs.insert(II.getParent());
        }
    }

    void visitStoreInst(StoreInst &SI) {
        StoreBBs.insert(SI.getParent());
    }

    void visitLoadInst(LoadInst &LI) {
        if (LI.isUnordered() && IsOffsetKnown) {
            Loads.push_back(&LI);
        }
    }
};

bool PromoteConstantStructs::runOnFunction(Function &F) {
    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    MD = &getAnalysis<MemoryDependenceWrapperPass>().getMemDep();
    LPI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    bool Changed = false;
    BasicBlock &EntryBB = F.getEntryBlock();
    for (BasicBlock::iterator I = EntryBB.begin(), E = std::prev(EntryBB.end());
        I != E; ++I) {
        if (AllocaInst *AI = dyn_cast<AllocaInst>(I))
            Changed |= processAlloca(*AI);
    }

    return Changed;
}

bool PromoteConstantStructs::processAlloca(AllocaInst &AI) {
    // we do not process single or array allocas
    if (!AI.getAllocatedType()->isStructTy())
        return false;

    AllocaChecker AC(AI.getModule()->getDataLayout());
    AllocaChecker::PtrInfo PtrI = AC.visitPtr(AI);
    if (PtrI.isEscaped() || PtrI.isAborted())
        return false;

    // if we don't have any stores, nothing to do
    if (AC.getStoreBBs().empty())
        return false;

    bool Changed = false;
    bool LocalChanged = true;
    while (LocalChanged) {
        LocalChanged = false;

        auto LII = AC.getPotentialLoads().begin();
        while (LII != AC.getPotentialLoads().end()) {
            if (processLoad(*LII, AC.getStoreBBs())) {
                LII = AC.getPotentialLoads().erase(LII);
                LocalChanged = true;
            } else {
                ++LII;
            }
        }
        Changed |= LocalChanged;
    }

    return Changed;
}

bool PromoteConstantStructs::processLoad(LoadInst *LI, SetVector<BasicBlock*>& StoreBBs) {
    unsigned limit = InstructionsLimit;
    StoreInst* SI = nullptr;

    auto ML = MemoryLocation::get(LI);
    for (auto StBB : StoreBBs) {
        SmallVector<BasicBlock*, 32> Worklist;
        Worklist.push_back(StBB);

        if (!isPotentiallyReachableFromMany(Worklist, LI->getParent(), nullptr, DT, LPI))
            continue;

        Instruction* InstPt = StBB->getTerminator();
        if (StBB == LI->getParent())
            InstPt = LI;

        MemDepResult Dep = MD->getPointerDependencyFrom(ML, true,
            InstPt->getIterator(), StBB, LI, &limit);

        if (Dep.isDef()) {
            // skip if more than one def
            if (SI)
                return false;

            // we search only for stores
            SI = dyn_cast<StoreInst>(Dep.getInst());
            if (!SI)
                return false;

            if (!DT->dominates(SI, LI))
                return false;
        } else if (!Dep.isNonLocal()) {
            return false;
        }
        // else no memdep in this BB, can move on
    }

    if (!SI)
        return false;

    // we search only for constants being stored
    Constant *SC = dyn_cast<Constant>(SI->getValueOperand());
    if (!SC)
        return false;

    // no type casts
    if (SC->getType() != LI->getType())
        return false;

    LI->replaceAllUsesWith(SC);
    LI->eraseFromParent();

    return true;
}
