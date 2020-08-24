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
#define MAX_PRESSURE_GRF_NUM            64

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

        virtual llvm::StringRef getPassName() const override
        {
            return "LowerGEPForPrivMem";
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

        bool CheckIfAllocaPromotable(llvm::AllocaInst* pAlloca);
        bool IsNativeType(Type* type);
        /// Conservatively check if a store allow an Alloca to be uniform
        bool IsUniformStore(llvm::StoreInst* pStore);

    public:
        static char ID;

    private:
        const llvm::DataLayout* m_pDL = nullptr;
        CodeGenContext* m_ctx = nullptr;
        DominatorTree* m_DT = nullptr;
        std::vector<llvm::AllocaInst*> m_allocasToPrivMem;
        RegisterPressureEstimate* m_pRegisterPressureEstimate = nullptr;
        llvm::Function* m_pFunc = nullptr;

        /// Keep track of each BB affected by promoting MemtoReg and the current pressure at that block
        llvm::DenseMap<llvm::BasicBlock*, unsigned> m_pBBPressure;

        struct PromotedLiverange
        {
            unsigned int lowId;
            unsigned int highId;
            unsigned int varSize;
        };

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

    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    IGC_ASSERT(nullptr != pMdUtils);
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return false;
    }
    IGC_ASSERT(nullptr != F.getParent());
    m_pDL = &F.getParent()->getDataLayout();
    m_pRegisterPressureEstimate = &getAnalysis<RegisterPressureEstimate>();
    IGC_ASSERT(nullptr != m_pRegisterPressureEstimate);
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

    if (!allocaToHande.empty())
        DumpLLVMIR(m_ctx, "AfterLowerGEP");
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

static void GetAllocaLiverange(Instruction* I, unsigned int& liverangeStart, unsigned int& liverangeEnd, RegisterPressureEstimate* rpe)
{
    IGC_ASSERT(nullptr != I);

    for (Value::user_iterator use_it = I->user_begin(), use_e = I->user_end(); use_it != use_e; ++use_it)
    {
        if (isa<GetElementPtrInst>(*use_it) || isa<BitCastInst>(*use_it))
        {
            GetAllocaLiverange(cast<Instruction>(*use_it), liverangeStart, liverangeEnd, rpe);
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
    if ((type->isDoubleTy() && m_ctx->platform.hasNoFP64Inst()) ||
        (type->isIntegerTy(64) && m_ctx->platform.hasNoInt64Inst()))
    {
        return false;
    }
    return true;
}

bool LowerGEPForPrivMem::CheckIfAllocaPromotable(llvm::AllocaInst* pAlloca)
{
    // vla is not promotable
    IGC_ASSERT(pAlloca != nullptr);
    if (IsVariableSizeAlloca(*pAlloca))
        return false;

    bool isUniformAlloca = pAlloca->getMetadata("uniform") != nullptr;
    unsigned int allocaSizeInBytes = extractConstAllocaSize(pAlloca);
    unsigned int allowedAllocaSizeInBytes = MAX_ALLOCA_PROMOTE_GRF_NUM * 4;

    // scale alloc size based on the number of GRFs we have.
    float grfRatio = m_ctx->getNumGRFPerThread() / 128.0f;
    allowedAllocaSizeInBytes = (uint32_t)(allowedAllocaSizeInBytes * grfRatio);

    if (m_ctx->type == ShaderType::COMPUTE_SHADER)
    {
        ComputeShaderContext* ctx = static_cast<ComputeShaderContext*>(m_ctx);
        SIMDMode simdMode = ctx->GetLeastSIMDModeAllowed();
        unsigned d = simdMode == SIMDMode::SIMD32 ? 4 : 1;

        allowedAllocaSizeInBytes = allowedAllocaSizeInBytes / d;
    }
    Type* baseType = nullptr;
    if (!CanUseSOALayout(pAlloca, baseType))
    {
        return false;
    }
    if (!IsNativeType(baseType))
    {
        return false;
    }

    const unsigned int hwRegSizeInBytes = 32;
    unsigned int hwRegsPerAlloca = iSTD::Round( allocaSizeInBytes, hwRegSizeInBytes) / hwRegSizeInBytes;

    if (allocaSizeInBytes <= IGC_GET_FLAG_VALUE(ByPassAllocaSizeHeuristic))
    {
        return true;
    }

    // if alloca size exceeds alloc size threshold, return false
    if (isUniformAlloca)
    {
        unsigned int allocaUniform = iSTD::Round(allocaSizeInBytes, 8) / 8;

        if (allocaUniform > allowedAllocaSizeInBytes)
        {
            return false;
        }
    }
    else if (allocaSizeInBytes > allowedAllocaSizeInBytes)
    {
        return false;
    }

    // if no live range info
    if (!m_pRegisterPressureEstimate->isAvailable())
    {
        return true;
    }

    // get all the basic blocks that contain the uses of the alloca
    // then estimate how much changing this alloca to register adds to the pressure at that block.
    unsigned int lowestAssignedNumber = 0xFFFFFFFF;
    unsigned int highestAssignedNumber = 0;

    GetAllocaLiverange(pAlloca, lowestAssignedNumber, highestAssignedNumber, m_pRegisterPressureEstimate);

    uint32_t maxGRFPressure = (uint32_t)(grfRatio * MAX_PRESSURE_GRF_NUM * 4);

    unsigned int pressure = 0;
    for (unsigned int i = lowestAssignedNumber; i <= highestAssignedNumber; i++)
    {
        pressure = std::max(
            pressure, m_pRegisterPressureEstimate->getRegisterPressureForInstructionFromRPMap(i));
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

    if (hwRegsPerAlloca + pressure > maxGRFPressure)
    {
        return false;
    }
    PromotedLiverange liverange;
    liverange.lowId = lowestAssignedNumber;
    liverange.highId = highestAssignedNumber;
    liverange.varSize = hwRegsPerAlloca;
    m_promotedLiveranges.push_back(liverange);
    return true;
}

static Type* GetBaseType(Type* pType)
{
    if (pType->isStructTy())
    {
        int num_elements = pType->getStructNumElements();
        if (num_elements != 1)
            return nullptr;

        pType = pType->getStructElementType(0);
    }

    while (pType->isArrayTy())
    {
        pType = pType->getArrayElementType();
    }

    if (pType->isStructTy())
    {
        int num_elements = pType->getStructNumElements();
        if (num_elements != 1)
            return nullptr;

        pType = pType->getStructElementType(0);
    }
    return pType;
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
            Type* baseT = GetBaseType(pBitCast->getType()->getPointerElementType());
            Type* sourceType = GetBaseType(pBitCast->getOperand(0)->getType()->getPointerElementType());
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
    llvm::Type* pType = I->getType()->getPointerElementType();
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
    if (!CheckIfAllocaPromotable(&I))
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
    AllocaInst* pVecAlloca;
    TransposeHelperPromote(AllocaInst* pAI) : TransposeHelper(false) { pVecAlloca = pAI; }
};

void LowerGEPForPrivMem::handleAllocaInst(llvm::AllocaInst* pAlloca)
{
    // Extract the Alloca size and the base Type
    Type* pType = pAlloca->getType()->getPointerElementType();
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
    helper.EraseDeadCode();
    IGC_ASSERT(nullptr != pAlloca);
    if (pAlloca->use_empty()) {
      IGC_ASSERT(m_DT);
      replaceAllDbgUsesWith(*pAlloca, *pVecAlloca, *pVecAlloca, *m_DT);
    }
}

void TransposeHelper::handleLifetimeMark(IntrinsicInst* inst)
{
    IGC_ASSERT(nullptr != inst);
    IGC_ASSERT((inst->getIntrinsicID() == llvm::Intrinsic::lifetime_start) ||
        (inst->getIntrinsicID() == llvm::Intrinsic::lifetime_end));
    inst->eraseFromParent();
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
    Type* T = pGEP->getPointerOperandType()->getPointerElementType();
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
                arr_sz = (unsigned)cast<VectorType>(T)->getNumElements();
            }
            T = cast<VectorType>(T)->getElementType();
        }

        pScalarizedIdx = IRB.CreateNUWAdd(pScalarizedIdx, GepOpnd);
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
    IRBuilder<>& IRB,
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
    IRBuilder<> IRB(pLoad);
    IGC_ASSERT(nullptr != pLoad->getType());
    unsigned N = pLoad->getType()->isVectorTy()
        ? (unsigned)cast<VectorType>(pLoad->getType())->getNumElements()
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

    IRBuilder<> IRB(pStore);
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
        for (unsigned i = 0, e = (unsigned)cast<VectorType>(pStoreVal->getType())->getNumElements(); i < e; ++i)
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
