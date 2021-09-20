/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*
FIXME? ::
1. We do not consider illegal vector lenghts of legal ints
2. we are not legalizing all the ALU instructions
3. !! When legalizing, there need to be a check on generated vector length. eg: non-power of 2 lengths not allowed except 3. what is max allowed?
4. StoreInst : Need to consider illegal type being stored directly without being cast back to a legal.
*/

#define DEBUG_TYPE "type-legalizer"
#include "PeepholeTypeLegalizer.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"
#include "GenISAIntrinsics/GenIntrinsics.h"

using namespace llvm;
using namespace IGC::Legalizer;

char PeepholeTypeLegalizer::ID = 0;
#define PASS_FLAG     "igc-int-type-legalizer"
#define PASS_DESC     "IGC Int Type Legalizer"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PeepholeTypeLegalizer, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
//IGC_INITIALIZE_PASS_DEPENDENCY(DataLayout);
IGC_INITIALIZE_PASS_END(PeepholeTypeLegalizer, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)


PeepholeTypeLegalizer::PeepholeTypeLegalizer() : FunctionPass(ID),
TheModule(nullptr), TheFunction(nullptr), NonBitcastInstructionsLegalized(false), CastInst_ZExtWithIntermediateIllegalsEliminated(false),
CastInst_TruncWithIntermediateIllegalsEliminated(false),
Bitcast_BitcastWithIntermediateIllegalsEliminated(false), Changed(false), DL(nullptr) {
    initializePeepholeTypeLegalizerPass(*PassRegistry::getPassRegistry());
}


void PeepholeTypeLegalizer::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.setPreservesCFG();
}

FunctionPass* createPeepholeTypeLegalizerPass() { return new PeepholeTypeLegalizer(); }


bool PeepholeTypeLegalizer::runOnFunction(Function& F) {
    DL = &F.getParent()->getDataLayout();
    IGC_ASSERT_MESSAGE(DL->isLittleEndian(), "ONLY SUPPORT LITTLE ENDIANNESS!");

    IGCLLVM::IRBuilder<> builder(F.getContext());
    m_builder = &builder;

    Changed = false;
    visit(F);
    if (Changed) {
        NonBitcastInstructionsLegalized = true;
        visit(F);
        CastInst_ZExtWithIntermediateIllegalsEliminated = true;
        visit(F);
        CastInst_TruncWithIntermediateIllegalsEliminated = true;
        visit(F);
        Bitcast_BitcastWithIntermediateIllegalsEliminated = true;
    }
    return Changed;
}

void promoteInt(unsigned srcWidth, unsigned& quotient, unsigned& promoteToInt, unsigned MAX_LEGAL_INT)
{
    for (unsigned i = 8; i <= MAX_LEGAL_INT; i *= 2) {
        quotient = i / srcWidth;
        if (quotient) {
            quotient = 1;
            promoteToInt = i;
            break;
        }
    }
    if (!quotient) {
        quotient = srcWidth / MAX_LEGAL_INT;
        if (srcWidth % MAX_LEGAL_INT != 0)
            quotient++;
        promoteToInt = MAX_LEGAL_INT; // FIXME? : Are all vector lengths legal?
    }
}


void PeepholeTypeLegalizer::visitInstruction(Instruction& I) {

    if (I.getNumOperands() == 0)
        return;

    if (!I.getOperand(0)->getType()->isIntOrIntVectorTy() &&
        !dyn_cast<ExtractElementInst>(&I))
        return; // Legalization for int types only or for extractelements

    m_builder->SetInsertPoint(&I);

    //Depending on the phase of legalization pass, call appropriate function
    if (!NonBitcastInstructionsLegalized) { // LEGALIZE ALUs first
        if (dyn_cast<PHINode>(&I)) {
            legalizePhiInstruction(I);  // phi nodes and all incoming values
        }
        else if (dyn_cast<UnaryInstruction>(&I)) {
            legalizeUnaryInstruction(I); // pointercast &/or load
        }
        else if (dyn_cast<ICmpInst>(&I) || dyn_cast<BinaryOperator>(&I) || dyn_cast<SelectInst>(&I)) {
            legalizeBinaryOperator(I);    // Bitwise and Arithmetic Operations
        }
        else if (dyn_cast<ExtractElementInst>(&I)) {
            legalizeExtractElement(I);
        }
    }
    else if (!CastInst_ZExtWithIntermediateIllegalsEliminated) { // Eliminate intermediate ILLEGAL operands in bitcast-zext or trunc-zext pairs
        if (dyn_cast<ZExtInst>(&I))
            cleanupZExtInst(I);
    }
    else if (!CastInst_TruncWithIntermediateIllegalsEliminated) { // Eliminate intermediate ILLEGAL operands in bitcast-zext or trunc-zext pairs
        if (dyn_cast<TruncInst>(&I))
            cleanupTruncInst(I);
    }
    else if (!Bitcast_BitcastWithIntermediateIllegalsEliminated) { // Eliminate redundant bitcast-bitcast pairs and eliminate intermediate ILLEGAL operands in bitcast-bitcast pairs with src == dest OR src != dest
        if (dyn_cast<BitCastInst>(&I))
            cleanupBitCastInst(I);
    }
}

/*
  %24 = bitcast <3 x i16> %23 to i48
  br label %BB1
BB2:
  %val48 = phi i48 [ %24, %BB1], [ 0, %BB2]
  %val64 = zext i48 %val to i64
---->
  %24 = bitcast <3 x i16> %23 to i48
  %25 = zext i48 %24 to i64
  %26 = bitcast i64 %25 to <2 x i32>
  br label %BB1
BB2:
  %27 = phi <2 x i32> [ %26, %BB1 ], [ zeroinitializer, %BB2]
  %28 = bitcast <2 x i32> %27 to i64
  %29 = trunc i64 %28 to i48
  %val64 = zext i48 %29 to i64
*/
void PeepholeTypeLegalizer::legalizePhiInstruction(Instruction& I)
{
    IGC_ASSERT(isa<PHINode>(&I));

    unsigned srcWidth = I.getType()->getScalarSizeInBits();
    if (!I.getType()->isIntOrIntVectorTy() || isLegalInteger(srcWidth) || srcWidth == 1) // nothing to legalize
        return;

    unsigned quotient, promoteToInt;
    promoteInt(srcWidth, quotient, promoteToInt, DL->getLargestLegalIntTypeSizeInBits());

    PHINode* oldPhi = dyn_cast<PHINode>(&I);
    Value* result;

    if (quotient > 1)
    {
        unsigned numElements = I.getType()->isVectorTy() ? (unsigned)cast<IGCLLVM::FixedVectorType>(I.getType())->getNumElements() : 1;
        Type* newVecType = IGCLLVM::FixedVectorType::get(Type::getIntNTy(I.getContext(), promoteToInt), quotient * numElements);
        Type* newLargeIntType = Type::getIntNTy(I.getContext(), promoteToInt * quotient);

        PHINode* newPhi = m_builder->CreatePHI(newVecType, oldPhi->getNumIncomingValues());
        for (unsigned i = 0; i < oldPhi->getNumIncomingValues(); i++)
        {
            Value* incomingValue = oldPhi->getIncomingValue(i);

            m_builder->SetInsertPoint(oldPhi->getIncomingBlock(i)->getTerminator());
            Value* newLargeIntValue = m_builder->CreateZExt(incomingValue, newLargeIntType);
            Value* newVecValue = m_builder->CreateBitCast(newLargeIntValue, newVecType);
            newPhi->addIncoming(newVecValue, oldPhi->getIncomingBlock(i));
        }
        // Cast back to original type
        m_builder->SetInsertPoint(newPhi->getParent()->getFirstNonPHI());
        Value* NewLargeIntPhi = m_builder->CreateBitCast(newPhi, newLargeIntType);
        result = m_builder->CreateTrunc(NewLargeIntPhi, oldPhi->getType());
    }
    else
    {
        // quotient == 1 (integer promotion)
        Type* newType = Type::getIntNTy(I.getContext(), promoteToInt);
        PHINode* newPhi = m_builder->CreatePHI(newType, oldPhi->getNumIncomingValues());
        for (unsigned i = 0; i < oldPhi->getNumIncomingValues(); i++)
        {
            Value* incomingValue = oldPhi->getIncomingValue(i);
            m_builder->SetInsertPoint(oldPhi->getIncomingBlock(i)->getTerminator());
            Value* newValue = m_builder->CreateZExt(incomingValue, newType);
            newPhi->addIncoming(newValue, oldPhi->getIncomingBlock(i));
        }
        // Cast back to original type
        m_builder->SetInsertPoint(newPhi->getParent()->getFirstNonPHI());
        result = m_builder->CreateTrunc(newPhi, oldPhi->getType());
    }
    oldPhi->replaceAllUsesWith(result);
    oldPhi->eraseFromParent();
}

void PeepholeTypeLegalizer::legalizeExtractElement(Instruction& I)
{
    IGC_ASSERT(isa<ExtractElementInst>(&I));

    // Handles ExtractElement from illegal vector types

    // sample pattern:
    //%61 = extractelement <2 x i128> %60, i<anysize> 0
    // ->
    //%157 = bitcast <2 x i128> %60 to <8 x i32>
    //%165 = extractelement <8 x i32> %157, i32 0
    //%166 = insertelement <4 x i32> undef, i32 %165, i32 0
    //%167 = extractelement <8 x i32> %157, i32 1
    //%168 = insertelement <4 x i32> %166, i32 %167, i32 1
    //%169 = extractelement <8 x i32> %157, i32 2
    //%170 = insertelement <4 x i32> %168, i32 %169, i32 2
    //%171 = extractelement <8 x i32> %157, i32 3
    //%172 = insertelement <4 x i32> %170, i32 %171, i32 3
    //%173 = bitcast <4 x i32> %172 to i128

    ExtractElementInst* extract = cast<ExtractElementInst>(&I);

    unsigned elementWidth = extract->getType()->getScalarSizeInBits();
    if (!isLegalInteger(elementWidth) && extract->getType()->isIntOrIntVectorTy())
    {
        unsigned numElements = (unsigned)cast<IGCLLVM::FixedVectorType>(extract->getOperand(0)->getType())->getNumElements();
        unsigned quotient, promoteToInt;
        promoteInt(elementWidth, quotient, promoteToInt, DL->getLargestLegalIntTypeSizeInBits());

        m_builder->SetInsertPoint(&I);

        // Bitcast the illegal vector type to legal type
        Type* newVecTy = IGCLLVM::FixedVectorType::get(Type::getIntNTy(I.getContext(), promoteToInt), quotient * numElements);
        Value* legalVector = m_builder->CreateBitCast(extract->getOperand(0), newVecTy);

        unsigned extractIndex = (unsigned)cast<ConstantInt>(extract->getOperand(1))->getZExtValue();
        Value* extractedVec = UndefValue::get(IGCLLVM::FixedVectorType::get(Type::getIntNTy(I.getContext(), promoteToInt), quotient));

        for (unsigned i = 0; i < quotient; i++)
        {
            unsigned index = extractIndex * quotient + i;
            IGC_ASSERT(index < quotient * numElements);
            Value* extractedVal = m_builder->CreateExtractElement(legalVector, m_builder->getInt32(index));
            extractedVec = m_builder->CreateInsertElement(extractedVec, extractedVal, m_builder->getInt32(i));
        }

        // Bitcast legal value back to original type. Will be removed in a later pass to cleanup bitcasts
        Value* revBitcast = m_builder->CreateBitCast(extractedVec, extract->getType());
        extract->replaceAllUsesWith(revBitcast);
        extract->eraseFromParent();

        Changed = true;
    }
    else
    {
        //We also need to check if the index of the extract element is a legal bitwidth
        auto Index = extract->getOperand(1);
        if (Index->getType()->isIntegerTy())
        {
            auto bitwidth = Index->getType()->getIntegerBitWidth();
            if (!isLegalInteger(bitwidth) || bitwidth == 64)
            {
                m_builder->SetInsertPoint(&I);

                Value* operand1 = NULL;
                if (bitwidth < 16)
                    operand1 = m_builder->CreateIntCast(Index, Type::getInt16Ty(I.getContext()), false);
                else
                    operand1 = m_builder->CreateIntCast(Index, Type::getInt32Ty(I.getContext()), false);

                Value* New_EI = m_builder->CreateExtractElement(extract->getOperand(0), operand1);
                extract->replaceAllUsesWith(New_EI);
                extract->eraseFromParent();
                Changed = true;
            }
        }
    }
}

void PeepholeTypeLegalizer::legalizeBinaryOperator(Instruction& I) {
    /*
    sample pattern:

    %1 bitcast <5 x i16>, i80
    ALU1( %1 i80)
    ALU2( ALU1 i80)
    ...
    -->
    %1 bitcast <5 x i16>, i80
    %2 zext %1 i80, i128
    %3 bitcast %2 i128, <2 x i64>
    %4 extract element %3 <2 x i64>, 0
    %5 ALU1.0 (%4 i64)
    %6 insert element %5 ALU1.0, <2 x i64>
    %7 bitcast %6 <2 x i64>, i128
    %8 trunc %7 i128, i80
    ALU2 (%8 i80)
    ...
    */
    Value* Src1 = nullptr;
    Value* Src2 = nullptr;

    //For Select instruction we need to act on operands 1 and 2
    if (isa<SelectInst>(&I))
    {
        Src1 = I.getOperand(1);
        Src2 = I.getOperand(2);
    }
    else
    {
        Src1 = I.getOperand(0);
        Src2 = I.getOperand(1);
    }

    unsigned quotient, promoteToInt, Src1width;

    if (!Src1->getType()->isIntOrIntVectorTy())
        return; // Legalization for int types only

    Src1width = Src1->getType()->getScalarSizeInBits();

    if (isLegalInteger(Src1width) || Src1width == 1) // nothing to legalize
        return;

    promoteInt(Src1width, quotient, promoteToInt, DL->getLargestLegalIntTypeSizeInBits());

    if (promoteToInt == Src1width) {
        return; // nothing to do
    }

    Value* NewLargeSrc1 = m_builder->CreateZExt(Src1, Type::getIntNTy(I.getContext(), promoteToInt * quotient));
    Value* NewLargeSrc2 = m_builder->CreateZExt(Src2, Type::getIntNTy(I.getContext(), promoteToInt * quotient));

    if (quotient > 1)
    {
        Value* NewLargeSrc1VecForm = m_builder->CreateBitCast(NewLargeSrc1,
            IGCLLVM::FixedVectorType::get(llvm::Type::getIntNTy(I.getContext(), promoteToInt), quotient));
        Value* NewLargeSrc2VecForm = m_builder->CreateBitCast(NewLargeSrc2,
            IGCLLVM::FixedVectorType::get(llvm::Type::getIntNTy(I.getContext(), promoteToInt), quotient));
        Value* NewLargeResVecForm = UndefValue::get(IGCLLVM::FixedVectorType::get(llvm::Type::getIntNTy(I.getContext(), promoteToInt), quotient));

        bool instSupported = true;
        for (unsigned Idx = 0; Idx < quotient; Idx++)
        {
            Value* NewInst = NULL;
            switch (I.getOpcode()) {
            case Instruction::And:
                NewInst = m_builder->CreateAnd(m_builder->CreateExtractElement(NewLargeSrc1VecForm, Idx),
                    m_builder->CreateExtractElement(NewLargeSrc2VecForm, Idx));
                break;
            case Instruction::Or:
                NewInst = m_builder->CreateOr(m_builder->CreateExtractElement(NewLargeSrc1VecForm, Idx),
                    m_builder->CreateExtractElement(NewLargeSrc2VecForm, Idx));
                break;
            case Instruction::Xor:
                NewInst = m_builder->CreateXor(m_builder->CreateExtractElement(NewLargeSrc1VecForm, Idx),
                    m_builder->CreateExtractElement(NewLargeSrc2VecForm, Idx));
                break;
            case Instruction::LShr:
            {
                if (auto val = dyn_cast<ConstantInt>(Src2))
                {
                    int64_t offset = val->getSExtValue();
                    IGC_ASSERT(offset >= 0 && offset < (int64_t)Src1width);
                    IGC_ASSERT(quotient == 2);
                    if (offset < promoteToInt)
                    {
                        NewInst = m_builder->CreateLShr(m_builder->CreateExtractElement(NewLargeSrc1VecForm, Idx), offset);
                        if (Idx == 0)
                        {
                            Value* shrVal = NewInst;
                            Value* shlVal = m_builder->CreateShl(m_builder->CreateExtractElement(NewLargeSrc1VecForm, 1), promoteToInt - offset);
                            NewInst = m_builder->CreateOr(shrVal, shlVal);
                        }
                    }
                    else
                    {
                        if (Idx == 0)
                            NewInst = m_builder->CreateLShr(m_builder->CreateExtractElement(NewLargeSrc1VecForm, 1), offset - promoteToInt);
                        else
                            NewInst = ConstantInt::get(IntegerType::get(I.getContext(), promoteToInt), 0, false);
                    }
                }
                else
                {
                    instSupported = false;
                    IGC_ASSERT_MESSAGE(0, "Shift by amount is not a constant.");
                }
                break;
            }
            case Instruction::Mul:
                IGC_ASSERT(quotient == 2);
                if (Idx == 0)
                {
                    NewInst = m_builder->CreateMul(m_builder->CreateExtractElement(NewLargeSrc1VecForm, Idx),
                        m_builder->CreateExtractElement(NewLargeSrc2VecForm, Idx));
                }
                else
                {
                    Type* type = llvm::Type::getIntNTy(I.getContext(), promoteToInt);
                    Function* MulHFunc = llvm::GenISAIntrinsic::getDeclaration(
                        m_builder->GetInsertBlock()->getParent()->getParent(),
                        llvm::GenISAIntrinsic::GenISA_umulH,
                        type);

                    Value* Lo1 = m_builder->CreateExtractElement(NewLargeSrc1VecForm, uint64_t(0));
                    Value* Hi1 = m_builder->CreateExtractElement(NewLargeSrc1VecForm, uint64_t(1));
                    Value* Lo2 = m_builder->CreateExtractElement(NewLargeSrc2VecForm, uint64_t(0));
                    Value* Hi2 = m_builder->CreateExtractElement(NewLargeSrc2VecForm, uint64_t(1));

                    Value* MulHiLo1Lo2 = m_builder->CreateCall(MulHFunc, { Lo1, Lo2 });
                    Value* MulLo1Hi2 = m_builder->CreateMul(Lo1, Hi2);
                    Value* MulLo2Hi1 = m_builder->CreateMul(Lo2, Hi1);
                    Value* AddLoHi = m_builder->CreateAdd(MulLo1Hi2, MulLo2Hi1);
                    Value* AddMulHi = m_builder->CreateAdd(AddLoHi, MulHiLo1Lo2);
                    uint64_t mask = (1ULL << (Src1width - promoteToInt)) - 1;
                    NewInst = m_builder->CreateAnd(AddMulHi, mask);
                }
                break;
            case Instruction::Add:
                instSupported = false;
                IGC_ASSERT_MESSAGE(0, "Add Instruction seen with 'large' illegal int type. Legalization support missing.");
                break;
            case Instruction::ICmp:
                instSupported = false;
                IGC_ASSERT_MESSAGE(0, "ICmp Instruction seen with 'large' illegal int type. Legalization support missing.");
                break;
            case Instruction::Select:
                instSupported = false;
                IGC_ASSERT_MESSAGE(0, "Select Instruction seen with 'large' illegal int type. Legalization support missing.");
                break;
            default:
                printf("Binary Instruction seen with illegal int type. Legalization support missing. Inst opcode:%d", I.getOpcode());
                IGC_ASSERT_MESSAGE(0, "Binary Instruction seen with illegal int type. Legalization support missing.");
                break;
            }
            if (instSupported)
                NewLargeResVecForm = m_builder->CreateInsertElement(NewLargeResVecForm, NewInst, Idx);
            else
                break;
        }
        if (instSupported) {
            // Re-bitcast vector into Large illegal type which is to be in turn trunc'ed to original illegal type
            NewLargeSrc1 = m_builder->CreateBitCast(NewLargeResVecForm, NewLargeSrc1->getType());
            Value* NewIllegal = m_builder->CreateTrunc(NewLargeSrc1, Src1->getType());

            I.replaceAllUsesWith(NewIllegal);
            I.eraseFromParent();
        }
    }
    else {
        Value* NewLargeRes = NULL;
        Value* NewIllegal = NULL;
        switch (I.getOpcode()) {
        case Instruction::And:
            NewLargeRes = m_builder->CreateAnd(NewLargeSrc1, NewLargeSrc2);
            break;
        case Instruction::Or:
            NewLargeRes = m_builder->CreateOr(NewLargeSrc1, NewLargeSrc2);
            break;
        case Instruction::Xor:
            NewLargeRes = m_builder->CreateXor(NewLargeSrc1, NewLargeSrc2);
            break;
        case Instruction::Add:
            NewLargeRes = m_builder->CreateAdd(NewLargeSrc1, NewLargeSrc2);
            break;
        case Instruction::Sub:
            NewLargeRes = m_builder->CreateSub(NewLargeSrc1, NewLargeSrc2);
            break;
        case Instruction::ICmp:
        {
            CmpInst* cmpInst = cast<ICmpInst>(&I);
            if (cmpInst->isSigned())
            {
                // Must use sext [note that NewLargeSrc1/2 are zext]
                int shiftAmt = promoteToInt - Src1width;
                IGC_ASSERT_MESSAGE(shiftAmt > 0, "Should not happen, something wrong!");
                Value* V1 = m_builder->CreateShl(NewLargeSrc1, shiftAmt);
                Value* PromotedSrc1 = m_builder->CreateAShr(V1, shiftAmt);
                Value* V2 = m_builder->CreateShl(NewLargeSrc2, shiftAmt);
                Value* PromotedSrc2 = m_builder->CreateAShr(V2, shiftAmt);
                NewLargeRes = m_builder->CreateICmp(cmpInst->getPredicate(), PromotedSrc1, PromotedSrc2);
            }
            else
            {
                NewLargeRes = m_builder->CreateICmp(cmpInst->getPredicate(), NewLargeSrc1, NewLargeSrc2);
            }
            NewIllegal = NewLargeRes;
            break;
        }
        case Instruction::Select:
        {
            SelectInst* selectInst = cast<SelectInst>(&I);
            NewLargeRes = m_builder->CreateSelect(selectInst->getCondition(), NewLargeSrc1, NewLargeSrc2);
            break;
        }
        case Instruction::LShr:
            NewLargeRes = m_builder->CreateLShr(NewLargeSrc1, NewLargeSrc2);
            break;
        case Instruction::AShr:
        {
            NewLargeRes = m_builder->CreateAShr(NewLargeSrc1, NewLargeSrc2);
            break;
        }
        case Instruction::Shl:
        {
            NewLargeRes = m_builder->CreateShl(NewLargeSrc1, NewLargeSrc2);
            break;
        }
        case Instruction::Mul:
        {
            Value* Mul = m_builder->CreateMul(NewLargeSrc1, NewLargeSrc2);
            uint64_t mask = (1ULL << Src1width) - 1;
            NewLargeRes = m_builder->CreateAnd(Mul, mask);
            break;
        }
        default:
            printf("Binary Instruction seen with illegal int type. Legalization support missing. Inst opcode:%d", I.getOpcode());
            IGC_ASSERT_MESSAGE(0, "Binary Instruction seen with illegal int type. Legalization support missing.");
            break;
        }

        if (!NewIllegal)
            NewIllegal = m_builder->CreateTrunc(NewLargeRes, Src1->getType());

        I.replaceAllUsesWith(NewIllegal);
        I.eraseFromParent();
    }
    Changed = true;
}

void PeepholeTypeLegalizer::legalizeUnaryInstruction(Instruction& I) {
    switch (I.getOpcode()) {
    case Instruction::BitCast:
    {
        /*
        sample pattern:
        %1 = bitcast i8 addrspace(1)* %7 to i70 addrspace(1)*
        %2 = load i70, i70 addrspace(1)* %1 align 1
        ALU(i70)
        -->
        %1 = bitcast i8 addrspace(1)* %7 to {i64, i8} addrspace(1)*
        %2 = bitcast <9 x i8> addrspace(1)* %1 to i70 addrspace(1)*
        %3 = load i70, i70 addrspace(1)* %2, align 1
        ALU(i70)

        OR

        %1 = bitcast i8 addrspace(1)* %7 to i24 addrspace(1)*
        %2 = load i24, i24 addrspace(1)* %1, align 1
        %3 = zext i24 %2 to i32
        %4 = and i32 undef, -16777216
        %5 = or i32 %4, %3
        -->
        %1 = bitcast i8 addrspace(1)* %7 to i32 addrspace(1)*
        %2 = bitcast i32 addrspace(1)* %1 to i24 addrspace(1)*
        %3 = load i24, i24 addrspace(1)* %2, align 1
        %4 = zext i24 %2 to i32
        %5 = and i32 undef, -16777216
        %6 = or i32 %4, %3

        */

        if (!I.getType()->isVectorTy() && I.getType()->isPointerTy()) {
            if (isLegalInteger(DL->getPointerTypeSizeInBits(I.getType())) || DL->getPointerTypeSizeInBits(I.getType()) == 1)
                return;

            unsigned quotient, promoteToInt, Src1width = DL->getPointerTypeSizeInBits(I.getType());
            promoteInt(Src1width, quotient, promoteToInt, 8);// byte level addressing

            if (quotient > 1)
            {
                Type* I8xXTy = IGCLLVM::FixedVectorType::get(m_builder->getInt8Ty(), quotient);
                Type* I8xXPtrTy = PointerType::get(I8xXTy, I.getType()->getPointerAddressSpace());

                Value* newBitCastToVec = m_builder->CreateBitCast(I.getOperand(0), I8xXPtrTy);
                Value* newBitCastToScalar = m_builder->CreateBitCast(newBitCastToVec, I.getType());

                I.replaceAllUsesWith(newBitCastToScalar);
                I.eraseFromParent();
            }
            else {
                Value* newUpBitCast = m_builder->CreateBitCast(I.getOperand(0), Type::getIntNPtrTy(I.getContext(), promoteToInt, I.getType()->getPointerAddressSpace()));
                Value* newDownBitCast = m_builder->CreateBitCast(newUpBitCast, I.getType());

                I.replaceAllUsesWith(newDownBitCast);
                I.eraseFromParent();
            }
            Changed = true;
        }
    }
    break;
    case Instruction::Load:
    {
        /* !!! LEGALIZE to i(quotient * promoteToInt) */
        /*
        sample pattern:
        %1 = bitcast i8 addrspace(1)* %7 to i70 addrspace(1)*
        %2 = load i70, i70 addrspace(1)* %1 align 1
        ALU(i70)

        -->
        %1 = bitcast i8 addrspace(1)* %7 to i70 addrspace(1)*
        %2 = bitcast i70 addrspace(1)* to <9 x i8> addrspace(1)*
        %3 = load <9 x i8>, <9 x i8> addrspace(1)*, align 1
        %4 = bitcast <9 x i8> %3 to i72
        %5 = zext i72, i128
        %6 = trunc i128 %5, i70
        ALU(i70)

        OR

        %1 = bitcast i8 addrspace(1)* %7 to i24 addrspace(1)*
        %2 = load i24, i24 addrspace(1)* %1, align 1
        ALU(i24)

        -->
        %1 = bitcast i8 addrspace(1)* %7 to i24 addrspace(1)*
        %2 = bitcast i24 addrspace(1)* %1 to i32 addrspace(1)*
        %3 = load i32, i24 addrspace(1)* %2, align 1
        %4 = trunc %2, i24
        ALU(i24)

        */

        if (isLegalInteger(I.getType()->getScalarSizeInBits()) || I.getType()->getScalarSizeInBits() == 1)
            return; // Nothing to legalize

        unsigned loadQuotient, loadPromoteToInt, loadSrcWidth = DL->getPointerTypeSizeInBits(I.getOperand(0)->getType());
        promoteInt(loadSrcWidth, loadQuotient, loadPromoteToInt, 8); // hard coded to 8 since our hardware is bte addressable.

        if (loadQuotient > 1) {
            Value* newBitCast = m_builder->CreatePointerCast(I.getOperand(0), IGCLLVM::FixedVectorType::get(llvm::Type::getIntNTy(I.getContext(), loadPromoteToInt), loadQuotient));
            Value* newLoadInst = m_builder->CreateLoad(newBitCast);
            // mask the extra bits loaded for type legalization
            unsigned maskCnt = (loadPromoteToInt * loadQuotient) - loadSrcWidth;
            unsigned char mask = 0xff;
            for (unsigned i = 0; i < maskCnt; ++i) {
                mask >>= 1;
            }
            Value* maskedHighByte = m_builder->CreateAnd(m_builder->CreateExtractElement(newLoadInst, loadQuotient - 1), mask);
            Value* newMaskedLoad = m_builder->CreateInsertElement(newLoadInst, maskedHighByte, loadQuotient - 1);
            Value* newBitCastBackToScalar = m_builder->CreateBitCast(newMaskedLoad, Type::getIntNTy(I.getContext(), loadPromoteToInt * loadQuotient));

            //extend new large scalar to a scalar length of legal vector of MAX_LEGAL_INT
            unsigned quotient, promoteToInt, SrcWidth = DL->getPointerTypeSizeInBits(I.getOperand(0)->getType());
            promoteInt(SrcWidth, quotient, promoteToInt, DL->getLargestLegalIntTypeSizeInBits());

            Value* newZextInst = m_builder->CreateZExt(newBitCastBackToScalar, Type::getIntNTy(I.getContext(), promoteToInt * quotient));
            Value* newTruncInst = m_builder->CreateTrunc(newZextInst, I.getType());

            I.replaceAllUsesWith(newTruncInst);
            I.eraseFromParent();
        }
        else {
            Value* newBitCast = m_builder->CreatePointerCast(I.getOperand(0), IGCLLVM::FixedVectorType::get(llvm::Type::getIntNTy(I.getContext(), loadPromoteToInt), loadQuotient));
            Value* newLoadInst = m_builder->CreateLoad(newBitCast);
            // mask the extra bits loaded for type legalization
            unsigned maskCnt = (loadPromoteToInt * loadQuotient) - loadSrcWidth;
            unsigned char mask = 0xff;
            for (unsigned i = 0; i < maskCnt; ++i) {
                mask >>= 1;
            }
            newLoadInst = m_builder->CreateAnd(newLoadInst, mask);
            Value* newTruncInst = m_builder->CreateTrunc(newLoadInst, I.getType());

            I.replaceAllUsesWith(newTruncInst);
            I.eraseFromParent();
        }
        Changed = true;
    }
    break;
    case Instruction::SExt:
    {
        if (isLegalInteger(I.getType()->getScalarSizeInBits()) || I.getType()->getScalarSizeInBits() == 1)
            return; // Nothing to legalize

        if (I.getOperand(0)->getType()->getIntegerBitWidth() == 1)
        {
            unsigned quotient, promoteToInt, Src1width = I.getOperand(0)->getType()->getIntegerBitWidth();
            promoteInt(Src1width, quotient, promoteToInt, DL->getLargestLegalIntTypeSizeInBits());

            Value* Src1 = I.getOperand(0);
            Value* newZextInst = m_builder->CreateZExt(Src1, Type::getIntNTy(I.getContext(), promoteToInt * quotient));
            Value* newtruncInst = m_builder->CreateTrunc(newZextInst, I.getType());

            I.replaceAllUsesWith(newtruncInst);
            I.eraseFromParent();

            Changed = true;
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "SExt Instruction seen with illegal int type and BitWidth > 1. Legalization support missing.");
        }
    }
    break;
    case Instruction::Trunc:
    {
        // %1 = trunc i192 %0 to i128
        // -->
        // %1 = bitcast i192 %0 to <3 x i64>
        // %2 = extractelement %1, 0
        // %3 = insertelement %undef, %2, 0
        // %4 = extractelement %1, 1
        // %5 = insertelement %3, %4, 1
        // %6 = bitcast <2 x i64> %5 to i128
        unsigned dstSize = I.getType()->getScalarSizeInBits();
        unsigned srcSize = I.getOperand(0)->getType()->getScalarSizeInBits();

        if (isLegalInteger(srcSize) && isLegalInteger(dstSize)) // nothing to legalize
            return;

        // Find the largest common denominator that's also a legal type size
        unsigned promotedInt = 0;
        for (unsigned i = DL->getLargestLegalIntTypeSizeInBits(); i >= 8; i >>= 1)
        {
            if (dstSize % i == 0 && srcSize % i == 0)
            {
                promotedInt = i;
                break;
            }
        }

        if (promotedInt == 0) // common denominator not found
        {
            return;
        }

        unsigned numSrcElements = srcSize / promotedInt;
        unsigned numDstElements = dstSize / promotedInt;
        Type* srcVecTy = IGCLLVM::FixedVectorType::get(Type::getIntNTy(I.getContext(), promotedInt), numSrcElements);
        Type* dstVecTy = IGCLLVM::FixedVectorType::get(Type::getIntNTy(I.getContext(), promotedInt), numDstElements);

        // Bitcast the illegal src type to a legal vector
        Value* srcVec = m_builder->CreateBitCast(I.getOperand(0), srcVecTy);
        Value* dstVec = UndefValue::get(dstVecTy);

        for (unsigned i = 0; i < numDstElements; i++)
        {
            Value* v = m_builder->CreateExtractElement(srcVec, m_builder->getInt32(i));
            dstVec = m_builder->CreateInsertElement(dstVec, v, m_builder->getInt32(i));
        }
        // Cast back to original dst type
        Value* result = m_builder->CreateBitCast(dstVec, I.getType());
        I.replaceAllUsesWith(result);
        I.eraseFromParent();
        Changed = true;
    }
    break;
    case Instruction::Store:
        // 1. load byte padded data from pointer
        // 2. mask out the all bits except top padded ones.
        // 3. zext the incoming ILLEGAL to byte padded value
        // 4. OR the zext'ed value and masked load.
        // 5. store the OR'ed value into byte padded size pointer
        IGC_ASSERT_MESSAGE(0, "Store Instruction seen with illegal int type. Legalization support missing.");
        break;
    }
}


void PeepholeTypeLegalizer::cleanupZExtInst(Instruction& I) {
    unsigned srcSize = I.getOperand(0)->getType()->getScalarSizeInBits();
    if (isLegalInteger(srcSize) || srcSize == 1)  // objective is to clean up any intermediate ILLEGAL ints
        return;

    Instruction* prevInst = dyn_cast<Instruction>(I.getOperand(0));
    if (!prevInst)
        return;

    switch (prevInst->getOpcode()) {
    case Instruction::Trunc:
    {
        //then we only need to mask out the truncated bits
        unsigned quotient, promoteToInt, Src1width = prevInst->getOperand(0)->getType()->getIntegerBitWidth();
        promoteInt(Src1width, quotient, promoteToInt, DL->getLargestLegalIntTypeSizeInBits());

        int activeBits = prevInst->getType()->getIntegerBitWidth(); // or I.getoperand(0)->getType()->getIntegerBitWidth();

        if (promoteToInt * quotient == Src1width) { // As in the example sighted below
            if (quotient > 1) {
                /*
                sample pattern:
                ALU
                ...
                %1.    insertelement %? i64, <2 x i64>, 1
                %2.    bitcast %1 <2 x i64>, i128
                %3.    trunc %2 i128, i80
                %4.    zext %3 i80, i128
                %5.    bitcast %4 i128 , <2 x i64>
                %6.    extractelement %5 i64, <2 x i64>, 0
                ...
                ALU

                {also applies for :
                %2.    bitcast %1 <1 x i64>, i64
                }
                -->
                ALU
                ...
                %1.    insertelement %? i64, <2 x i64>, 1
                %2.    bitcast %1 <2 x i64>, i128
                %3.    bitcast %2, <2 x i64>
                %4.    extractelement %3, 0
                %5.    and %4, mask_0
                %6.    insertelement %5, 0
                %7.    bitcast %6, i128
                %8.    bitcast %7 i128 , <2 x i64>
                %9.    extractelement %5 i64, <2 x i64>, 0
                ...
                ALU
                */
                if ((promoteToInt * quotient) == I.getType()->getScalarSizeInBits()) { // rhis is the case for all trunc-zext pairs generated by first step of this legalization pass
                    Value* truncSrcAsVec = m_builder->CreateBitCast(prevInst->getOperand(0),
                        IGCLLVM::FixedVectorType::get(llvm::Type::getIntNTy(I.getContext(), promoteToInt), quotient));
                    Value* vecRes = UndefValue::get(IGCLLVM::FixedVectorType::get(llvm::Type::getIntNTy(I.getContext(), promoteToInt), quotient));
                    Value* Elmt;
                    Value* maskedElmt;

                    for (unsigned Idx = 0; Idx < quotient; ++Idx)
                    {
                        Elmt = m_builder->CreateExtractElement(truncSrcAsVec, Idx);
                        uint64_t mask = (1ULL << std::min(promoteToInt, activeBits - Idx * promoteToInt)) - 1;
                        maskedElmt = m_builder->CreateAnd(Elmt, mask);
                        vecRes = m_builder->CreateInsertElement(vecRes, maskedElmt, Idx);
                    }
                    Value* bitcastBackToScalar = m_builder->CreateBitCast(vecRes, I.getType());

                    I.replaceAllUsesWith(bitcastBackToScalar);
                    I.eraseFromParent();
                    Changed = true;
                }
                else {
                    // this is a place holder, but DO NOT expect to need an implementation for this case.
                }
            }
            else {
                /*
                sample pattern:
                ALU1
                ...
                %1.    trunc %ALU1 i64, i37
                %2.    zext %1 i37, i64
                ...
                ALU2 (%2)
                -->
                ALU
                ...
                %1. and i64 %ALU1, mask
                ...
                ALU2 (%1)
                */
                uint64_t mask = (1ULL << activeBits) - 1;
                Value* maskedElmt = m_builder->CreateAnd(prevInst->getOperand(0), mask);
                Value* cleanedUpInst = NULL;
                if (I.getType()->getScalarSizeInBits() < maskedElmt->getType()->getScalarSizeInBits())
                    cleanedUpInst = m_builder->CreateTrunc(maskedElmt, I.getType());
                else if (I.getType()->getScalarSizeInBits() > maskedElmt->getType()->getScalarSizeInBits())
                    cleanedUpInst = m_builder->CreateZExt(maskedElmt, I.getType());
                else
                    cleanedUpInst = maskedElmt;

                I.replaceAllUsesWith(cleanedUpInst);
                I.eraseFromParent();
                Changed = true;
            }
        }
        else { // (promoteToInt*quotient != Src1width) case
               // No support yet
        }
    }
    break;
    case Instruction::BitCast:
    {
        /*
        Does this handle the first bitcast-zext? :: NO

        %1.    bitcast %src <5 x i16>, i80
        %2.    zext %1 i80, i128
        %3.    bitcast %2, <2x i64>
        %4.    extractelement %3, 0
        ...
        ALU (%4)
        -->
        %1.    convert %src <5 x i16>, <2 x i64>
        %2.    bitcast <2 x i64>, i128
        %3.    bitcast %2, <2 x i64>
        ...
        ALU (%7)
        */

        unsigned quotient, promoteToInt, srcWidth = I.getOperand(0)->getType()->getScalarSizeInBits();
        promoteInt(srcWidth, quotient, promoteToInt, DL->getLargestLegalIntTypeSizeInBits());

        if (quotient * promoteToInt != I.getType()->getScalarSizeInBits()) {
            IGC_ASSERT_MESSAGE(0, "Target size of zext is also illegal and needs promotion to a legal int or vec of largest legal int. Support for this extra legalization is not implemented yet.");
            return;
        }

        unsigned ipElmtSize = prevInst->getOperand(0)->getType()->getScalarSizeInBits();
        unsigned ipVecSize = (unsigned)cast<IGCLLVM::FixedVectorType>(prevInst->getOperand(0)->getType())->getNumElements();
        unsigned convFactor = promoteToInt / ipElmtSize;

        Value* vecRes = UndefValue::get(IGCLLVM::FixedVectorType::get(llvm::Type::getIntNTy(I.getContext(), promoteToInt), quotient));
        Type* resElmtTy = Type::getIntNTy(I.getContext(), promoteToInt);
        unsigned Idx = 0;

        for (unsigned o = 0; o < quotient; ++o) {
            Value* NewVal, * Hi;
            unsigned i = 0;

            NewVal = m_builder->CreateZExt(
                m_builder->CreateExtractElement(prevInst->getOperand(0), m_builder->getIntN(promoteToInt, Idx)), resElmtTy);
            ++Idx;
            if (++i < convFactor) {
                if (Idx < ipVecSize) {
                    Hi = m_builder->CreateZExt(
                        m_builder->CreateExtractElement(prevInst->getOperand(0), m_builder->getIntN(promoteToInt, Idx)), resElmtTy);
                    NewVal = m_builder->CreateOr(m_builder->CreateShl(Hi, ipElmtSize), NewVal);
                    ++Idx;
                    vecRes = m_builder->CreateInsertElement(vecRes, NewVal, o);
                }
                else {
                    /*
                      %32 is "NewVal" above. Idx==convFactor==3.
                      %24 = bitcast <3 x i16> %23 to i48
                      %25 = zext i48 %24 to i64
                      %26 = bitcast i64 %25 to <2 x i32>
                      ---->
                      %24 = extractelement <3 x i16> %23, i32 0
                      %25 = zext i16 %24 to i32
                      %26 = extractelement <3 x i16> %23, i32 1
                      %27 = zext i16 %26 to i32
                      %28 = shl i32 %27, 16
                      %29 = or i32 %28, %25
                      %30 = insertelement <2 x i32> undef, i32 %29, i64 0
                      %31 = extractelement <3 x i16> %23, i32 2
                      %32 = zext i16 %31 to i32
                      %33 = insertelement <2 x i32> %30, i32 %32, i64 1
                    */
                    vecRes = m_builder->CreateInsertElement(vecRes, NewVal, o);
                    break;
                }
            }
        }
        Value* bitcastBackToScalar = m_builder->CreateBitCast(vecRes, I.getType());

        I.replaceAllUsesWith(bitcastBackToScalar);
        I.eraseFromParent();
        Changed = true;
    }
    break;
    case Instruction::ZExt:
    {
        /*

        %1.    zext %1 i32, i33
        %2.    zext %2 i33, i64
        ...
        ALU
        -->
        %1.    zext %1 i32, i64
        ...
        ALU
        */
        I.setOperand(0, prevInst->getOperand(0));
    }
    break;
    default:
        IGC_ASSERT_MESSAGE(0, "Unhandled source to ZExt Instruction seen with illegal int type. Legalization support missing.");
        break;
    }
    if (prevInst->use_empty())
    {
        prevInst->eraseFromParent();
        Changed = true;
    }

}

void PeepholeTypeLegalizer::cleanupTruncInst(Instruction& I) {
    //Only cleanup if the type is illegal and it is not a dead instruction and that
    //the src is a legal type
    if (!isLegalInteger(I.getType()->getScalarSizeInBits()) &&
        I.hasOneUse() &&
        isLegalInteger(I.getOperand(0)->getType()->getScalarSizeInBits()))
    {
        //Need to see if it is safe to replace, combine, or wipe out
        Value* new_inst = NULL;
        Instruction* castInst = I.user_back();
        auto Src = I.getOperand(0);
        auto Src_bitsize = Src->getType()->getScalarSizeInBits();
        auto Trunc_bitsize = I.getType()->getScalarSizeInBits();
        auto castInst_bitsize = castInst->getType()->getScalarSizeInBits();
        if (Src_bitsize <= castInst_bitsize)
        {
            //Example 1:
            //%a = trunc i8 %in to i5
            //%out = sext i5 %a to i32
            //=>
            //%q = zext i8 %in to i32
            //%s = shl i32 %q, 27
            //%out = ashr i32 %s, 27

            //Example 2:
            //%a = trunc i8 %in to i5
            //%out = zext i5 %a to i32
            //=>
            //%q = and i8 %in, 31
            //%out = zext i8 %q to i32

            if (isa<SExtInst>(castInst))
            {
                auto shiftAmt = castInst_bitsize - Trunc_bitsize;
                auto inst1 = m_builder->CreateZExt(Src, castInst->getType());
                auto inst2 = m_builder->CreateShl(inst1, shiftAmt);
                new_inst = m_builder->CreateAShr(inst2, shiftAmt);
            }
            else if (isa<ZExtInst>(castInst))
            {
                auto inst1 = m_builder->CreateAnd(Src, (1 << Trunc_bitsize) - 1);
                new_inst = m_builder->CreateZExt(inst1, castInst->getType());
            }
        }
        else if (Src_bitsize > castInst_bitsize)
        {
            //Most likely a trunc instruction lets combine these two truncs and try again
            //Example:
            //%261 = trunc i8 %260 to i5
            //%262 = trunc i5 %261 to i3
            //------>
            //%out = trunc i8 %260 to i3
            if (isa<TruncInst>(castInst))
            {
                auto new_val = m_builder->CreateTrunc(Src, castInst->getType());
                castInst->replaceAllUsesWith(new_val);
                if (auto *new_trunc = dyn_cast<TruncInst>(new_val))
                    cleanupTruncInst(*new_trunc);
                Changed = true;
            }
        }

        if (new_inst != NULL)
        {
            castInst->replaceAllUsesWith(new_inst);
            Changed = true;
        }
    }

    if (I.use_empty())
    {
        I.eraseFromParent();
        Changed = true;
    }

    return;
}

void PeepholeTypeLegalizer::cleanupBitCastInst(Instruction& I) {

    /*
    Need to handle:
    1.    bitcast
    2.    bitcast addrspace*

    a.    bitcast iSrc , iILLEGAL
        bitcast iILLEGAL, iSrc
    b.    bitcast iSrc, iILLEGAL
        bitcast iILLEGAL, iLEGAL
    */

    Instruction* prevInst = dyn_cast<Instruction>(I.getOperand(0));
    if (!prevInst)
        return;
    switch (prevInst->getOpcode()) {
    case Instruction::BitCast:
    {
        Type* srcType = prevInst->getOperand(0)->getType();
        Type* dstType = I.getType();
        if (srcType == dstType)
        {
            //then we have a redundant pair of bitcast
            /*
            sample pattern:
            ALU
            ...
            %1.    insertelement %? i64, <2 x i64>, 1
            %2.    bitcast %1 <2 x i64>, i128
            %3. bitcast %2, <2 x i64>
            %4.    extractelement %3, 0
            %5.    and %4, mask_0
            %6.    insertelement %5, 0
            %7.    bitcast %6, i128
            %8.    bitcast %7 i128 , <2 x i64>
            %9.    extractelement %5 i64, <2 x i64>, 0
            ...
            ALU
            -->
            ALU
            ...
            %1.    insertelement %? i64, <2 x i64>, 1
            %2.    extractelement %1, 0
            %3.    and %2, mask_0
            %4.    insertelement %3, 0
            %5.    extractelement %4 i64, <2 x i64>, 0
            ...
            ALU
            */
            I.replaceAllUsesWith(prevInst->getOperand(0));
            I.eraseFromParent();
            Changed = true;
        }
        else
        {
            // Handles 1.b Directly bitcasts iSrc to iLegal
            m_builder->SetInsertPoint(&I);
            Value* newBitcast = m_builder->CreateBitCast(prevInst->getOperand(0), I.getType());
            I.replaceAllUsesWith(newBitcast);
            I.eraseFromParent();
            Changed = true;
        }
        // We may be able to remove the first bitcast
        if (prevInst->use_empty())
        {
            prevInst->eraseFromParent();
            Changed = true;
        }
        break;
    }
    default:
        break;
    }
}
