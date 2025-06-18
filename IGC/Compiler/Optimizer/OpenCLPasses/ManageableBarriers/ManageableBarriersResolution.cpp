/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ManageableBarriers/ManageableBarriersResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/NamedBarriers/NamedBarriersResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Optimizer//OpenCLPasses/Atomics/ResolveOCLAtomics.hpp"
#include "Optimizer/OpenCLPasses/WIFuncs/WIFuncResolution.hpp"
#include "Optimizer/OCLBIUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvmWrapper/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "MDFrameWork.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-manage-barriers-resolution"
#define PASS_DESCRIPTION "Resolves manage barriers"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ManageableBarriersResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ManageableBarriersResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ManageableBarriersResolution::ID = 0;

ManageableBarriersResolution::MBFuncType ManageableBarriersResolution::getFuncType(Function* pFunc)
{
    if (pFunc->getName() == "intel_manageable_barrier_init")
        return MBFuncType::InitProdCons;
    else if (pFunc->getName() == "intel_manageable_barrier_release")
        return MBFuncType::Release;
    else if (pFunc->getName() == "intel_manageable_barrier_arrive")
        return MBFuncType::Arrive;
    else if (pFunc->getName() == "intel_manageable_barrier_wait")
        return MBFuncType::Wait;
    else if (pFunc->getName() == "intel_manageable_barrier_arrivewait")
        return MBFuncType::ArriveWait;
    else if (pFunc->getName() == "intel_manageable_barrier_arrivedrop")
        return MBFuncType::ArriveDrop;
   return MBFuncType::None;
}

/// Named Barriers functions START

int ManageableBarriersResolution::getMaxNamedBarrierCount()
{
    return 31;
}

/// Named Barriers functions END

/// Barriers Data Pool functions START

Value* ManageableBarriersResolution::allocBarriersDataPool(Function* pFunc)
{
    Instruction* pFirstInst = &*pFunc->getEntryBlock().getFirstInsertionPt();
    IGCIRBuilder<> builder(pFirstInst);

    Type* manageBarrierDataPoolType =
        ArrayType::get(builder.getInt8Ty(),
            (((int)MBDynamicStructFields::Max * getMaxNamedBarrierCount()) // Data for ManageableBarriers
                + 1) // Data for the IDPool
            * sizeof(int));

    Value* SLMPool = new GlobalVariable(
        *(pFunc->getParent()), manageBarrierDataPoolType, false, GlobalVariable::InternalLinkage, nullptr, "", nullptr, GlobalVariable::NotThreadLocal, ADDRESS_SPACE_LOCAL, false);

    Value* cast =
        BitCastInst::CreateBitOrPointerCast(SLMPool, builder.getInt8PtrTy(ADDRESS_SPACE_LOCAL), "", pFirstInst);

    return cast;
}

Value* ManageableBarriersResolution::preparePointerToBarrierStruct(Value* ptrToBarrierSlot, MBDynamicStructFields FiledType, Instruction* pInsertBefore)
{
    IGCIRBuilder<> builder(dyn_cast<Instruction>(pInsertBefore));

    Value* ptrToBarrierSlotPtr2Int = builder.CreatePtrToInt(ptrToBarrierSlot, builder.getInt32Ty());

    // Pick insterested us data/field from this selected barrier data
    Value* barrierIdx2Field = builder.CreateAdd(ptrToBarrierSlotPtr2Int, builder.getInt32((int)FiledType * sizeof(int)));
    Value* slmPoolInt2Ptr = builder.CreateIntToPtr(barrierIdx2Field, builder.getInt8PtrTy(ADDRESS_SPACE_LOCAL));
    return slmPoolInt2Ptr;
}

Value* ManageableBarriersResolution::getManageableBarrierstructDataPtr(CallInst* pFuncInit, Value* barrierID, Instruction* pInsertBefore)
{
    IGCIRBuilder<> builder(pInsertBefore);
    // Pick ptr to barriers data pool
    Value* slmPoint = getBarriersDataPoolPtr(pInsertBefore);

    Value* slmPoolPtr2Int = builder.CreatePtrToInt(slmPoint, builder.getInt32Ty());

    // We need to move to correct barrier slot
    Value* barrierIdx = builder.CreateMul(barrierID, builder.getInt32((int)MBDynamicStructFields::Max * sizeof(int)));
    Value* slmBarrierSlotPtr2Int = builder.CreateAdd(barrierIdx, slmPoolPtr2Int);
    Value* slmBarrierSlotPtr = builder.CreateIntToPtr(slmBarrierSlotPtr2Int, builder.getInt8PtrTy(ADDRESS_SPACE_LOCAL));

    // Return ptr to the begining of the Barrier Data slot
    return slmBarrierSlotPtr;
}

ArrayType* ManageableBarriersResolution::getMBDynamicStructType()
{
    return ArrayType::get(Type::getInt32Ty(mModule->getContext()), (int)MBDynamicStructFields::Max);
}

Value* ManageableBarriersResolution::loadManageBarrierAllStructData(Value* ptrToBarrierSlot, Instruction* pInsertBefore)
{
    IGCIRBuilder<> builder(pInsertBefore);
    Value* ptr = getManageableBarrierstructDataFieldPtr(ptrToBarrierSlot, MBDynamicStructFields::BarrierID, pInsertBefore);
    Value* ptrCast = builder.CreatePointerCast(ptr, getMBDynamicStructType()->getPointerTo(ADDRESS_SPACE_LOCAL));
    Value* allDataLoad = builder.CreateLoad(getMBDynamicStructType(), ptrCast);
    return allDataLoad;
}

Value* ManageableBarriersResolution::loadManageableBarrierstructData(Value* ptrToBarrierSlot, MBDynamicStructFields DataType, Instruction* pInsertBefore, Value* prefetchedData)
{
    IGCIRBuilder<> builder(pInsertBefore);
    Value* loadedData = nullptr;

    if (prefetchedData == nullptr)
    {
        Value* ptr = getManageableBarrierstructDataFieldPtr(ptrToBarrierSlot, DataType, pInsertBefore);
        loadedData = builder.CreateLoad(builder.getInt32Ty(), ptr);
    }
    else
    {
        loadedData = builder.CreateExtractValue(prefetchedData, { (unsigned int)DataType });
    }
    return loadedData;
}

void ManageableBarriersResolution::storeManageableBarrierstructData(Value* ptrToBarrierSlot, MBDynamicStructFields DataType, Value* pData, Instruction* pInsertBefore)
{
    IGCIRBuilder<> builder(pInsertBefore);
    Value* ptr = getManageableBarrierstructDataFieldPtr(ptrToBarrierSlot, DataType, pInsertBefore);

    builder.CreateStore(pData, ptr);
}

Value* ManageableBarriersResolution::getManageableBarrierstructDataFieldPtr(Value* ptrToBarrierSlot, MBDynamicStructFields DataType, Instruction* pInsertBefore)
{
    IGCIRBuilder<> builder(pInsertBefore);
    Value* ptr8ty = preparePointerToBarrierStruct(ptrToBarrierSlot, DataType, pInsertBefore);
    Value* ptr32ty = builder.CreatePointerCast(ptr8ty, Type::getInt32PtrTy(pInsertBefore->getContext(), ADDRESS_SPACE_LOCAL));

    return ptr32ty;
}

/// Barriers Data Pool functions END
/// Barrier ID Pool functions START

Value* ManageableBarriersResolution::getBarriersDataPoolPtr(Instruction* pCallInst)
{
    return mGlobalDataPoolPerFunc[pCallInst->getFunction()];
}

Value* ManageableBarriersResolution::prepareBarrierIDPoolPtr(Instruction* pInsertBefore)
{
    IGCIRBuilder<> builder(pInsertBefore);

    Value* pBarriersDataPool = mGlobalDataPoolPerFunc[pInsertBefore->getFunction()];
    Value* ptr2int = builder.CreatePtrToInt(pBarriersDataPool, builder.getInt32Ty());

    Value* offset2IDPool = builder.CreateAdd(ptr2int, builder.getInt32(getMaxNamedBarrierCount() * (int)MBDynamicStructFields::Max * sizeof(int)));
    Value* pIDPool = builder.CreateIntToPtr(offset2IDPool, PointerType::getInt32PtrTy(pInsertBefore->getContext(), ADDRESS_SPACE_LOCAL));

    if (hasSimpleBarrier())
    {
        IGC_ASSERT_MESSAGE(mManageBarrierInstructionsInit.size() < 31, "There is no free ID for the barrier");
        // As the simple barriers took the first IDs
        // then start from the first available.
        builder.CreateStore(builder.getInt32(~0u << (mManageBarrierInstructionsInit.size() + 2)), pIDPool);
    }
    else
    {
        // setup -2 as we want to have in bits:
        // 1111 1111 1111 1110
        // where for named barrier the first barrier id is reserved for regular workgroup barrier
        builder.CreateStore(builder.getInt32(-2), pIDPool);
    }

    return pIDPool;
}

// IDPool
// is 32bit var showing which of the ID barriers are free/busy
// 0 - on the bit means busy
// 1 - on the bit means free
// __ctz(IDPool)+1 return first free ID to pick
// example:
// Binary form of IDPool 00000000 00000000 00000000 00010000
// first free ID is 5

void ManageableBarriersResolution::markID(Value* IDPool, Value* IDBarrier, Instruction* pInsertBefore)
{
    IGCIRBuilder<> builder(pInsertBefore);

    Value* maskID = builder.CreateShl(builder.getInt32(1), IDBarrier);
    Value* notBit_maskID = builder.CreateXor(maskID, builder.getInt32(-1));

    Value* currentIDPoolState = builder.CreateLoad(builder.getInt32Ty(), IDPool);
    Value* currentIDPoolStateUpdated = builder.CreateAnd(notBit_maskID, currentIDPoolState);

    builder.CreateStore(currentIDPoolStateUpdated, IDPool);
}

void ManageableBarriersResolution::releaseID(Value* IDPool, Value* IDBarrier, Instruction* pInsertBefore)
{
    IGCIRBuilder<> builder(pInsertBefore);
    Value* maskID = builder.CreateShl(builder.getInt32(1), IDBarrier);

    ResolveOCLAtomics::CallAtomicSingleLane(AtomicOp::EATOMIC_OR, IDPool, maskID, pInsertBefore);
}

Value* ManageableBarriersResolution::getFreeID(Value* IDPool, Instruction* pInsertBefore)
{
    IGCIRBuilder<> builder(pInsertBefore);
    Value* currentIDPoolState = builder.CreateLoad(builder.getInt32Ty(), IDPool);

    Function* func_llvm_GenISA_firstbitLo = GenISAIntrinsic::getDeclaration(mModule, GenISAIntrinsic::GenISA_firstbitLo);
    Value* freeIDNumber = builder.CreateCall(func_llvm_GenISA_firstbitLo, { currentIDPoolState });

    return freeIDNumber; // call __ctz(IDPool) + 1 in IR
}

Value* ManageableBarriersResolution::getBarrierIDPoolPtr(Instruction* pCallInst)
{
    return mGlobalIDPoolPPerFunc[pCallInst->getFunction()];
}

/// Barrier ID Pool functions END
/// ManageBarrier emitters START

void ManageableBarriersResolution::emitInit(CallInst* pInsertPoint)
{
    IGCIRBuilder<> builder(pInsertPoint);

    if (isSimpleBarrier(pInsertPoint))
    {
        // The barrier will not use the SLM memory
        // just feed the gateway sends with constant values
        auto& dataBarrier = mManageBarrierInstructionsInitSimple[pInsertPoint];
        dataBarrier.ID = getFreeID();
        dataBarrier.ProducerCount =
            builder.getInt32(
                dyn_cast<ConstantInt>(
                    pInsertPoint->getArgOperand(0))->getZExtValue());
        dataBarrier.ConsumerCount =
            builder.getInt32(
                dyn_cast<ConstantInt>(
                    pInsertPoint->arg_size() > 1 ?
                    pInsertPoint->getArgOperand(1) : pInsertPoint->getArgOperand(0))->getZExtValue());
    }
    else
    {
        // Prepare basic blocks
        BasicBlock* bbBefore = pInsertPoint->getParent();
        BasicBlock* bbAfter = bbBefore->splitBasicBlock(pInsertPoint);
        BasicBlock* bbInitSection = BasicBlock::Create(
            builder.getContext(), "", pInsertPoint->getFunction(), bbAfter);

        // Prepare branch instructions
        BranchInst* instrJumpbbBefore2bbAfter =
            dyn_cast<BranchInst>(bbBefore->getTerminator());

        CallInst* getLocalID = WIFuncResolution::CallGetLocalID(instrJumpbbBefore2bbAfter);
        ICmpInst* checkLocalThreadID = new ICmpInst(
            instrJumpbbBefore2bbAfter,
            ICmpInst::ICMP_EQ,
            getLocalID,
            builder.getInt32(0));
        // Setup new branch conditional instruction
        BranchInst* chekForSingleLane = BranchInst::Create(bbInitSection, bbAfter, checkLocalThreadID, bbBefore);
        // Remove old branch non-conditional instruction
        instrJumpbbBefore2bbAfter->eraseFromParent();

        // Get current free ID for named barrier
        Value* barrierIDPoolPtr = getBarrierIDPoolPtr(pInsertPoint);

        Value* getFirstFreeID = getFreeID(barrierIDPoolPtr, chekForSingleLane);
        Value* ptrToBarrierSlot = getManageableBarrierstructDataPtr(pInsertPoint, getFirstFreeID, chekForSingleLane);

        // Fill the basic block section for the Init function of barriers
        Instruction* instrJump = BranchInst::Create(bbAfter, bbInitSection);

        markID(barrierIDPoolPtr, getFirstFreeID, instrJump);

        storeManageableBarrierstructData(ptrToBarrierSlot,
            MBDynamicStructFields::BarrierID,
            getFirstFreeID,
            instrJump
        );

        storeManageableBarrierstructData(ptrToBarrierSlot,
            MBDynamicStructFields::ProducerCount,
            pInsertPoint->getArgOperand(0),
            instrJump
        );

        storeManageableBarrierstructData(ptrToBarrierSlot,
            MBDynamicStructFields::ConsumerCount,
            pInsertPoint->arg_size() > 1 ?
            pInsertPoint->getArgOperand(1) : pInsertPoint->getArgOperand(0),
            instrJump
        );

        storeManageableBarrierstructData(ptrToBarrierSlot,
            MBDynamicStructFields::ExpectedArrvial,
            pInsertPoint->arg_size() > 1 ?
            pInsertPoint->getArgOperand(1) : pInsertPoint->getArgOperand(0),
            instrJump
        );

        CallMemoryFenceWorkgroup(pInsertPoint);

        // Add workgroup barrier
        GenIntrinsicInst::Create(
            GenISAIntrinsic::getDeclaration(mModule, GenISAIntrinsic::GenISA_threadgroupbarrier),
            {},
            "",
            pInsertPoint);

        pInsertPoint->replaceAllUsesWith(ptrToBarrierSlot);
    }
}

void ManageableBarriersResolution::emitRelease(CallInst* pInsertPoint)
{
    if (!usesSimpleBarrier(pInsertPoint))
    {
        Value* pBarrierData = pInsertPoint->getOperand(0);
        Value* barrierIDPoolPtr = getBarrierIDPoolPtr(pInsertPoint);
        Value* barrierID = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::BarrierID, pInsertPoint);

        releaseID(barrierIDPoolPtr, barrierID, pInsertPoint);
    }
}

void ManageableBarriersResolution::emitArrive(CallInst* pInsertPoint)
{
    Value* bID = nullptr;
    Value* bProdCnt = nullptr;
    Value* bConsCnt = nullptr;

    if (usesSimpleBarrier(pInsertPoint))
    {
        auto barrierData = getSimpleBarrierDataFromCall(pInsertPoint);

        bID = barrierData.ID;
        bProdCnt = barrierData.ProducerCount;
        bConsCnt = barrierData.ConsumerCount;
    }
    else
    {
        Value* pBarrierData = pInsertPoint->getOperand(0);
        Value* allData = loadManageBarrierAllStructData(pBarrierData, pInsertPoint);

        bID = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::BarrierID, pInsertPoint, allData);
        bProdCnt = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ProducerCount, pInsertPoint, allData);
        bConsCnt = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ConsumerCount, pInsertPoint, allData);
    }

    NamedBarriersResolution::CallSignal(bID, bProdCnt, bConsCnt, NamedBarriersResolution::NamedBarrierType::Producer, pInsertPoint);
}

void ManageableBarriersResolution::emitWait(CallInst* pInsertPoint)
{
    Value* bID = nullptr;
    Value* bProdCnt = nullptr;
    Value* bConsCnt = nullptr;
    Value* allData = nullptr;
    bool isSimpleBarrier = usesSimpleBarrier(pInsertPoint);

    if (isSimpleBarrier)
    {
        auto barrierData = getSimpleBarrierDataFromCall(pInsertPoint);

        bID = barrierData.ID;
        bProdCnt = barrierData.ProducerCount;
        bConsCnt = barrierData.ConsumerCount;
    }
    else
    {
        Value* pBarrierData = pInsertPoint->getOperand(0);
        allData = loadManageBarrierAllStructData(pBarrierData, pInsertPoint);

        bID = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::BarrierID, pInsertPoint, allData);
        bProdCnt = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ProducerCount, pInsertPoint, allData);
        bConsCnt = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ConsumerCount, pInsertPoint, allData);
    }

    NamedBarriersResolution::CallSignal(bID, bProdCnt, bConsCnt, NamedBarriersResolution::NamedBarrierType::Consumer, pInsertPoint);
    NamedBarriersResolution::CallWait(bID, pInsertPoint);

    if (!isSimpleBarrier)
    {
        Value* pBarrierData = pInsertPoint->getOperand(0);
        // Update the Producer count after any calls of ArriveDrop
        Value* getExpectedArrvial = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ExpectedArrvial, pInsertPoint, allData);
        storeManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ProducerCount, getExpectedArrvial, pInsertPoint);
    }
}

void ManageableBarriersResolution::emitArriveWait(CallInst* pInsertPoint)
{
    Value* bID = nullptr;
    Value* bProdCnt = nullptr;
    Value* bConsCnt = nullptr;
    Value* allData = nullptr;
    bool isSimpleBarrier = usesSimpleBarrier(pInsertPoint);

    if (isSimpleBarrier)
    {
        auto barrierData = getSimpleBarrierDataFromCall(pInsertPoint);

        bID = barrierData.ID;
        bProdCnt = barrierData.ProducerCount;
        bConsCnt = barrierData.ConsumerCount;
    }
    else
    {
        Value* pBarrierData = pInsertPoint->getOperand(0);
        allData = loadManageBarrierAllStructData(pBarrierData, pInsertPoint);

        bID = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::BarrierID, pInsertPoint, allData);
        bProdCnt = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ProducerCount, pInsertPoint, allData);
        bConsCnt = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ConsumerCount, pInsertPoint, allData);
    }

    NamedBarriersResolution::CallSignal(bID, bProdCnt, bConsCnt, NamedBarriersResolution::NamedBarrierType::ProducerConsumer, pInsertPoint);
    NamedBarriersResolution::CallWait(bID, pInsertPoint);

    if (!isSimpleBarrier)
    {
        Value* pBarrierData = pInsertPoint->getOperand(0);
        // Update the Producer count after any calls of ArriveDrop
        Value* getExpectedArrvial = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ExpectedArrvial, pInsertPoint, allData);
        storeManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ProducerCount, getExpectedArrvial, pInsertPoint);
    }
}

void ManageableBarriersResolution::emitArriveDrop(CallInst* pInsertPoint)
{
    Value* pBarrierData = pInsertPoint->getOperand(0);
    Value* allData = loadManageBarrierAllStructData(pBarrierData, pInsertPoint);

    Value* getIDInt32 = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::BarrierID, pInsertPoint, allData);
    Value* getProducerCntIn32 = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ProducerCount, pInsertPoint, allData);
    Value* getConsumerCntInt32 = loadManageableBarrierstructData(pBarrierData, MBDynamicStructFields::ConsumerCount, pInsertPoint, allData);

    Value* ptrExpectedArrvial = getManageableBarrierstructDataFieldPtr(pBarrierData, MBDynamicStructFields::ExpectedArrvial, pInsertPoint);
    ResolveOCLAtomics::CallAtomicSingleLane(AtomicOp::EATOMIC_DEC, ptrExpectedArrvial, nullptr, pInsertPoint);

    NamedBarriersResolution::CallSignal(getIDInt32, getProducerCntIn32, getConsumerCntInt32, NamedBarriersResolution::NamedBarrierType::Producer, pInsertPoint);
}

void ManageableBarriersResolution::emit(CallInst* CI, MBFuncType FuncType)
{
    switch (FuncType)
    {
    case MBFuncType::InitProd:
    case MBFuncType::InitProdCons:
        // this was already done
        break;
    case MBFuncType::Release:
        emitRelease(CI);
        break;
    case MBFuncType::Arrive:
        emitArrive(CI);
        break;
    case MBFuncType::Wait:
        emitWait(CI);
        break;
    case MBFuncType::ArriveWait:
        emitArriveWait(CI);
        break;
    case MBFuncType::ArriveDrop:
        emitArriveDrop(CI);
        break;
    case MBFuncType::None:
        break;
    }
}

/// ManageBarrier emitters END

ManageableBarriersResolution::ManageableBarriersResolution() : ModulePass(ID)
{
    initializeManageableBarriersResolutionPass(*PassRegistry::getPassRegistry());
}

ManageableBarriersResolution::~ManageableBarriersResolution(void)
{ }

bool ManageableBarriersResolution::runOnModule(Module& M)
{
    mModule = &M;
    bool isManageableBarriersAdded = false;
    mCurrentMode = static_cast<MBMode>(IGC_GET_FLAG_VALUE(ManageableBarriersMode));
    for (auto& F : M)
    {
        if (F.getCallingConv() == CallingConv::SPIR_KERNEL && !F.isDeclaration())
        {
            visit(F);

            if (mManageBarrierInstructionsInit.size() > 0)
            {
                mSimpleBarrierIDCount = 1;
                isManageableBarriersAdded = true;

                if (mCurrentMode == MBMode::Mix ||
                    mCurrentMode == MBMode::SimpleOnly)
                {
                    lookForSimpleBarriers();
                }

                if (!allBarrierSimple())
                {
                    auto* slmSpot = allocBarriersDataPool(&F);
                    // Prepare memory space in SLM for barriers data
                    mGlobalDataPoolPerFunc.insert({ &F, slmSpot });
                    // Prepare a ptr to the IDPool
                    mGlobalIDPoolPPerFunc.insert({ &F, prepareBarrierIDPoolPtr(
                        dyn_cast<Instruction>(slmSpot)->getNextNode()) });
                }

                // In first place, take care of the init functions
                for (auto& pair : mManageBarrierInstructionsInit)
                {
                    emitInit(pair.first);
                }
                // Now, we can take the rest of the API calls
                for (auto& pair : mManageBarrierInstructions)
                {
                    emit(pair.first, pair.second);
                }

                clearData();
            }
        }
    }

    if (isManageableBarriersAdded)
    {
        // Add attribute NBarrierCnt to metadata
        auto MD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        // Adding max as it could be dynamic
        MD->NBarrierCnt = getMaxNamedBarrierCount();
        return true;
    }
    return false;
}

void ManageableBarriersResolution::visitCallInst(CallInst& CI)
{
    if (CI.getCalledFunction() == nullptr ||
        !CI.getCalledFunction()->hasName())
    {
        return;
    }

    MBFuncType Type = getFuncType(CI.getCalledFunction());
    if (Type == MBFuncType::InitProdCons ||
        Type == MBFuncType::InitProd)
    {
        mManageBarrierInstructionsInit.insert({ &CI, Type });
    }
    else if (Type != MBFuncType::None)
    {
        mManageBarrierInstructions.insert({ &CI, Type });
    }
}

bool ManageableBarriersResolution::HasHWSupport(GFXCORE_FAMILY GFX_CORE)
{
    return NamedBarriersResolution::NamedBarrierHWSupport(GFX_CORE);
}

bool ManageableBarriersResolution::isConstantInit(CallInst* pFunc)
{
    if (pFunc->arg_size() > 1)
    {
        return
            isa<Constant>(pFunc->getArgOperand(0)) &&
            isa<Constant>(pFunc->getArgOperand(1));
    }
    else
    {
        return
            isa<Constant>(pFunc->getArgOperand(0));
    }
}

bool ManageableBarriersResolution::isCallOnceInit(CallInst* pFunc)
{
    llvm::BasicBlock* BB = pFunc->getParent();
    while (BB)
    {
        if (BB->isEntryBlock())
            return true;
        auto *BI = llvm::dyn_cast<BranchInst>(BB->getTerminator());
        if (!BI || BI->isConditional())
            return false;
        BB = BB->getSinglePredecessor();
    }
    return false;
}

bool ManageableBarriersResolution::hasSimpleCallsOnly(CallInst* pFunc)
{
    for (auto user : pFunc->users())
    {
        CallInst* callAPI = dyn_cast<CallInst>(user);
        if (!callAPI)
            return false;

        Function* funcSign = callAPI->getCalledFunction();
        if ((getFuncType(funcSign) == MBFuncType::ArriveDrop) ||
            (getFuncType(funcSign) ==  MBFuncType::None))
            return false;
    }
    return true;
}

void ManageableBarriersResolution::lookForSimpleBarriers()
{
    for (auto& init : mManageBarrierInstructionsInit)
    {
        CallInst* pFunc = init.first;
        if (mCurrentMode == MBMode::SimpleOnly ||
            (isConstantInit(pFunc) && isCallOnceInit(pFunc) && hasSimpleCallsOnly(pFunc)))
        {
            // It's simple barrier, we can use constant values
            mManageBarrierInstructionsInitSimple.insert({ pFunc, {} });
        }
    }
}

bool ManageableBarriersResolution::hasSimpleBarrier()
{
    return mManageBarrierInstructionsInitSimple.size() > 0;
}

bool ManageableBarriersResolution::allBarrierSimple()
{
    return mManageBarrierInstructionsInit.size() == mManageBarrierInstructionsInitSimple.size();
}

bool ManageableBarriersResolution::isSimpleBarrier(CallInst* pFunc)
{
    return mManageBarrierInstructionsInitSimple.count(pFunc) != 0;
}

bool ManageableBarriersResolution::usesSimpleBarrier(CallInst* pFunc)
{
    return isSimpleBarrier(dyn_cast<CallInst>(pFunc->getOperand(0)));
}

Value* ManageableBarriersResolution::getFreeID()
{
    IRBuilder<> builder(mModule->getContext());

    int currentID = mSimpleBarrierIDCount;
    mSimpleBarrierIDCount++;

    return builder.getInt32(currentID);
}

ManageableBarriersResolution::SimpleBarrierStruct& ManageableBarriersResolution::getSimpleBarrierDataFromCall(CallInst* pFunc)
{
    return getSimpleBarrierData(dyn_cast<CallInst>(pFunc->getOperand(0)));
}

ManageableBarriersResolution::SimpleBarrierStruct& ManageableBarriersResolution::getSimpleBarrierData(CallInst* pFunc)
{
    return mManageBarrierInstructionsInitSimple[pFunc];
}

void ManageableBarriersResolution::clearData()
{
    for (auto& call : mManageBarrierInstructions)
    {
        call.first->eraseFromParent();
    }

    for (auto& init : mManageBarrierInstructionsInit)
    {
        init.first->eraseFromParent();
    }

    mManageBarrierInstructionsInit.clear();
    mManageBarrierInstructions.clear();
    mManageBarrierInstructionsInitSimple.clear();
}