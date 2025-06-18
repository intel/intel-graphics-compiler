/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SplitStructurePhisPass.hpp"

using namespace llvm;
using namespace IGC;

char SplitStructurePhisPass::ID = 0;

#define PASS_FLAG "split-structure-phis"
#define PASS_DESCRIPTION "Split structure phis pass."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define POISON_SIZE_T 999


// The SplitStructurePhisPass is a function pass designed to optimize the handling of PHI nodes that operate on structures containing multiple fields,
// such as vectors and scalars. This pass splits the PHI nodes into separate PHI nodes for each individual field in the structure in case one of
// the incoming values is a zeroinitializer.
// This helps the emitter avoid generating intermediate mov instructions to initialize the structure with zero values.

IGC_INITIALIZE_PASS_BEGIN(SplitStructurePhisPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(SplitStructurePhisPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

SplitStructurePhisPass::SplitStructurePhisPass() : FunctionPass(ID) {
    initializeSplitStructurePhisPassPass(*PassRegistry::getPassRegistry());
}

bool SplitStructurePhisPass::runOnFunction(Function &F) {
    if (skipFunction(F))
        return false;

    for (auto &BB : F) {
        // Iterate over all instructions in the basic block.
        for (auto &I : BB) {
            auto *Phi = dyn_cast<PHINode>(&I);
            IncomingValuesMap InsertValues;
            ExtractValueMap ExtractValues;

            // Skip non-phi instructions.
            if (!Phi)
                continue;

            // Currently, we only support PHI nodes with two incoming values.
            if (Phi->getNumIncomingValues() != 2)
                continue;

            // Skip phi node instruction if its structure type doesn't have vector types.
            if (!isStructOfVectorsType(Phi->getType()))
                continue;

            // Get indices of the incoming values.
            IndicesTuple Indices = getIndices(Phi);

            // Skip if phi node doesn't have zero incoming value.
            if (std::get<Zero>(Indices) == POISON_SIZE_T || std::get<NonZero>(Indices) == POISON_SIZE_T)
                continue;

            // Skip phi nodes that are used by other instructions, other than extractvalue.
            if (!isPhiNodeParsedByExtrVal(Phi, ExtractValues))
                continue;

            Value *NonZeroIncVal = Phi->getIncomingValue(std::get<NonZero>(Indices));

            // Check that the non-zero incoming value was created by insertvalue instructions.
            if (!checkNonZeroIncValue(NonZeroIncVal, InsertValues))
                continue;

            PhiNodes[Phi] = std::make_tuple(Indices, ExtractValues, InsertValues);
        }
    }

    bool Changed = PhiNodes.size() > 0 ? true : false;

    // Iterate over the collected PHI nodes and
    // 1. create new phis for each vector type
    // 2. create new phis for each scalar types
    // 3. save dead instructions for removal
    // 4. update incoming phis incoming values
    for (auto& PhiPair : PhiNodes) {
        PHINode *OldPhi = PhiPair.first;
        auto Indices = std::get<0>(PhiPair.second);
        ExtractValueMap ExtractValues = std::get<1>(PhiPair.second);
        IncomingValuesMap InsertValues = std::get<2>(PhiPair.second);

        StructType *StTy = cast<StructType>(OldPhi->getType());
        for (unsigned i = 0; i < StTy->getNumElements(); ++i) {
            auto *VecTy = dyn_cast<VectorType>(StTy->getElementType(i));

            if (VecTy) {
                createVectorPhi(OldPhi, Indices, ExtractValues[i], InsertValues[i]);
            } else {
                createScalarPhi(OldPhi, StTy->getElementType(i), Indices, ExtractValues[i], InsertValues[i]);
            }
        }

        // Save old phi to remove it later.
        PhiNodeInstsToRemove.insert(OldPhi);
    }

    // Clean up the dead instructions.
    cleanUp();

    return Changed;
}

void SplitStructurePhisPass::cleanUp() {
    for (auto *ExtrValInst : ExtractValueInstsToRemove)
        ExtrValInst->eraseFromParent();

    for (auto *Phi : PhiNodeInstsToRemove)
        Phi->eraseFromParent();

    for (auto *InsValInst : InsertValueInstsToRemove) {
        while (InsValInst) {
            InsertValueInst *InstToRemov = InsValInst;
            InsValInst = dyn_cast<InsertValueInst>(InsValInst->getAggregateOperand());
            InstToRemov->eraseFromParent();
        }
    }

    // Clear the maps and sets after work on function.
    PhiNodes.clear();
    InsertValueInstsToRemove.clear();
    ExtractValueInstsToRemove.clear();
    PhiNodeInstsToRemove.clear();
}

IndicesTuple SplitStructurePhisPass::getIndices(PHINode *Phi) {
    size_t ZeroIncValIndex = POISON_SIZE_T;
    size_t OtherIncValIndex = POISON_SIZE_T;

    if (isa<ConstantAggregateZero>(Phi->getIncomingValue(0))) {
        ZeroIncValIndex = 0;
        OtherIncValIndex = 1;
    } else if (isa<ConstantAggregateZero>(Phi->getIncomingValue(1))) {
        ZeroIncValIndex = 1;
        OtherIncValIndex = 0;
    } else {
        return std::make_tuple(POISON_SIZE_T, POISON_SIZE_T);
    }

    return std::make_tuple(ZeroIncValIndex, OtherIncValIndex);
}

void SplitStructurePhisPass::createScalarPhi(PHINode *OldPhi, Type *NewScalarType, const IndicesTuple &Indices, ExtractValueInst *OldExtractInst, InsertValueInst *OldInsertValInst) {
    IRBuilder<> Builder(OldPhi);
    auto *NewPhi = cast<PHINode>(Builder.CreatePHI(NewScalarType, 2, "splitted_phi"));

    size_t ZeroIncomingIndex = std::get<Zero>(Indices);
    size_t NonZeroIncomingIndex = std::get<NonZero>(Indices);

    NewPhi->addIncoming(Constant::getNullValue(NewScalarType), OldPhi->getIncomingBlock(ZeroIncomingIndex));
    NewPhi->addIncoming(OldInsertValInst->getInsertedValueOperand(), OldPhi->getIncomingBlock(NonZeroIncomingIndex));

    OldExtractInst->replaceAllUsesWith(NewPhi);
    ExtractValueInstsToRemove.insert(OldExtractInst);
    if (isLastInsertValueInst(OldInsertValInst, OldPhi))
        InsertValueInstsToRemove.insert(OldInsertValInst);
}

void SplitStructurePhisPass::createVectorPhi(PHINode *OldPhi, const IndicesTuple &Indices, ExtractValueInst *ExtractInst, InsertValueInst *InsertValInst) {
    Value *NewIncomingNonZeroVal = InsertValInst->getInsertedValueOperand();
    Type *NewIncomingTy = NewIncomingNonZeroVal->getType();

    IRBuilder<> Builder(OldPhi);
    PHINode *NewPhi = cast<PHINode>(Builder.CreatePHI(NewIncomingTy, 2, "splitted_phi"));

    size_t ZeroIncomingIndex = std::get<Zero>(Indices);
    size_t NonZeroIncomingIndex = std::get<NonZero>(Indices);

    NewPhi->addIncoming(ConstantAggregateZero::get(NewIncomingTy), OldPhi->getIncomingBlock(ZeroIncomingIndex));
    NewPhi->addIncoming(NewIncomingNonZeroVal, OldPhi->getIncomingBlock(NonZeroIncomingIndex));
    ExtractInst->replaceAllUsesWith(NewPhi);

    ExtractValueInstsToRemove.insert(ExtractInst);

    // Save only the last insert value instruction for safe removal.
    if (isLastInsertValueInst(InsertValInst, OldPhi))
        InsertValueInstsToRemove.insert(InsertValInst);
}

bool SplitStructurePhisPass::isLastInsertValueInst(InsertValueInst *InsertValInst, PHINode *OldPhi) {
    auto U = *InsertValInst->user_begin();
    if (U != OldPhi)
        return false;

    return true;
}

// Check if non-zero increment value was created by insertvalue instructions.
bool SplitStructurePhisPass::checkNonZeroIncValue(Value *IncVal, IncomingValuesMap &InsertValues) {
    StructType *StTy = cast<StructType>(IncVal->getType());

    Value *InsertVal = IncVal;
    for (unsigned i = 0; i < StTy->getNumElements(); ++i) {
        InsertValueInst *InsertInst = dyn_cast<InsertValueInst>(InsertVal);

        if (!InsertInst)
            return false;

        if (!InsertInst->hasOneUse())
            return false;

        if (InsertInst->getNumIndices() != 1)
            return false;

        size_t ValueIndexInStruct = InsertInst->getIndices()[0];
        if (InsertValues.find(ValueIndexInStruct) != InsertValues.end())
            return false;

        InsertValues[ValueIndexInStruct] = InsertInst;
        InsertVal = InsertInst->getAggregateOperand();
    }

    if (!isa<PoisonValue>(InsertVal) && !isa<UndefValue>(InsertVal))
        return false;

    return true;
}

bool SplitStructurePhisPass::isPhiNodeParsedByExtrVal(PHINode *Phi, ExtractValueMap &ExtractValues) {
    for (auto *User : Phi->users()) {
        ExtractValueInst *ExtractInst = dyn_cast<ExtractValueInst>(User);
        if (!ExtractInst)
            return false;

        if (ExtractInst->getNumIndices() != 1)
            return false;

        size_t ValueIndexInStruct = ExtractInst->getIndices()[0];
        if (ExtractValues.find(ValueIndexInStruct) != ExtractValues.end())
            return false;

        ExtractValues[ValueIndexInStruct] = ExtractInst;
    }

    return true;
}

bool SplitStructurePhisPass::isStructOfVectorsType(Type *Ty) {
    bool HasVector = false;
    // Check if the type is a struct
    auto *STy = dyn_cast<StructType>(Ty);

    if (!STy)
        return false;

    // Check if the struct type is an array of structs
    for (unsigned i = 0; i < STy->getNumElements(); ++i) {
        Type *ElemTy = STy->getElementType(i);

        if (ElemTy->isIntegerTy() || ElemTy->isFloatingPointTy() || ElemTy->isPointerTy())
            continue;

        auto *VecTy = dyn_cast<VectorType>(ElemTy);
        if (!VecTy)
            return false;

        Type *VecElTy = VecTy->getElementType();
        if (!VecElTy->isIntegerTy() && !VecElTy->isFloatingPointTy() && !VecElTy->isPointerTy())
            return false;

        HasVector = true;
    }

    return HasVector;
}