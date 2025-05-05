/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "gen-simplification"
#include "Compiler/CISACodeGen/GenSimplification.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/InstVisitor.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

    class GenSimplification : public FunctionPass,
        public InstVisitor<GenSimplification> {
        bool Changed;

    public:
        static char ID;

        GenSimplification() : FunctionPass(ID), Changed(false) {
            initializeGenSimplificationPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }
        void visitPHINode(PHINode&);
        void visitExtractElement(ExtractElementInst&);

    private:
        bool simplifyVectorPHINodeCase1(PHINode&) const;
        bool simplifyVectorPHINodeCase2(PHINode&) const;
    };

} // End anonymous namespace

char GenSimplification::ID = 0;

#define PASS_FLAG     "igc-shuffle-simplification"
#define PASS_DESC     "Gen Simplification"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(GenSimplification, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_END(GenSimplification, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

FunctionPass* IGC::createGenSimplificationPass() {
    return new GenSimplification();
}

bool GenSimplification::runOnFunction(Function& F) {
    // Skip non-kernel function.
    MetaDataUtils* MDU = nullptr;
    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    Changed = false;
    visit(F);

    return Changed;
}

bool GenSimplification::simplifyVectorPHINodeCase1(PHINode& PN) const {
    IGC_ASSERT(isa<VectorType>(PN.getType()));

    // Check all users are 'bitcast'.
    Type* Ty = nullptr;
    for (auto* U : PN.users()) {
        auto BC = dyn_cast<BitCastInst>(U);
        if (!BC)
            return false;
        if (Ty && Ty != BC->getType())
            return false;
        Ty = BC->getType();
    }
    // Check all operands are constant or 'bitcast' from the same type.
    for (unsigned i = 0, e = PN.getNumIncomingValues(); i != e; ++i) {
        auto V = PN.getIncomingValue(i);
        if (isa<Constant>(V))
            continue;
        auto BC = dyn_cast<BitCastInst>(V);
        if (!BC)
            return false;
        if (BC->getSrcTy() != Ty)
            return false;
    }

    IRBuilder<> IRB(&PN);
    // Form a phi-node of type 'Ty'.
    PHINode* NewPN = IRB.CreatePHI(Ty, PN.getNumIncomingValues());
    for (unsigned i = 0, e = PN.getNumIncomingValues(); i != e; ++i) {
        auto V = PN.getIncomingValue(i);
        if (auto BC = dyn_cast<BitCastInst>(V)) {
            V = BC->getOperand(0);
        }
        else {
            IGC_ASSERT(isa<Constant>(V));
            V = IRB.CreateBitCast(V, Ty);
        }
        NewPN->addIncoming(V, PN.getIncomingBlock(i));
    }
    for (auto* U : PN.users()) {
        auto BC = cast<BitCastInst>(U);
        BC->replaceAllUsesWith(NewPN);
    }

    return true;
}

bool GenSimplification::simplifyVectorPHINodeCase2(PHINode& PN) const {
    IGC_ASSERT(isa<VectorType>(PN.getType()));

    // Check all users are 'extractelement' with constant indices.
    for (auto* U : PN.users()) {
        auto EEI = dyn_cast<ExtractElementInst>(U);
        if (!EEI)
            return false;
        ConstantInt* Idx = dyn_cast<ConstantInt>(EEI->getIndexOperand());
        if (!Idx)
            return false;
    }

    Type* Ty = PN.getType();
    Type* EltTy = Ty->getScalarType();
    unsigned NumElts = (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements();

    SmallVector<Value*, 8> Lanes;
    SmallVector<SmallVector<Value*, 8>, 4> Values;

    // Check all operands are constants or chains of 'insertelement'.
    for (unsigned i = 0, e = PN.getNumIncomingValues(); i != e; ++i) {
        auto V = PN.getIncomingValue(i);
        // Refill lanes.
        Lanes.clear();
        Lanes.resize(NumElts);
        std::fill(Lanes.begin(), Lanes.end(), UndefValue::get(EltTy));
        // Constant
        if (auto C = dyn_cast<Constant>(V)) {
            for (unsigned j = 0; j != NumElts; ++j) {
                auto Elt = C->getAggregateElement(j);
                if (!Elt)
                    return false;
                Lanes[j] = Elt;
            }
            Values.push_back(Lanes);
            continue;
        }
        // Or chain of 'insertelement'.
        APInt IdxMask(NumElts, 0); // Variable length bit mask.
        while (auto IEI = dyn_cast<InsertElementInst>(V)) {
            ConstantInt* Idx = dyn_cast<ConstantInt>(IEI->getOperand(2));
            if (!Idx)
                return false;
            unsigned j = unsigned(Idx->getZExtValue());
            // Skip if 'j' is already populated.
            if (!IdxMask[j]) {
                Lanes[j] = IEI->getOperand(1);
                IdxMask.setBit(j);
            }
            // Move next.
            V = IEI->getOperand(0);
        }
        if (!isa<UndefValue>(V))
            return false;
        Values.push_back(Lanes);
    }

    IRBuilder<> IRB(&PN);
    // Form scalar phi-nodes of type 'EltTy'.
    for (unsigned j = 0; j != NumElts; ++j) {
        PHINode* NewPN = IRB.CreatePHI(EltTy, PN.getNumIncomingValues());
        for (unsigned i = 0, e = PN.getNumIncomingValues(); i != e; ++i) {
            auto V = Values[i][j];
            NewPN->addIncoming(V, PN.getIncomingBlock(i));
        }
        for (auto* U : PN.users()) {
            auto EEI = cast<ExtractElementInst>(U);
            auto Idx = cast<ConstantInt>(EEI->getIndexOperand());
            if (Idx->getZExtValue() != j)
                continue;
            EEI->replaceAllUsesWith(NewPN);
        }
    }

    return true;
}

void GenSimplification::visitPHINode(PHINode& PN) {
    VectorType* VTy = dyn_cast<VectorType>(PN.getType());
    if (!VTy)
        return;

    Changed |= simplifyVectorPHINodeCase1(PN) || simplifyVectorPHINodeCase2(PN);
}

void GenSimplification::visitExtractElement(ExtractElementInst& EEI) {
    // Skip non-2-element vector.
    Value* Vec = EEI.getVectorOperand();
    IGCLLVM::FixedVectorType* VTy = cast<IGCLLVM::FixedVectorType>(Vec->getType());
    if (VTy->getNumElements() != 2)
        return;

    // Skip if the index is not zero-extended from a boolean value.
    ZExtInst* ZEI = dyn_cast<ZExtInst>(EEI.getIndexOperand());
    if (!ZEI || !ZEI->getOperand(0)->getType()->isIntegerTy(1))
        return;
    Value* Cond = ZEI->getOperand(0);

    // Collect all scalar values forming this 2-element vector.
    SmallVector<Value*, 2> Values(2);
    unsigned Mask = 0;
    InsertElementInst* IEI = nullptr;
    while ((IEI = dyn_cast<InsertElementInst>(Vec))) {
        ConstantInt* Idx = dyn_cast<ConstantInt>(IEI->getOperand(2));
        if (!Idx)
            return;
        unsigned i = unsigned(Idx->getZExtValue());
        if (i != 0 && i != 1)
            return;
        Values[i] = IEI->getOperand(1);
        Mask |= (1U << i);
        // Early quit if all elements are collected.
        if (Mask == 3)
            break;
        Vec = IEI->getOperand(0);
    }
    // Skip if one of them could not be collected.
    if (Values[0] == nullptr || Values[1] == nullptr)
        return;
    // Convert such an extractelement into select.
    IRBuilder<> IRB(&EEI);
    Value* NewVal = IRB.CreateSelect(Cond, Values[1], Values[0]);
    EEI.replaceAllUsesWith(NewVal);
    Changed = true;
}
