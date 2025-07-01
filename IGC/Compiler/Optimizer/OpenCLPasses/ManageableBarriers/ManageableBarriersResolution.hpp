/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include <llvm/IR/DataLayout.h>
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "IGC/common/Types.hpp"

namespace IGC
{
    /// @brief  ManageableBarriersResolution pass used for resolving OpenCL named barriers functions.
    class ManageableBarriersResolution : public llvm::ModulePass, public llvm::InstVisitor<ManageableBarriersResolution, void>
    {
    private:

        enum class MBFuncType
        {
            InitProdCons,
            InitProd,
            Release,
            Arrive,
            Wait,
            ArriveWait,
            ArriveDrop,

            None
        };

        enum class MBMode
        {
            Mix = 0,
            DynamicOnly = 1,
            SimpleOnly = 2
        };

        enum class MBDynamicStructFields
        {
            BarrierID = 0,
            ProducerCount,
            ConsumerCount,
            ExpectedArrvial,

            Max
        };

        llvm::ArrayType* getMBDynamicStructType();

        struct DynamicBarrierStruct
        {
            llvm::Value* ptrID;
            llvm::Value* ptrProducerCount;
            llvm::Value* ptrConsumerCount;
            llvm::Value* ptrExpectedArrival;
        };

        struct SimpleBarrierStruct
        {
            llvm::Value* ID;
            llvm::Value* ProducerCount;
            llvm::Value* ConsumerCount;
        };

        llvm::Module* mModule = nullptr;
        MBMode mCurrentMode = {};

        llvm::DenseMap<llvm::CallInst*, MBFuncType> mManageBarrierInstructions;
        llvm::DenseMap<llvm::CallInst*, MBFuncType> mManageBarrierInstructionsInit;
        llvm::DenseMap<llvm::CallInst*, SimpleBarrierStruct> mManageBarrierInstructionsInitSimple;
        int mSimpleBarrierIDCount = 0;

        llvm::DenseMap<llvm::Function*, llvm::Value*> mGlobalDataPoolPerFunc;
        llvm::DenseMap<llvm::Function*, llvm::Value*> mGlobalIDPoolPPerFunc;


        MBFuncType getFuncType(llvm::Function* pFunc);


        void emit(llvm::CallInst* CI, MBFuncType FuncType);
        void emitInit(llvm::CallInst* pInsertPoint);
        void emitRelease(llvm::CallInst* pInsertPoint);
        void emitArrive(llvm::CallInst* pInsertPoint);
        void emitWait(llvm::CallInst* pInsertPoint);
        void emitArriveWait(llvm::CallInst* pInsertPoint);
        void emitArriveDrop(llvm::CallInst* pInsertPoint);

        int getMaxNamedBarrierCount();
        llvm::Value* allocBarriersDataPool(llvm::Function* pFunc);
        llvm::Value* getManageableBarrierstructDataPtr(llvm::CallInst* pFuncInit, llvm::Value* barrierID, llvm::Instruction* pInsertBefore);
        llvm::Value* preparePointerToBarrierStruct(llvm::Value* ptrToBarrierSlot, MBDynamicStructFields FiledType, llvm::Instruction* pInsertBefore);
        llvm::Value* getManageableBarrierstructDataFieldPtr(llvm::Value* ptrToBarrierSlot, MBDynamicStructFields DataType, llvm::Instruction* pInsertBefore);

        void storeManageableBarrierstructData(llvm::Value* ptrToBarrierSlot, MBDynamicStructFields DataType, llvm::Value* pData, llvm::Instruction* pInsertBefore);
        llvm::Value* loadManageableBarrierstructData(llvm::Value* ptrToBarrierSlot, MBDynamicStructFields DataType, llvm::Instruction* pInsertBefore, llvm::Value* prefetchedData = nullptr);
        llvm::Value* loadManageBarrierAllStructData(llvm::Value* ptrToBarrierSlot, llvm::Instruction* pInsertBefore);


        llvm::Value* prepareBarrierIDPoolPtr(llvm::Instruction* pInsertBefore);
        llvm::Value* getBarriersDataPoolPtr(llvm::Instruction* pCallInst);
        llvm::Value* getBarrierIDPoolPtr(llvm::Instruction* pCallInst);
        void markID(llvm::Value* IDPool, llvm::Value* IDBarrier, llvm::Instruction* pInsertBefore);
        void releaseID(llvm::Value* IDPool, llvm::Value* IDBarrier, llvm::Instruction* pInsertBefore);
        llvm::Value* getFreeID(llvm::Value* IDPool, llvm::Instruction* pInsertBefore);

        void clearData();

        bool isConstantInit(llvm::CallInst* pFunc);
        bool isCallOnceInit(llvm::CallInst* pFunc);
        bool hasSimpleCallsOnly(llvm::CallInst* pFunc);

        bool isSimpleBarrier(llvm::CallInst* pFunc);
        bool usesSimpleBarrier(llvm::CallInst* pFunc);
        SimpleBarrierStruct& getSimpleBarrierData(llvm::CallInst* pFunc);
        SimpleBarrierStruct& getSimpleBarrierDataFromCall(llvm::CallInst* pFunc);

        void lookForSimpleBarriers();

        bool allBarrierSimple();
        bool hasSimpleBarrier();

        llvm::Value* getFreeID();

    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ManageableBarriersResolution();

        /// @brief  Destructor
        ~ManageableBarriersResolution();

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ManageableBarriersResolution";
        }

        virtual bool runOnModule(llvm::Module& M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override { AU.addRequired<MetaDataUtilsWrapper>(); }

        /// @brief  Call instructions visitor.
        ///         Checks for OpenCL Named Barier init  functions and resolves them into appropriate sequence of code
        /// @param  CI The call instruction.
        void visitCallInst(llvm::CallInst& CI);

        static bool HasHWSupport(GFXCORE_FAMILY GFX_CORE);
    };

} // namespace IGC
