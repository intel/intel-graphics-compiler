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

#include "common/LLVMUtils.h"

#include "Compiler/CISACodeGen/LowerGEPForPrivMem.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

#define MAX_ALLOCA_PROMOTE_GRF_NUM      48
#define MAX_PRESSURE_GRF_NUM            64

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-priv-mem-to-reg"
#define PASS_DESCRIPTION "Lower GEP of Private Memory to Register Pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowerGEPForPrivMem, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(RegisterPressureEstimate)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LowerGEPForPrivMem, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char LowerGEPForPrivMem::ID = 0;

LowerGEPForPrivMem::LowerGEPForPrivMem() : FunctionPass(ID), m_pFunc(nullptr)
{
    initializeLowerGEPForPrivMemPass(*PassRegistry::getPassRegistry());
}

llvm::AllocaInst* LowerGEPForPrivMem::createVectorForAlloca(
    llvm::AllocaInst* pAlloca,
    llvm::Type* pBaseType)
{
    IRBuilder<> IRB(pAlloca);

    unsigned int totalSize = extractAllocaSize(pAlloca) / int_cast<unsigned int>(m_pDL->getTypeAllocSize(pBaseType));

    llvm::VectorType* pVecType = llvm::VectorType::get(pBaseType, totalSize);

    AllocaInst *pAllocaValue = IRB.CreateAlloca(pVecType, 0);
    return pAllocaValue;
}

bool LowerGEPForPrivMem::runOnFunction(llvm::Function &F)
{
    m_pFunc = &F;
    CodeGenContextWrapper* pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
    m_ctx = pCtxWrapper->getCodeGenContext();

    MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return false;
    }
    m_pDL = &F.getParent()->getDataLayout();
    m_pRegisterPressureEstimate = &getAnalysis<RegisterPressureEstimate>();

    m_allocasToPrivMem.clear();

    visit(F);

    m_toBeRemovedGEP.clear();
    m_toBeRemovedLoadStore.clear();

    std::vector<llvm::AllocaInst*> &allocaToHande = m_allocasToPrivMem;
    for (auto pAlloca : allocaToHande)
    {
        handleAllocaInst(pAlloca);
    }

    // Delete handled Alloca, GEP, Store and Load instructions
    // First remove load/store instructions
    for (auto pInst : m_toBeRemovedLoadStore)
    {
        assert(pInst->use_empty() && "Instruction still has usage");
        pInst->eraseFromParent();
    }

    // Second remove GEP instructions
    for (auto pInst = m_toBeRemovedGEP.rbegin(); pInst != m_toBeRemovedGEP.rend(); ++pInst)
    {
        assert((*pInst)->use_empty() && "Instruction still has usage");
        (*pInst)->eraseFromParent();
    }

    // Last remove alloca instructions
    for (auto pInst : allocaToHande)
    {
        if (pInst->use_empty())
        {
            pInst->eraseFromParent();
        }
    }

    if (!allocaToHande.empty())
        DumpLLVMIR(m_ctx, "AfterLowerGEP");
    // IR changed only if we had alloca instruction to optimize
    return !allocaToHande.empty();
}

unsigned int LowerGEPForPrivMem::extractAllocaSize(llvm::AllocaInst* pAlloca)
{
    unsigned int arraySize = int_cast<unsigned int>(cast<ConstantInt>(pAlloca->getArraySize())->getZExtValue());
    unsigned int totalArrayStructureSize = int_cast<unsigned int>(m_pDL->getTypeAllocSize(pAlloca->getAllocatedType()) * arraySize);

    return totalArrayStructureSize;
}

bool LowerGEPForPrivMem::CheckIfAllocaPromotable(llvm::AllocaInst* pAlloca)
{
    unsigned int allocaSize = extractAllocaSize(pAlloca);
    unsigned int allowedAllocaSizeInBytes = MAX_ALLOCA_PROMOTE_GRF_NUM * 4;

    if (m_ctx->type == ShaderType::COMPUTE_SHADER)
    {
        ComputeShaderContext* ctx = static_cast<ComputeShaderContext*>(m_ctx);
        SIMDMode simdMode = ctx->GetLeastSIMDModeAllowed();
        unsigned d = simdMode == SIMDMode::SIMD32 ? 4 : 1;

        allowedAllocaSizeInBytes = allowedAllocaSizeInBytes / d;
    }

    // if alloca size exceeds alloc size threshold, return false
    if (allocaSize > allowedAllocaSizeInBytes)
    {
        return false;
    }
    // if no live range info
    if (!m_pRegisterPressureEstimate->isAvailable())
    {
        return true;
    }

    bool isUniformAlloca = true;
    bool allocaCandidate = ValidUses(pAlloca, isUniformAlloca);
    if(!allocaCandidate)
    {
        return false;
    }
    if(isUniformAlloca)
    {
        // Heuristic: for uniform alloca we divide the size by 8 to adjust the pressure
        // as they will be allocated as uniform array
        allocaSize = iSTD::Round(allocaSize, 8) / 8;
    }
    // get all the basic blocks that contain the uses of the alloca
    // then estimate how much changing this alloca to register adds to the pressure at that block.
    unsigned int assignedNumber = 0;
    unsigned int lowestAssignedNumber = m_pRegisterPressureEstimate->getMaxAssignedNumberForFunction();
    unsigned int highestAssignedNumber = 0;

    for (auto II = pAlloca->user_begin(), IE = pAlloca->user_end(); II != IE; ++II)
    {
        if (Instruction* inst = dyn_cast<Instruction>(*II))
        {
            assignedNumber = m_pRegisterPressureEstimate->getAssignedNumberForInst(inst);
            lowestAssignedNumber = (lowestAssignedNumber < assignedNumber) ? lowestAssignedNumber : assignedNumber;
            highestAssignedNumber = (highestAssignedNumber > assignedNumber) ? highestAssignedNumber : assignedNumber;
        }
    }
    
    // find all the BB's that lie in the liverange of lowestAssignedNumber 
    // and highestAssignedNumber for the use of the alloca instruction
    auto &BBs = m_pFunc->getBasicBlockList();
    DenseSet<BasicBlock*> bbList;
    for (auto BI = BBs.begin(), BE = BBs.end(); BI != BE; ++BI)
    {
        BasicBlock *BB = &*BI;
        unsigned int bbMaxAssignedNumber = m_pRegisterPressureEstimate->getMaxAssignedNumberForBB(BB);
        unsigned int bbMinAssignedNumber = m_pRegisterPressureEstimate->getMinAssignedNumberForBB(BB);
        if (((lowestAssignedNumber >= bbMinAssignedNumber) && (lowestAssignedNumber <= bbMaxAssignedNumber)) ||
            ((bbMinAssignedNumber >= lowestAssignedNumber) && (bbMinAssignedNumber <= highestAssignedNumber)))
        {
            if (!m_pBBPressure.count(BB))
            {
                m_pBBPressure[BB] = m_pRegisterPressureEstimate->getRegisterPressure(BB);
            }
 
            if (allocaSize + m_pBBPressure[BB] > MAX_PRESSURE_GRF_NUM*4)
            {
                return false;
            }

            bbList.insert(BB);
        }
    }

    for(auto it : bbList)
    {
        m_pBBPressure[it] += allocaSize;
    }
    return true;
}

bool LowerGEPForPrivMem::IsUniformAddress(Value* val)
{
    if(isa<Constant>(val))
    {
        return true;
    }
    else if(isa<AllocaInst>(val))
    {
        // once we found the alloca that mean all the calculation was uniform
        return true;
    }
    else if(BitCastInst* bitcast = dyn_cast<BitCastInst>(val))
    {
        return IsUniformAddress(bitcast->getOperand(0));
    }
    else if(GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(val))
    {
        for(unsigned int i = 0; i < gep->getNumOperands(); i++)
        {
            if(!IsUniformAddress(gep->getOperand(i)))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool LowerGEPForPrivMem::IsUniformStore(StoreInst* pStore)
{
    if(pStore->getParent() != &pStore->getParent()->getParent()->getEntryBlock())
    {
        // Conservative logic, only consider the entry block for now
        // We could improve it with dominator analysis or uniform analysis
        return false;
    }
    if(!IsUniformAddress(pStore->getPointerOperand()) || !IsUniformAddress(pStore->getValueOperand()))
    {
        return false;
    }
    return true;
}

static Type* GetBaseType(Type* pType)
{
    if(pType->isStructTy())
    {
        int num_elements = pType->getStructNumElements();
        if(num_elements > 1)
            return nullptr;

        pType = pType->getStructElementType(0);
    }

    while(pType->isArrayTy())
    {
        pType = pType->getArrayElementType();
    }

    Type* pBaseType = nullptr;
    if(pType->isVectorTy())
    {
        pBaseType = pType->getContainedType(0);
    }
    else
    {
        pBaseType = pType;
    }
    return pBaseType;
}

bool LowerGEPForPrivMem::ValidUses(Instruction* I, bool& IsUniform)
{
    for(Value::user_iterator use_it = I->user_begin(), use_e = I->user_end(); use_it != use_e; ++use_it)
    {
        if(GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(*use_it))
        {
            if(ValidUses(gep, IsUniform))
                continue;
        }
        if(llvm::LoadInst* pLoad = llvm::dyn_cast<llvm::LoadInst>(*use_it))
        {
            if(!pLoad->isSimple())
                return false;
        }
        else if(llvm::StoreInst* pStore = llvm::dyn_cast<llvm::StoreInst>(*use_it))
        {
            if(!pStore->isSimple())
                return false;
            llvm::Value* pValueOp = pStore->getValueOperand();
            if(pValueOp == I)
            {
                // GEP instruction is the stored value of the StoreInst (not supported case)
                return false;
            }
            IsUniform &= IsUniformStore(pStore);
        }
        else if(llvm::BitCastInst *pBitCast = llvm::dyn_cast<llvm::BitCastInst>(*use_it))
        {
            Type* baseT = GetBaseType(pBitCast->getType()->getPointerElementType());
            Type* sourceType = GetBaseType(pBitCast->getOperand(0)->getType()->getPointerElementType());
            if(pBitCast->use_empty())
            {
                continue;
            }
            else if(baseT != nullptr &&
                baseT->getPrimitiveSizeInBits() != 0 &&
                baseT->getPrimitiveSizeInBits() == sourceType->getPrimitiveSizeInBits() )
            {
                if(ValidUses(pBitCast, IsUniform))
                    continue;
            }
            else if(IsBitCastForLifetimeMark(pBitCast))
            {
                continue;
            }
            // Not a candidate.
            return false;
        }
        else if(IntrinsicInst* intr = dyn_cast<IntrinsicInst>(*use_it))
        {
            llvm::Intrinsic::ID  IID = intr->getIntrinsicID();
            if(IID == llvm::Intrinsic::lifetime_start ||
                IID == llvm::Intrinsic::lifetime_end)
            {
                continue;
            }
            return false;
        }
        else
        {
            // This is some other instruction. Right now we don't want to handle these
            return false;
        }
    }
    return true;
}

void LowerGEPForPrivMem::visitAllocaInst(AllocaInst &I)
{
    // Don't even look at non-array allocas.
    // (extractAllocaDim can not handle them anyway, causing a crash)
    llvm::Type* pType = I.getType()->getPointerElementType();
    if (pType->isStructTy() && pType->getStructNumElements() == 1)
    {
            pType = pType->getStructElementType(0);
    }
    if ((!pType->isArrayTy() && !pType->isVectorTy()) || I.isArrayAllocation())
        return;

    Type* pBaseType = GetBaseType(pType);
    if(pBaseType == nullptr)
        return;
    // only handle case with a simple base type
    if(!(pBaseType->isFloatingPointTy() || pBaseType->isIntegerTy()))
        return;

    // Alloca should always be private memory
    assert(I.getType()->getAddressSpace() == ADDRESS_SPACE_PRIVATE);

    if (!CheckIfAllocaPromotable(&I))
    {
        // alloca size extends remain per-lane-reg space
        return;
    }
    m_allocasToPrivMem.push_back(&I);
}

void LowerGEPForPrivMem::HandleAllocaSources(
    Instruction* v, AllocaInst* pVecAlloca, Value* idx)
{
    SmallVector<Value*, 10> instructions;
    for(Value::user_iterator it = v->user_begin(), e = v->user_end(); it != e; ++it)
    {
        Value* inst = cast<Value>(*it);
        instructions.push_back(inst);
    }
    
    for(auto instruction : instructions)
    {
        if(GetElementPtrInst *pGEP = dyn_cast<GetElementPtrInst>(instruction))
        {
            handleGEPInst(pGEP, pVecAlloca, idx);
        }
        else if(BitCastInst* bitcast = dyn_cast<BitCastInst>(instruction))
        {
            m_toBeRemovedGEP.push_back(bitcast);
            HandleAllocaSources(bitcast, pVecAlloca, idx);
        }
        else if(StoreInst *pStore = llvm::dyn_cast<StoreInst>(instruction))
        {
            handleStoreInst(pStore, pVecAlloca, idx);
        }
        else if(LoadInst *pLoad = llvm::dyn_cast<LoadInst>(instruction))
        {
            handleLoadInst(pLoad, pVecAlloca, idx);
        }
        else if(IntrinsicInst* inst = dyn_cast<IntrinsicInst>(instruction))
        {
            handleLifetimeMark(inst);
        }
    }
}

void LowerGEPForPrivMem::handleAllocaInst(llvm::AllocaInst* pAlloca)
{
    // Extract the Alloca size and the base Type
    Type* pType = pAlloca->getType()->getPointerElementType();
    Type* pBaseType = GetBaseType(pType);
	assert(pBaseType);
    llvm::AllocaInst* pVecAlloca = createVectorForAlloca(pAlloca, pBaseType);
    if (!pVecAlloca)
    {
        return;
    }
    
    IRBuilder<> IRB(pVecAlloca);
    Value* idx = IRB.getInt32(0);
    HandleAllocaSources(pAlloca, pVecAlloca, idx);
}

void LowerGEPForPrivMem::handleLifetimeMark(IntrinsicInst *inst)
{
    assert(inst->getIntrinsicID() == llvm::Intrinsic::lifetime_start ||
        inst->getIntrinsicID() == llvm::Intrinsic::lifetime_end);
    inst->eraseFromParent();
}

void LowerGEPForPrivMem::handleGEPInst(
    llvm::GetElementPtrInst *pGEP,
    llvm::AllocaInst* pVecAlloca,
    llvm::Value* idx)
{
    assert(static_cast<ADDRESS_SPACE>(pGEP->getPointerAddressSpace()) == ADDRESS_SPACE_PRIVATE);
    // Add GEP instruction to remove list
    m_toBeRemovedGEP.push_back(pGEP);
    if (pGEP->use_empty())
    {
        // GEP has no users, do nothing.
        return;
    }

    // Given %p = getelementptr [4 x [3 x <2 x float>]]* %v, i64 0, i64 %1, i64 %2
    // compute the scalarized index with an auxiliary array [4, 3, 2]:
    //
    // Formula: index = (%1 x 3 + %2) x 2
    //
    IRBuilder<> IRB(pGEP);
    Value *pScalarizedIdx = idx;
    Type* T = pGEP->getPointerOperandType()->getPointerElementType();
    for (unsigned i = 0, e = pGEP->getNumIndices(); i < e; ++i)
    {
        auto GepOpnd = IRB.CreateZExtOrTrunc(pGEP->getOperand(i + 1), IRB.getInt32Ty());
        unsigned int arr_sz = 1;
        if(T->isStructTy())
        {
            arr_sz = 1;
            T = T->getStructElementType(0);
        }
        else if(T->isArrayTy())
        {
            arr_sz = int_cast<unsigned int>(T->getArrayNumElements());
            T = T->getArrayElementType();
        }
        else if(T->isVectorTy())
        {
            arr_sz = T->getVectorNumElements();
            T = T->getVectorElementType();
        }

        pScalarizedIdx = IRB.CreateNUWAdd(pScalarizedIdx, GepOpnd);
        pScalarizedIdx = IRB.CreateNUWMul(pScalarizedIdx, IRB.getInt32(arr_sz));
    }
    HandleAllocaSources(pGEP, pVecAlloca, pScalarizedIdx);
}

// Load N elements from a vector alloca, Idx, ... Idx + N - 1. Return a scalar
// or a vector value depending on N.
static Value *loadEltsFromVecAlloca(
    unsigned N, AllocaInst *pVecAlloca,
    Value *pScalarizedIdx, 
    IRBuilder<> &IRB,
    Type* scalarType)
{
    Value *pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
    if (N == 1)
    {
        return IRB.CreateBitCast(
            IRB.CreateExtractElement(pLoadVecAlloca, pScalarizedIdx),
            scalarType);
    }

    // A vector load
    // %v = load <2 x float>* %ptr
    // becomes
    // %w = load <32 x float>* %ptr1
    // %v0 = extractelement <32 x float> %w, i32 %idx
    // %v1 = extractelement <32 x float> %w, i32 %idx+1
    // replace all uses of %v with <%v0, %v1>
    assert(N > 1 && "out of sync");
    Type* Ty = VectorType::get(scalarType, N);
    Value *Result = UndefValue::get(Ty);

    for(unsigned i = 0; i < N; ++i)
    {
        Value *VectorIdx = ConstantInt::get(pScalarizedIdx->getType(), i);
        auto Idx = IRB.CreateAdd(pScalarizedIdx, VectorIdx);
        auto Val = IRB.CreateExtractElement(pLoadVecAlloca, Idx);
        Val = IRB.CreateBitCast(Val, scalarType);
        Result = IRB.CreateInsertElement(Result, Val, VectorIdx);
    }
    return Result;
}

void LowerGEPForPrivMem::handleLoadInst(
    llvm::LoadInst *pLoad,
    llvm::AllocaInst *pVecAlloca,
    llvm::Value *pScalarizedIdx)
{
    // Add Load instruction to remove list
    m_toBeRemovedLoadStore.push_back(pLoad);
    assert(pLoad->isSimple());
    IRBuilder<> IRB(pLoad);
    unsigned N = pLoad->getType()->isVectorTy()
                     ? pLoad->getType()->getVectorNumElements()
                     : 1;
    Value *Val = loadEltsFromVecAlloca(N, pVecAlloca, pScalarizedIdx, IRB, pLoad->getType()->getScalarType());
    pLoad->replaceAllUsesWith(Val);
}

void LowerGEPForPrivMem::handleStoreInst(
    llvm::StoreInst *pStore,
    llvm::AllocaInst *pVecAlloca,
    llvm::Value *pScalarizedIdx)
{
    // Add Store instruction to remove list
    m_toBeRemovedLoadStore.push_back(pStore);
    assert(pStore->isSimple());

    IRBuilder<> IRB(pStore);
    llvm::Value* pStoreVal = pStore->getValueOperand();
    llvm::Value* pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
    llvm::Value* pIns = pLoadVecAlloca;
    if (pStoreVal->getType()->isVectorTy())
    {
        // A vector store
        // store <2 x float> %v, <2 x float>* %ptr
        // becomes
        // %w = load <32 x float> *%ptr1
        // %v0 = extractelement <2 x float> %v, i32 0
        // %w0 = insertelement <32 x float> %w, float %v0, i32 %idx
        // %v1 = extractelement <2 x float> %v, i32 1
        // %w1 = insertelement <32 x float> %w0, float %v1, i32 %idx+1
        // store <32 x float> %w1, <32 x float>* %ptr1
        for (unsigned i = 0, e = pStoreVal->getType()->getVectorNumElements(); i < e; ++i)
        {
            Value *VectorIdx = ConstantInt::get(pScalarizedIdx->getType(), i);
            auto Val = IRB.CreateExtractElement(pStoreVal, VectorIdx);
            Val = IRB.CreateBitCast(Val, pLoadVecAlloca->getType()->getScalarType());
            auto Idx = IRB.CreateAdd(pScalarizedIdx, VectorIdx);
            pIns = IRB.CreateInsertElement(pIns, Val, Idx);
        }
    }
    else
    {
        pStoreVal = IRB.CreateBitCast(pStoreVal, pLoadVecAlloca->getType()->getScalarType());
        pIns = IRB.CreateInsertElement(pLoadVecAlloca, pStoreVal, pScalarizedIdx);
    }
    IRB.CreateStore(pIns, pVecAlloca);
}
