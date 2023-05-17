/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/RegisterPressureEstimate.hpp"
#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/LowerGEPForPrivMem.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

#define MAX_ALLOCA_PROMOTE_GRF_NUM      48
#define MAX_PRESSURE_GRF_NUM            90

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace IGC {
    /// @brief  LowerGEPForPrivMem pass is used for lowering the allocas identified while visiting the alloca instructions
    ///         and then inserting insert/extract elements instead of load stores. This allows us
    ///         to store the data in registers instead of propagating it to scratch space.
    class LowerGEPForPrivMem : public llvm::FunctionPass, public llvm::InstVisitor<LowerGEPForPrivMem>
    {
    public:
        LowerGEPForPrivMem();

        ~LowerGEPForPrivMem() {}

        virtual StringRef getPassName() const override
        {
            return IGCOpts::LowerGEPForPrivMemPass;
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<RegisterPressureEstimate>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        void visitAllocaInst(llvm::AllocaInst& I);

        unsigned int extractConstAllocaSize(llvm::AllocaInst* pAlloca);

        static bool IsVariableSizeAlloca(llvm::AllocaInst& pAlloca);

    private:
        llvm::AllocaInst* createVectorForAlloca(
            llvm::AllocaInst* pAlloca,
            llvm::Type* pBaseType);
        void handleAllocaInst(llvm::AllocaInst* pAlloca);

        StatusPrivArr2Reg CheckIfAllocaPromotable(llvm::AllocaInst* pAlloca);
        bool IsNativeType(Type* type);

    public:
        static char ID;

        struct PromotedLiverange
        {
            unsigned int lowId;
            unsigned int highId;
            unsigned int varSize;
            RegisterPressureEstimate::LiveRange* LR;
        };

    private:
        const llvm::DataLayout* m_pDL = nullptr;
        CodeGenContext* m_ctx = nullptr;
        DominatorTree* m_DT = nullptr;
        std::vector<llvm::AllocaInst*> m_allocasToPrivMem;
        RegisterPressureEstimate* m_pRegisterPressureEstimate = nullptr;
        llvm::Function* m_pFunc = nullptr;
        MetaDataUtils* pMdUtils = nullptr;

        /// Keep track of each BB affected by promoting MemtoReg and the current pressure at that block
        llvm::DenseMap<llvm::BasicBlock*, unsigned> m_pBBPressure;

        std::vector<PromotedLiverange> m_promotedLiveranges;
    };

    FunctionPass* createPromotePrivateArrayToReg()
    {
        return new LowerGEPForPrivMem();
    }
}

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
    IGC_ASSERT(pAlloca != nullptr);
    IGCLLVM::IRBuilder<> IRB(pAlloca);
    AllocaInst* pAllocaValue = nullptr;
    if (IsVariableSizeAlloca(*pAlloca)) {
        pAllocaValue = IRB.CreateAlloca(pBaseType, pAlloca->getArraySize());

    } else {
        IGC_ASSERT(nullptr != m_pDL);
        const unsigned int denominator = int_cast<unsigned int>(m_pDL->getTypeAllocSize(pBaseType));
        IGC_ASSERT(0 < denominator);
        const unsigned int totalSize = extractConstAllocaSize(pAlloca) / denominator;
        pAllocaValue = IRB.CreateAlloca(IGCLLVM::FixedVectorType::get(pBaseType, totalSize));
    }

    return pAllocaValue;
}

bool LowerGEPForPrivMem::runOnFunction(llvm::Function& F)
{
    m_pFunc = &F;
    CodeGenContextWrapper* pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
    IGC_ASSERT(nullptr != pCtxWrapper);
    m_ctx = pCtxWrapper->getCodeGenContext();
    m_DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    if (isOptDisabledForFunction(m_ctx->getModuleMetaData(), getPassName(), &F))
        return false;

    pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    IGC_ASSERT(nullptr != pMdUtils);
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return false;
    }
    IGC_ASSERT(nullptr != F.getParent());
    m_pDL = &F.getParent()->getDataLayout();
    m_pRegisterPressureEstimate = &getAnalysis<RegisterPressureEstimate>();
    IGC_ASSERT(nullptr != m_pRegisterPressureEstimate);
    // if no live range info
    if (!m_pRegisterPressureEstimate->isAvailable())
    {
        return false;
    }
    m_pRegisterPressureEstimate->buildRPMapPerInstruction();

    m_allocasToPrivMem.clear();
    visit(F);

    std::vector<llvm::AllocaInst*>& allocaToHande = m_allocasToPrivMem;
    for (auto pAlloca : allocaToHande)
    {
        handleAllocaInst(pAlloca);
    }

    // Last remove alloca instructions
    for (auto pInst : allocaToHande)
    {
        if (pInst->use_empty())
        {
            pInst->eraseFromParent();
        }
    }

    // IR changed only if we had alloca instruction to optimize
    return !allocaToHande.empty();
}

void TransposeHelper::EraseDeadCode()
{
    for (auto pInst = m_toBeRemovedGEP.rbegin(); pInst != m_toBeRemovedGEP.rend(); ++pInst)
    {
        IGC_ASSERT_MESSAGE((*pInst)->use_empty(), "Instruction still has usage");
        (*pInst)->eraseFromParent();
    }
}

bool LowerGEPForPrivMem::IsVariableSizeAlloca(llvm::AllocaInst& pAlloca)
{
    IGC_ASSERT(nullptr != pAlloca.getArraySize());
    if (isa<ConstantInt>(pAlloca.getArraySize()))
        return false;
    return true;
}

unsigned int LowerGEPForPrivMem::extractConstAllocaSize(llvm::AllocaInst* pAlloca)
{
    IGC_ASSERT(nullptr != m_pDL);
    IGC_ASSERT(nullptr != pAlloca);
    IGC_ASSERT(nullptr != pAlloca->getArraySize());
    IGC_ASSERT(nullptr != pAlloca->getAllocatedType());
    unsigned int arraySize = int_cast<unsigned int>(cast<ConstantInt>(pAlloca->getArraySize())->getZExtValue());
    unsigned int totalArrayStructureSize = int_cast<unsigned int>(m_pDL->getTypeAllocSize(pAlloca->getAllocatedType()) * arraySize);

    return totalArrayStructureSize;
}

static void GetAllocaLiverange(Instruction* I, unsigned int& liverangeStart, unsigned int& liverangeEnd,
    RegisterPressureEstimate* rpe, SmallVector<LowerGEPForPrivMem::PromotedLiverange, 16>& GEPliveranges)
{
    IGC_ASSERT(nullptr != I);

    for (Value::user_iterator use_it = I->user_begin(), use_e = I->user_end(); use_it != use_e; ++use_it)
    {
        if (isa<GetElementPtrInst>(*use_it) || isa<BitCastInst>(*use_it))
        {
            // collect liveranges for GEP operations related to alloca
            Instruction* Inst = cast<Instruction>(*use_it);
            LowerGEPForPrivMem::PromotedLiverange GEPliverange;
            GEPliverange.LR = rpe->getLiveRangeOrNull(Inst);
            GEPliverange.lowId = GEPliverange.highId = rpe->getAssignedNumberForInst(Inst);
            GetAllocaLiverange(Inst, GEPliverange.lowId, GEPliverange.highId, rpe, GEPliveranges);
            GEPliverange.varSize = rpe->getRegisterWeightForInstruction(Inst);

            if (GEPliverange.LR)
                GEPliveranges.push_back(GEPliverange);

            liverangeStart = std::min(liverangeStart, GEPliverange.lowId);
            liverangeEnd = std::max(liverangeEnd, GEPliverange.highId);
        }
        else if (isa<LoadInst>(*use_it) || isa<StoreInst>(*use_it) || isa<llvm::IntrinsicInst>(*use_it))
        {
            unsigned int idx = rpe->getAssignedNumberForInst(cast<Instruction>(*use_it));
            liverangeStart = std::min(liverangeStart, idx);
            liverangeEnd = std::max(liverangeEnd, idx);
        }
    }
}

bool LowerGEPForPrivMem::IsNativeType(Type* type)
{
    if (type->isDoubleTy() && m_ctx->platform.hasNoFP64Inst())
    {
        return false;
    }

    if (type->isIntegerTy(8) &&
        (IGC_IS_FLAG_ENABLED(ForcePromoteI8) ||
         (IGC_IS_FLAG_ENABLED(EnablePromoteI8) && !m_ctx->platform.supportByteALUOperation())))
    {
        // Byte indirect: not supported for Vx1 and VxH on PVC.
        // As GRF from promoted privMem may use indirect accesses, disable it
        // to prevent Vx1 and VxH accesses.
        return false;
    }

    return true;
}

StatusPrivArr2Reg LowerGEPForPrivMem::CheckIfAllocaPromotable(llvm::AllocaInst* pAlloca)
{
    // vla is not promotable
    IGC_ASSERT(pAlloca != nullptr);
    if (IsVariableSizeAlloca(*pAlloca))
        return StatusPrivArr2Reg::IsDynamicAlloca;

    bool isUniformAlloca = pAlloca->getMetadata("uniform") != nullptr;
    bool useAssumeUniform = pAlloca->getMetadata("UseAssumeUniform") != nullptr;
    unsigned int allocaSize = extractConstAllocaSize(pAlloca);
    unsigned int allowedAllocaSizeInBytes = MAX_ALLOCA_PROMOTE_GRF_NUM * 4;
    unsigned int SIMDSize = numLanes(SIMDMode::SIMD8);

    // scale alloc size based on the number of GRFs we have
    float grfRatio = m_ctx->getNumGRFPerThread() / 128.0f;
    allowedAllocaSizeInBytes = (uint32_t)(allowedAllocaSizeInBytes * grfRatio);

    if (m_ctx->type == ShaderType::OPENCL_SHADER)
    {
        FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(m_pFunc);
        SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
        if (subGroupSize->hasValue())
        {
            SIMDSize = (uint32_t)subGroupSize->getSIMD_size();
            allowedAllocaSizeInBytes = (allowedAllocaSizeInBytes * 8) / SIMDSize;
        }
    }
    Type* baseType = nullptr;
    if (!CanUseSOALayout(pAlloca, baseType))
    {
        return StatusPrivArr2Reg::CannotUseSOALayout;
    }
    if (!IsNativeType(baseType))
    {
        return StatusPrivArr2Reg::IsNotNativeType;
    }
    if (isUniformAlloca)
    {
        // Heuristic: for uniform alloca we divide the size by SIMDSize to adjust the pressure
        // as they will be allocated as uniform array
        allocaSize = iSTD::Round(allocaSize, SIMDSize) / SIMDSize;
    }

    if (useAssumeUniform || allocaSize <= IGC_GET_FLAG_VALUE(ByPassAllocaSizeHeuristic))
    {
        return StatusPrivArr2Reg::OK;
    }

    // if alloca size exceeds alloc size threshold, return false
    if (allocaSize > allowedAllocaSizeInBytes)
    {
        return StatusPrivArr2Reg::OutOfAllocSizeLimit;
    }

    // get all the basic blocks that contain the uses of the alloca
    // then estimate how much changing this alloca to register adds to the pressure at that block.
    unsigned int lowestAssignedNumber = 0xFFFFFFFF;
    unsigned int highestAssignedNumber = 0;
    SmallVector<PromotedLiverange, 16> GEPliveranges;

    GetAllocaLiverange(pAlloca, lowestAssignedNumber, highestAssignedNumber, m_pRegisterPressureEstimate, GEPliveranges);

    uint32_t maxGRFPressure = (uint32_t)(grfRatio * MAX_PRESSURE_GRF_NUM * 4);

    unsigned int pressure = 0;
    for (unsigned int i = lowestAssignedNumber; i <= highestAssignedNumber; i++)
    {
        // subtract impact from GEP operations related to alloca from the register pressure
        // since after promotion alloca to register these GEPs will be eliminated
        unsigned int GEPImpact = 0;
        for (auto GEPinst : GEPliveranges)
        {
            if (GEPinst.LR->contains(i))
                GEPImpact += GEPinst.varSize;
        }

        unsigned RPinst = m_pRegisterPressureEstimate->getRegisterPressureForInstructionFromRPMap(i);
        pressure = std::max(pressure, RPinst - GEPImpact);
    }

    for (auto it : m_promotedLiveranges)
    {
        // check interval intersection
        if ((it.lowId < lowestAssignedNumber && it.highId > lowestAssignedNumber) ||
            (it.lowId > lowestAssignedNumber && it.lowId < highestAssignedNumber))
        {
            pressure += it.varSize;
        }
    }

    if (allocaSize + pressure > maxGRFPressure)
    {
        return StatusPrivArr2Reg::OutOfMaxGRFPressure;
    }
    PromotedLiverange liverange;
    liverange.lowId = lowestAssignedNumber;
    liverange.highId = highestAssignedNumber;
    liverange.varSize = allocaSize;
    liverange.LR = nullptr;
    m_promotedLiveranges.push_back(liverange);
    return StatusPrivArr2Reg::OK;
}

static bool CheckUsesForSOAAlyout(Instruction* I, bool& vectorSOA)
{
    for (Value::user_iterator use_it = I->user_begin(), use_e = I->user_end(); use_it != use_e; ++use_it)
    {
        if (GetElementPtrInst * gep = dyn_cast<GetElementPtrInst>(*use_it))
        {
            if (CheckUsesForSOAAlyout(gep, vectorSOA))
                continue;
        }
        if (llvm::LoadInst * pLoad = llvm::dyn_cast<llvm::LoadInst>(*use_it))
        {
            vectorSOA &= pLoad->getType()->isVectorTy();
            if (!pLoad->isSimple())
                return false;
        }
        else if (llvm::StoreInst * pStore = llvm::dyn_cast<llvm::StoreInst>(*use_it))
        {
            if (!pStore->isSimple())
                return false;
            llvm::Value* pValueOp = pStore->getValueOperand();
            vectorSOA &= pStore->getValueOperand()->getType()->isVectorTy();
            if (pValueOp == I)
            {
                // GEP instruction is the stored value of the StoreInst (not supported case)
                return false;
            }
        }
        else if (llvm::BitCastInst * pBitCast = llvm::dyn_cast<llvm::BitCastInst>(*use_it))
        {
            Type* baseT = GetBaseType(IGCLLVM::getNonOpaquePtrEltTy(pBitCast->getType()));
            Type* sourceType = GetBaseType(IGCLLVM::getNonOpaquePtrEltTy(pBitCast->getOperand(0)->getType()));
            if (pBitCast->use_empty())
            {
                continue;
            }
            else if (baseT != nullptr &&
                baseT->getScalarSizeInBits() != 0 &&
                baseT->getScalarSizeInBits() == sourceType->getScalarSizeInBits())
            {
                vectorSOA &= (unsigned int)baseT->getPrimitiveSizeInBits() == sourceType->getPrimitiveSizeInBits();
                if (CheckUsesForSOAAlyout(pBitCast, vectorSOA))
                    continue;
            }
            else if (IsBitCastForLifetimeMark(pBitCast))
            {
                continue;
            }
            // Not a candidate.
            return false;
        }
        else if (IntrinsicInst * intr = dyn_cast<IntrinsicInst>(*use_it))
        {
            llvm::Intrinsic::ID  IID = intr->getIntrinsicID();
            if (IID == llvm::Intrinsic::lifetime_start ||
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


bool IGC::CanUseSOALayout(AllocaInst* I, Type*& base)
{
    // Do not allow SOA layout for vla which will be stored on the stack.
    // We don't support SOA layout for privates on stack at all so this is just to make
    // the implementation simpler.
    if (LowerGEPForPrivMem::IsVariableSizeAlloca(*I))
        return false;

    // Don't even look at non-array allocas.
    // (extractAllocaDim can not handle them anyway, causing a crash)
    llvm::Type* pType = IGCLLVM::getNonOpaquePtrEltTy(I->getType());
    if (pType->isStructTy() && pType->getStructNumElements() == 1)
    {
        pType = pType->getStructElementType(0);
    }
    if ((!pType->isArrayTy() && !pType->isVectorTy()) || I->isArrayAllocation())
        return false;

    base = GetBaseType(pType);
    if (base == nullptr)
        return false;
    // only handle case with a simple base type
    if (!(base->getScalarType()->isFloatingPointTy() || base->getScalarType()->isIntegerTy()))
        return false;
    bool vectorSOA = true;
    bool useSOA = CheckUsesForSOAAlyout(I, vectorSOA);
    if (!vectorSOA)
    {
        base = base->getScalarType();
    }
    return useSOA;
}

void LowerGEPForPrivMem::visitAllocaInst(AllocaInst& I)
{
    // Alloca should always be private memory
    IGC_ASSERT(nullptr != I.getType());
    IGC_ASSERT(I.getType()->getAddressSpace() == ADDRESS_SPACE_PRIVATE);

    StatusPrivArr2Reg status = CheckIfAllocaPromotable(&I);
    if (I.getType()->getAddressSpace() == ADDRESS_SPACE_PRIVATE)
    {
        m_ctx->metrics.CollectMem2Reg(&I, status);
    }
    if (status != StatusPrivArr2Reg::OK)
    {
        // alloca size extends remain per-lane-reg space
        return;
    }
    m_allocasToPrivMem.push_back(&I);
}

void TransposeHelper::HandleAllocaSources(Instruction* v, Value* idx)
{
    SmallVector<Value*, 10> instructions;
    for (Value::user_iterator it = v->user_begin(), e = v->user_end(); it != e; ++it)
    {
        Value* inst = cast<Value>(*it);
        instructions.push_back(inst);
    }

    for (auto instruction : instructions)
    {
        if (GetElementPtrInst * pGEP = dyn_cast<GetElementPtrInst>(instruction))
        {
            handleGEPInst(pGEP, idx);
        }
        else if (BitCastInst * bitcast = dyn_cast<BitCastInst>(instruction))
        {
            m_toBeRemovedGEP.push_back(bitcast);
            HandleAllocaSources(bitcast, idx);
        }
        else if (StoreInst * pStore = llvm::dyn_cast<StoreInst>(instruction))
        {
            handleStoreInst(pStore, idx);
        }
        else if (LoadInst * pLoad = llvm::dyn_cast<LoadInst>(instruction))
        {
            handleLoadInst(pLoad, idx);
        }
        else if (IntrinsicInst * inst = dyn_cast<IntrinsicInst>(instruction))
        {
            handleLifetimeMark(inst);
        }
    }
}

class TransposeHelperPromote : public TransposeHelper
{
public:
    void handleLoadInst(
        LoadInst* pLoad,
        Value* pScalarizedIdx);
    void handleStoreInst(
        StoreInst* pStore,
        Value* pScalarizedIdx);
    void handleLifetimeMark(IntrinsicInst* inst);
    AllocaInst* pVecAlloca;
    // location of lifetime starts
    llvm::SmallPtrSet<Instruction*, 4> pStartPoints;
    TransposeHelperPromote(AllocaInst* pAI) : TransposeHelper(false) { pVecAlloca = pAI; }
};

void LowerGEPForPrivMem::handleAllocaInst(llvm::AllocaInst* pAlloca)
{
    // Extract the Alloca size and the base Type
    Type* pType = IGCLLVM::getNonOpaquePtrEltTy(pAlloca->getType());
    Type* pBaseType = GetBaseType(pType)->getScalarType();
    IGC_ASSERT(pBaseType);
    llvm::AllocaInst* pVecAlloca = createVectorForAlloca(pAlloca, pBaseType);
    if (!pVecAlloca)
    {
        return;
    }

    IRBuilder<> IRB(pVecAlloca);
    Value* idx = IRB.getInt32(0);
    TransposeHelperPromote helper(pVecAlloca);
    helper.HandleAllocaSources(pAlloca, idx);
    IGC_ASSERT(nullptr != pAlloca);
    // for uniform alloca, we need to insert an initial definition
    // to keep the promoted vector as uniform in the next round of WIAnalysis
    bool isUniformAlloca = pAlloca->getMetadata("uniform") != nullptr;
    if (isUniformAlloca && pAlloca->getAllocatedType()->isArrayTy())
    {
        if (helper.pStartPoints.empty())
            helper.pStartPoints.insert(pAlloca);
        for (auto InsertionPoint : helper.pStartPoints)
        {
            IRBuilder<> IRB1(InsertionPoint);
            auto pVecF = GenISAIntrinsic::getDeclaration(m_pFunc->getParent(),
                GenISAIntrinsic::GenISA_vectorUniform, pVecAlloca->getAllocatedType());
            auto pVecInit = IRB1.CreateCall(pVecF);
            // create a store of pVecInit into pVecAlloca
            IRB1.CreateStore(pVecInit, pVecAlloca);
        }
    }
    helper.EraseDeadCode();
    if (pAlloca->use_empty())
    {
        IGC_ASSERT(m_DT);
        replaceAllDbgUsesWith(*pAlloca, *pVecAlloca, *pVecAlloca, *m_DT);
    }
}

void TransposeHelper::handleGEPInst(
    llvm::GetElementPtrInst* pGEP,
    llvm::Value* idx)
{
    IGC_ASSERT(nullptr != pGEP);
    IGC_ASSERT(static_cast<ADDRESS_SPACE>(pGEP->getPointerAddressSpace()) == ADDRESS_SPACE_PRIVATE);
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
    Value* pScalarizedIdx = IRB.getInt32(0);
    Type* T = IGCLLVM::getNonOpaquePtrEltTy(pGEP->getPointerOperandType());
    for (unsigned i = 0, e = pGEP->getNumIndices(); i < e; ++i)
    {
        auto GepOpnd = IRB.CreateZExtOrTrunc(pGEP->getOperand(i + 1), IRB.getInt32Ty());
        unsigned int arr_sz = 1;
        if (T->isStructTy())
        {
            arr_sz = 1;
            T = T->getStructElementType(0);
        }
        else if (T->isArrayTy())
        {
            arr_sz = int_cast<unsigned int>(T->getArrayNumElements());
            T = T->getArrayElementType();
        }
        else if (T->isVectorTy())
        {
            // based on whether we want the index in number of element or number of vector
            if (m_vectorIndex)
            {
                arr_sz = 1;
            }
            else
            {
                arr_sz = (unsigned)cast<IGCLLVM::FixedVectorType>(T)->getNumElements();
            }
            T = cast<VectorType>(T)->getElementType();
        }

        pScalarizedIdx = IRB.CreateNUWAdd(pScalarizedIdx, GepOpnd);
        pScalarizedIdx = IRB.CreateNUWMul(pScalarizedIdx, IRB.getInt32(arr_sz));
    }
    while (T->isStructTy() || T->isArrayTy() || T->isVectorTy()) {
        unsigned int arr_sz = 1;
        if (T->isStructTy())
        {
            IGC_ASSERT(T->getStructNumElements() == 1);
            T = T->getStructElementType(0);
        }
        else if (T->isArrayTy())
        {
            arr_sz = int_cast<unsigned int>(T->getArrayNumElements());;
            T = T->getArrayElementType();
        }
        else if (T->isVectorTy())
        {
            arr_sz = (unsigned)cast<IGCLLVM::FixedVectorType>(T)->getNumElements();
            T = cast<VectorType>(T)->getElementType();
        }
        else
        {
            IGC_ASSERT(0);
        }
        pScalarizedIdx = IRB.CreateNUWMul(pScalarizedIdx, IRB.getInt32(arr_sz));
    }
    pScalarizedIdx = IRB.CreateNUWAdd(pScalarizedIdx, idx);
    HandleAllocaSources(pGEP, pScalarizedIdx);
}

// Load N elements from a vector alloca, Idx, ... Idx + N - 1. Return a scalar
// or a vector value depending on N.
static Value* loadEltsFromVecAlloca(
    unsigned N, AllocaInst* pVecAlloca,
    Value* pScalarizedIdx,
    IGCLLVM::IRBuilder<>& IRB,
    Type* scalarType)
{
    Value* pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
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
    IGC_ASSERT_MESSAGE((N > 1), "out of sync");
    Type* Ty = IGCLLVM::FixedVectorType::get(scalarType, N);
    Value* Result = UndefValue::get(Ty);

    for (unsigned i = 0; i < N; ++i)
    {
        Value* VectorIdx = ConstantInt::get(pScalarizedIdx->getType(), i);
        auto Idx = IRB.CreateAdd(pScalarizedIdx, VectorIdx);
        auto Val = IRB.CreateExtractElement(pLoadVecAlloca, Idx);
        Val = IRB.CreateBitCast(Val, scalarType);
        Result = IRB.CreateInsertElement(Result, Val, VectorIdx);
    }
    return Result;
}

void TransposeHelperPromote::handleLoadInst(
    LoadInst* pLoad,
    Value* pScalarizedIdx)
{
    IGC_ASSERT(nullptr != pLoad);
    IGC_ASSERT(pLoad->isSimple());
    IGCLLVM::IRBuilder<> IRB(pLoad);
    IGC_ASSERT(nullptr != pLoad->getType());
    unsigned N = pLoad->getType()->isVectorTy()
        ? (unsigned)cast<IGCLLVM::FixedVectorType>(pLoad->getType())->getNumElements()
        : 1;
    Value* Val = loadEltsFromVecAlloca(N, pVecAlloca, pScalarizedIdx, IRB, pLoad->getType()->getScalarType());
    pLoad->replaceAllUsesWith(Val);
    pLoad->eraseFromParent();
}

void TransposeHelperPromote::handleStoreInst(
    llvm::StoreInst* pStore,
    llvm::Value* pScalarizedIdx)
{
    // Add Store instruction to remove list
    IGC_ASSERT(nullptr != pStore);
    IGC_ASSERT(pStore->isSimple());

    IGCLLVM::IRBuilder<> IRB(pStore);
    llvm::Value* pStoreVal = pStore->getValueOperand();
    llvm::Value* pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
    llvm::Value* pIns = pLoadVecAlloca;
    IGC_ASSERT(nullptr != pStoreVal);
    IGC_ASSERT(nullptr != pStoreVal->getType());
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
        for (unsigned i = 0, e = (unsigned)cast<IGCLLVM::FixedVectorType>(pStoreVal->getType())->getNumElements(); i < e; ++i)
        {
            Value* VectorIdx = ConstantInt::get(pScalarizedIdx->getType(), i);
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
    pStore->eraseFromParent();
}

void TransposeHelperPromote::handleLifetimeMark(IntrinsicInst* inst)
{
    IGC_ASSERT(nullptr != inst);
    IGC_ASSERT((inst->getIntrinsicID() == llvm::Intrinsic::lifetime_start) ||
        (inst->getIntrinsicID() == llvm::Intrinsic::lifetime_end));
    if (inst->getIntrinsicID() == llvm::Intrinsic::lifetime_start)
    {
        pStartPoints.insert(inst);
    }
    m_toBeRemovedGEP.push_back(inst);
}
