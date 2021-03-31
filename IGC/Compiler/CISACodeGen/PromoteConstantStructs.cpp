/*========================== begin_copyright_notice ============================

Copyright (c) 2014-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/PromoteConstantStructs.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/PtrUseVisitor.h"
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
    // -- All stores to the structure must be in the same BB
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

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.addRequired<MemoryDependenceWrapperPass>();
            AU.setPreservesCFG();
            AU.addPreserved<DominatorTreeWrapperPass>();
        }

        bool processAlloca(AllocaInst &AI);

        bool processLoad(LoadInst *LI, BasicBlock *StoreBB);

    };

    char PromoteConstantStructs::ID = 0;

} // End anonymous namespace

llvm::FunctionPass* createPromoteConstantStructsPass() {
    return new PromoteConstantStructs();
}

#define PASS_FLAG     "igc-promoteconstantstructs"
#define PASS_DESC     "Promote constant structs"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PromoteConstantStructs, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
INITIALIZE_PASS_DEPENDENCY(MemoryDependenceWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(PromoteConstantStructs, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

// This class visits all alloca uses to check that
// -- it does not escape
// -- all stores are in the same BB
// and collect all loads with constant offsets from alloca

class AllocaChecker : public PtrUseVisitor<AllocaChecker> {
    friend class PtrUseVisitor<AllocaChecker>;
    friend class InstVisitor<AllocaChecker>;

public:
    AllocaChecker(const DataLayout &DL)
        : PtrUseVisitor<AllocaChecker>(DL), StoreBB(nullptr) {}

    SmallVector<LoadInst*, 8>& getPotentialLoads() {
        return Loads;
    }

    BasicBlock *getStoreBB() {
        return StoreBB;
    }

private:
    SmallVector<LoadInst*, 8> Loads;

    BasicBlock *StoreBB;

    void visitStoreInst(StoreInst &SI) {
        if (!StoreBB) {
            StoreBB = SI.getParent();
        }
        else if (SI.getParent() != StoreBB) {
            PI.setAborted(&SI);
        }
    }

    void visitLoadInst(LoadInst &LI) {
        if (LI.isUnordered() && IsOffsetKnown) {
            Loads.push_back(&LI);
        }
    }
};

bool PromoteConstantStructs::runOnFunction(Function &F)
{
    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    MD = &getAnalysis<MemoryDependenceWrapperPass>().getMemDep();

    bool Changed = false;
    BasicBlock &EntryBB = F.getEntryBlock();
    for (BasicBlock::iterator I = EntryBB.begin(), E = std::prev(EntryBB.end());
        I != E; ++I) {
        if (AllocaInst *AI = dyn_cast<AllocaInst>(I))
            Changed |= processAlloca(*AI);
    }

    return Changed;
}

bool PromoteConstantStructs::processAlloca(AllocaInst &AI)
{
    // we do not process single or array allocas
    if (!AI.getAllocatedType()->isStructTy())
        return false;

    AllocaChecker AC(AI.getModule()->getDataLayout());
    AllocaChecker::PtrInfo PtrI = AC.visitPtr(AI);
    if (PtrI.isEscaped() || PtrI.isAborted())
        return false;

    BasicBlock *StoreBB = AC.getStoreBB();
    // if we don't have any stores, nothing to do
    if (!StoreBB)
        return false;

    bool Changed = false;
    for (auto LI : AC.getPotentialLoads())
        Changed |= processLoad(LI, StoreBB);

    return Changed;
}

bool PromoteConstantStructs::processLoad(LoadInst *LI, BasicBlock *StoreBB)
{
    if (!DT->dominates(StoreBB->getFirstNonPHI(), LI))
        return false;

    Instruction *InstPt = StoreBB->getTerminator();
    if (StoreBB == LI->getParent()) InstPt = LI;

    unsigned limit = InstructionsLimit;

    MemDepResult Dep = MD->getPointerDependencyFrom(MemoryLocation::get(LI), true,
        InstPt->getIterator(), StoreBB, LI, &limit);

    // we care only about must aliases, no clobber dependencies
    if (!Dep.isDef())
        return false;

    // we search only for stores
    StoreInst *SI = dyn_cast<StoreInst>(Dep.getInst());
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
