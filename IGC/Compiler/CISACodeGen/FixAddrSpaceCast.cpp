/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "addrspacecast-fixer"
#include "Compiler/CISACodeGen/FixAddrSpaceCast.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {
    class AddrSpaceCastFixing : public FunctionPass {
        const unsigned GAS = ADDRESS_SPACE_GENERIC;
        const unsigned PrivateAS = ADDRESS_SPACE_PRIVATE;

    public:
        static char ID;

        AddrSpaceCastFixing() : FunctionPass(ID) {
            initializeAddrSpaceCastFixingPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
        }

    private:
        bool fixOnBasicBlock(BasicBlock*) const;

        bool fixCase1(Instruction* I, BasicBlock::iterator& BI) const;
        bool fixCase2(Instruction* I, BasicBlock::iterator& BI) const;
    };
} // End anonymous namespace

FunctionPass* IGC::createFixAddrSpaceCastPass() {
    return new AddrSpaceCastFixing();
}

char AddrSpaceCastFixing::ID = 0;

#define PASS_FLAG     "igc-addrspacecast-fix"
#define PASS_DESC     "Fix invalid addrspacecast-relevant patterns"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(AddrSpaceCastFixing, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_END(AddrSpaceCastFixing, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

bool AddrSpaceCastFixing::runOnFunction(Function& F) {
    bool Changed = false;
    for (auto& BB : F)
        Changed |= fixOnBasicBlock(&BB);

    return Changed;
}

bool AddrSpaceCastFixing::fixOnBasicBlock(BasicBlock* BB) const {
    bool Changed = false;

    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; /* EMPTY */) {
        Instruction* I = &(*BI++);
        if (fixCase1(I, BI)) {
            Changed = true;
            continue;
        }
        if (fixCase2(I, BI)) {
            Changed = true;
            continue;
        }
    }

    return Changed;
}

/// fixCase1 - Convert pair of `ptrtoint`/`inttoptr` through `i64` back to
/// `addrspacecast`.
bool AddrSpaceCastFixing::fixCase1(Instruction* I, BasicBlock::iterator& BI) const {
    // Find the eligible pair of `ptrtoint`/`inttoptr` and convert them back
    // to `addrspacecast`.
    IntToPtrInst* I2P = dyn_cast<IntToPtrInst>(I);
    if (!I2P || !I2P->getType()->isPointerTy())
        return false;
    PointerType* DstPtrTy = cast<PointerType>(I2P->getType());
    PtrToIntInst* P2I = dyn_cast<PtrToIntInst>(I2P->getOperand(0));
    if (!P2I)
        return false;
    if (!P2I->hasOneUse() || !P2I->getType()->isIntegerTy(64))
        return false;

    Value* V = P2I->getOperand(0);
    PointerType* SrcPtrTy = cast<PointerType>(V->getType());

    // Skip generating `bitcast` if their element types are different. Leave that
    // canonicalization in GAS resolver.

    // Create `addrspacecast` if necessary.
    if (SrcPtrTy->getAddressSpace() != DstPtrTy->getAddressSpace())
        V = CastInst::Create(Instruction::AddrSpaceCast, V, DstPtrTy,
            I->getName() + ".fix1.addrspacecast", I);
    else
        V = CastInst::Create(Instruction::BitCast, V, DstPtrTy,
            I->getName() + ".fix1.bitcast", I);

    // Remove the short sequence of `ptrtoint` followed by `inttoptr`.
    if (Instruction* VInst = dyn_cast<Instruction>(V))
    {
        VInst->setDebugLoc(I2P->getDebugLoc());
    }
    I2P->replaceAllUsesWith(V);
    I2P->eraseFromParent();
    P2I->eraseFromParent();

    if (isa<Instruction>(V))
        BI = BasicBlock::iterator(cast<Instruction>(V));

    return true;
}

/// fixCase2 - Convert the following sequence into a valid one.
///
/// (addrspacecast-gas
///   (select %cond (addrspacecast-private %ptr1)
///                 (addrspacecast-private %ptr2)))
///
/// into
///
/// (select %cond (addrspacecast-gas %ptr1)
///               (addrspacecast-gas %ptr2))
///
bool AddrSpaceCastFixing::fixCase2(Instruction* I, BasicBlock::iterator& BI) const {
    AddrSpaceCastInst* CI = dyn_cast<AddrSpaceCastInst>(I);
    // Skip if it's not an addrspacecast to GAS
    if (!CI || cast<PointerType>(CI->getType())->getAddressSpace() != GAS)
        return false;

    // Skip if it's not a select of private pointers.
    SelectInst* SI = dyn_cast<SelectInst>(CI->getOperand(0));
    if (!SI || cast<PointerType>(SI->getType())->getAddressSpace() != PrivateAS)
        return false;

    PointerType* DstPtrTy = cast<PointerType>(CI->getType());

    // Skip if T/F values don't agree on address space.
    Value* TVal = SI->getTrueValue();
    if (TVal->hasOneUse() && isa<AddrSpaceCastInst>(TVal))
        TVal->mutateType(DstPtrTy);
    else
        TVal = CastInst::Create(Instruction::AddrSpaceCast, TVal, DstPtrTy,
            TVal->getName() + ".fix2.addrspacecast", SI);

    Value* FVal = SI->getFalseValue();
    if (FVal->hasOneUse() && isa<AddrSpaceCastInst>(FVal))
        FVal->mutateType(DstPtrTy);
    else
        FVal = CastInst::Create(Instruction::AddrSpaceCast, FVal, DstPtrTy,
            FVal->getName() + ".fix2.addrspacecast", SI);

    SI->setOperand(1, TVal);
    SI->setOperand(2, FVal);
    SI->mutateType(DstPtrTy);

    CI->replaceAllUsesWith(SI);
    CI->eraseFromParent();

    return true;
}
