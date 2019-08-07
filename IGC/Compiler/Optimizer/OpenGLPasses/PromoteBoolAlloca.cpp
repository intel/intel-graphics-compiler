/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Constants.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/helper.h"

using namespace IGC;
using namespace IGC::IGCMD;

class PromoteBoolAllocaPass : public llvm::FunctionPass
{
public:
    PromoteBoolAllocaPass() : FunctionPass(ID)
    {

    }
    static char ID;

    bool runOnFunction(llvm::Function& F) override;

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
    }

    virtual llvm::StringRef getPassName() const override
    {
        return "PromoteBoolAlloca";
    }

private:
    llvm::Value* recreateVectorOrArray(llvm::Value* pV, llvm::Type* pNewTy, llvm::Instruction* pInsertBeforeInstr);
    llvm::Value* recreateStructure(llvm::Value* pV, llvm::Type* pNewTy, llvm::Instruction* pInsertBeforeInstr);
    llvm::Type* getBaseTy(llvm::Type* pTy);
    void visitAlloca(llvm::AllocaInst& I);
    void RecursivelyChangePointerType(llvm::Instruction* oldPtr, llvm::Instruction* newPtr);
    llvm::Type* LegalAllocaType(llvm::Type* type) const;

private:
    std::vector<llvm::Instruction*> m_instructionsToRemove;
    const llvm::DataLayout* m_pDL;
};

char PromoteBoolAllocaPass::ID = 0;

llvm::Pass* createPromoteBoolAllocaPass()
{
    return new PromoteBoolAllocaPass();
}

static llvm::Value* Cast(llvm::Value* pVal, llvm::Type* pType, llvm::Instruction* pInsertBefore)
{
    llvm::Value* pNewVal = nullptr;
    if (pType->isIntegerTy())
    {
        pNewVal = llvm::CastInst::CreateIntegerCast(pVal, pType, false, "", pInsertBefore);
    }
    else if (pType->isFloatingPointTy())
    {
        pNewVal = llvm::CastInst::CreateFPCast(pVal, pType, "", pInsertBefore);
    }
    else
    {
        assert(0 && "unexpected type");
    }
    return pNewVal;
}

llvm::Type* PromoteBoolAllocaPass::getBaseTy(llvm::Type* pTy)
{
    if (pTy->isArrayTy() || pTy->isVectorTy())
        return getBaseTy(pTy->getSequentialElementType());
    else
        return pTy;
}

llvm::Value* PromoteBoolAllocaPass::recreateVectorOrArray(llvm::Value* pV, llvm::Type* pNewTy, llvm::Instruction* pInsertBeforeInstr)
{
    llvm::IRBuilder<> builder(pInsertBeforeInstr->getContext());
    builder.SetInsertPoint(pInsertBeforeInstr);

    llvm::Type* pOriginalValueTy = getBaseTy(pV->getType());
    llvm::Type* pNewValueTy = getBaseTy(pNewTy);

    // We want to only promote from or to i1 types.
    if (pOriginalValueTy->isIntegerTy(1) || pNewValueTy->isIntegerTy(1) || pOriginalValueTy->isStructTy())
    {
        llvm::Value* pLoaded = pV;
        if (pV->getType()->isPointerTy())
            pLoaded = builder.CreateLoad(pV);

        llvm::Type* pBaseTy = getBaseTy(pNewTy);

        llvm::Value* pArr = nullptr;
        if (llvm::isa<llvm::ConstantAggregateZero>(pV))
        {
            pArr = llvm::ConstantAggregateZero::get(pNewTy);
        }
        // Recreate a new array/vector based on the new type
        else if (pV->getType()->isArrayTy())
        {
            assert(pV->getType()->getArrayNumElements() == pNewTy->getArrayNumElements());

            pArr = llvm::UndefValue::get(pNewTy);

            int num_elements = int_cast<int>(pV->getType()->getArrayNumElements());
            for (int i = 0; i < num_elements; ++i)
            {
                llvm::Value* extractedValue = builder.CreateExtractValue(pLoaded, i);
                llvm::Value* elementToInset = nullptr;
                if (pV->getType()->getSequentialElementType()->isVectorTy() ||
                    pV->getType()->getSequentialElementType()->isArrayTy())
                {
                    elementToInset = recreateVectorOrArray(extractedValue, pNewTy->getSequentialElementType(), pInsertBeforeInstr);
                }
                else if (pV->getType()->getSequentialElementType()->isStructTy())
                {
                    elementToInset = recreateStructure(extractedValue, pNewTy->getSequentialElementType(), pInsertBeforeInstr);
                }
                else
                {
                    elementToInset = builder.CreateIntCast(extractedValue, pBaseTy, false);
                }

                pArr = builder.CreateInsertValue(pArr, elementToInset, i);
            }
        }
        else if (pV->getType()->isVectorTy())
        {
            assert(pV->getType()->getVectorNumElements() == pNewTy->getVectorNumElements());

            pArr = llvm::UndefValue::get(pNewTy);

            int num_elements = pV->getType()->getVectorNumElements();
            for (int i = 0; i < num_elements; ++i)
            {
                pArr = builder.CreateInsertElement(pArr, builder.CreateIntCast(builder.CreateExtractElement(pLoaded, builder.getInt32(i)), pBaseTy, false), builder.getInt32(i));
            }
        }

        pV = pArr;
    }

    return pV;
}

llvm::Value* PromoteBoolAllocaPass::recreateStructure(llvm::Value* pV, llvm::Type* pNewTy, llvm::Instruction* pInsertBeforeInstr)
{
    llvm::IRBuilder<> builder(pInsertBeforeInstr->getContext());
    builder.SetInsertPoint(pInsertBeforeInstr);

    assert(pV->getType()->isStructTy());

    llvm::Value* pLoaded = pV;
    if (pV->getType()->isPointerTy())
        pLoaded = builder.CreateLoad(pV);

    llvm::Value* pStruct = llvm::UndefValue::get(pNewTy);

    int num_elements = int_cast<int>(pV->getType()->getStructNumElements());
    for (int i = 0; i < num_elements; ++i)
    {
        llvm::Value* extractedValue = builder.CreateExtractValue(pLoaded, i);
        llvm::Value* elementToInset = nullptr;

        if (pLoaded->getType()->getStructElementType(i)->isArrayTy() ||
            pLoaded->getType()->getStructElementType(i)->isVectorTy())
        {
            elementToInset = recreateVectorOrArray(extractedValue, pNewTy->getStructElementType(i), pInsertBeforeInstr);
        }
        else if (pLoaded->getType()->getStructElementType(i)->isStructTy())
        {
            elementToInset = recreateStructure(extractedValue, pNewTy->getStructElementType(i), pInsertBeforeInstr);
        }
        else
        {
            elementToInset = builder.CreateIntCast(extractedValue, getBaseTy(pNewTy->getStructElementType(i)), false);
        }

        pStruct = builder.CreateInsertValue(pStruct, elementToInset, i);
    }

    return pStruct;
}

void PromoteBoolAllocaPass::RecursivelyChangePointerType(llvm::Instruction* pOldPtr, llvm::Instruction* pNewPtr)
{
    for (auto II = pOldPtr->user_begin(), IE = pOldPtr->user_end(); II != IE; ++II)
    {
        llvm::Value* pNewVal = nullptr;
        if (llvm::GetElementPtrInst * pGep = llvm::dyn_cast<llvm::GetElementPtrInst>(*II))
        {
            llvm::SmallVector<llvm::Value*, 8> Idx(pGep->idx_begin(), pGep->idx_end());
            llvm::GetElementPtrInst* newGep = llvm::GetElementPtrInst::Create(nullptr, pNewPtr, Idx, "", pGep);
            RecursivelyChangePointerType(pGep, newGep);
        }
        else if (llvm::LoadInst * pLoad = llvm::dyn_cast<llvm::LoadInst>(*II))
        {
            llvm::Instruction* pNewLoad = IGC::cloneLoad(pLoad, pNewPtr);
            if (pLoad->getType()->isArrayTy() || pLoad->getType()->isVectorTy())
                pNewVal = recreateVectorOrArray(pNewLoad, pLoad->getType(), pLoad->getNextNode());
            else if (pLoad->getType()->isStructTy())
                pNewVal = recreateStructure(pNewLoad, pLoad->getType(), pLoad->getNextNode());
            else
                pNewVal = Cast(pNewLoad, pLoad->getType(), pLoad->getNextNode());
            pLoad->replaceAllUsesWith(pNewVal);
        }
        else if (llvm::StoreInst * pStore = llvm::dyn_cast<llvm::StoreInst>(*II))
        {
            llvm::Value* pStoredValue = pStore->getValueOperand();
            // If the Stored value is a vector or array treat it, create a new value of the right type
            llvm::Value* pNewData = nullptr;

            if (pStoredValue->getType()->isArrayTy() || pStoredValue->getType()->isVectorTy())
                pNewData = recreateVectorOrArray(pStoredValue, pNewPtr->getType()->getPointerElementType(), pStore);
            else if (pStoredValue->getType()->isStructTy())
                pNewData = recreateStructure(pStoredValue, pNewPtr->getType()->getPointerElementType(), pStore);
            else
                pNewData = Cast(pStoredValue, pNewPtr->getType()->getPointerElementType(), pStore);

            IGC::cloneStore(pStore, pNewData, pNewPtr);
        }
        else if (llvm::CastInst * pCast = llvm::dyn_cast<llvm::CastInst>(*II))
        {
            llvm::Value* newCast = llvm::CastInst::CreatePointerCast(pNewPtr, pCast->getType(), "", pCast);
            pCast->replaceAllUsesWith(newCast);
        }
        // We cannot delete any instructions as the visitor
        m_instructionsToRemove.push_back(llvm::cast<llvm::Instruction>(*II));
    }
}

llvm::Type* PromoteBoolAllocaPass::LegalAllocaType(llvm::Type* pType) const
{
    llvm::Type* pLegalType = pType;
    switch (pType->getTypeID())
    {
    case llvm::Type::IntegerTyID:
        if (pType->isIntegerTy(1))
        {
            unsigned int size = int_cast<unsigned int>(m_pDL->getTypeAllocSizeInBits(pType));
            pLegalType = llvm::Type::getIntNTy(pType->getContext(), size);
        }
        break;
    case llvm::Type::ArrayTyID:
        pLegalType = llvm::ArrayType::get(
            LegalAllocaType(pType->getSequentialElementType()),
            pType->getArrayNumElements());
        break;
    case llvm::Type::VectorTyID:
        pLegalType = llvm::VectorType::get(
            LegalAllocaType(pType->getSequentialElementType()),
            pType->getVectorNumElements());
        break;
    case llvm::Type::StructTyID:
    {
        llvm::StructType* pStructTy = llvm::cast<llvm::StructType>(pType);
        llvm::SmallVector<llvm::Type*, 8> fieldTypes;
        for (auto itt = pStructTy->element_begin(), ite = pStructTy->element_end(); itt != ite; ++itt)
        {
            llvm::Type* LegalTy = LegalAllocaType(*itt);
            fieldTypes.push_back(LegalTy);
        }
        auto* pNewType = llvm::StructType::get(pType->getContext(), fieldTypes);
        if (!pStructTy->isLayoutIdentical(pNewType))
        {
            pLegalType = pNewType;
        }
        break;
    }
    case llvm::Type::HalfTyID:
    case llvm::Type::FloatTyID:
    case llvm::Type::DoubleTyID:
    case llvm::Type::PointerTyID:
        break;
    default:
        assert(0 && "Alloca of unsupported type");
        break;
    }
    return pLegalType;
}

void PromoteBoolAllocaPass::visitAlloca(llvm::AllocaInst& I)
{
    llvm::Type* pType = I.getAllocatedType();
    llvm::Type* pLegalAllocaType = LegalAllocaType(pType);
    if (pType != pLegalAllocaType)
    {
        // Remaining alloca of i1 need to be promoted
        llvm::AllocaInst* newAlloca = new llvm::AllocaInst(pLegalAllocaType, 0, "", &I);
        RecursivelyChangePointerType(&I, newAlloca);
        m_instructionsToRemove.push_back(&I);
    }
}

bool PromoteBoolAllocaPass::runOnFunction(llvm::Function& F)
{
    bool changed_instruction = false;
    m_pDL = &F.getParent()->getDataLayout();

    for (llvm::Function::iterator bb = F.begin(), be = F.end(); bb != be; ++bb)
    {
        for (llvm::BasicBlock::iterator ii = bb->begin(), ie = bb->end();
            ii != ie;
            ++ii)
        {
            if (llvm::AllocaInst * pAlloca = llvm::dyn_cast<llvm::AllocaInst>(ii))
            {
                visitAlloca(*pAlloca);
            }
        }
    }

    for (auto I : m_instructionsToRemove)
    {
        I->eraseFromParent();
    }

    m_instructionsToRemove.clear();

    return changed_instruction;
}
