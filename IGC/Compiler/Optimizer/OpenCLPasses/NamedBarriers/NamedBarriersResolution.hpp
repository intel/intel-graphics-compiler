/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

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
    /// @brief  NamedBarriersResolution pass used for resolving OpenCL named barriers functions.
    class NamedBarriersResolution : public llvm::ModulePass, public llvm::InstVisitor<NamedBarriersResolution, void>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        NamedBarriersResolution();
        NamedBarriersResolution(GFXCORE_FAMILY GFX_CORE);

        /// @brief  Destructor
        ~NamedBarriersResolution();

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "NamedBarriersResolution";
        }

        virtual bool runOnModule(llvm::Module& M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override { AU.addRequired<MetaDataUtilsWrapper>(); }

        /// @brief  Call instructions visitor.
        ///         Checks for OpenCL Named Barier init  functions and resolves them into appropriate sequence of code
        /// @param  CI The call instruction.
        void visitCallInst(llvm::CallInst& CI);

        void initGlobalVariables(llvm::Module*, llvm::Type*);

        static const char* NAMED_BARRIERS_INIT;
        static const char* NAMED_BARRIERS_BARRIER_ARG2;
        static const char* NAMED_BARRIERS_BARRIER_ARG3;
        const int GetMaxNamedBarriers();

        static int AlignNBCnt2BarrierNumber(uint NBCnt);
        static bool NamedBarrierHWSupport(GFXCORE_FAMILY GFX_CORE);

        enum NamedBarrierType
        {
            ProducerConsumer = 0,
            Producer = 1,
            Consumer = 2
        };

        static void CallSignal(llvm::Value* barrierID, llvm::Value* ProducerCnt, llvm::Value* ConsumerCnt, NamedBarrierType Type, llvm::Instruction* pInsertBefore);
        static void CallWait(llvm::Value* barrierID, llvm::Instruction* pInsertBefore);
    private:
        GFXCORE_FAMILY m_GFX_CORE;
        llvm::Type* m_NamedBarrierType;
        llvm::GlobalVariable* m_NamedBarrierID;
        llvm::GlobalVariable* m_NamedBarrierArray;
        /// @brief  Indicates if the pass changed the processed function
        inline bool IsNamedBarriersAdded()
        {
            return m_CountNamedBarriers > 0;
        }

        int m_CountNamedBarriers;

        bool isNamedBarrierInit(llvm::StringRef& FuncionName);
        bool isNamedBarrierSync(llvm::StringRef& FuncionName);

        void HandleNamedBarrierInitSW(llvm::CallInst& NBarrierInitCall);
        void HandleNamedBarrierSyncSW(llvm::CallInst& NBarrierSyncCall);

        struct s_namedBarrierInfo
        {
            llvm::Value* threadGroupNBarrierID;
            llvm::Value* threadGroupNBarrierCount;
            llvm::CallInst* threadGroupNBarrierInit;
        };
        llvm::Value* FindAllocStructNBarrier(llvm::Value* Val, bool IsNBarrierInitCall);

        llvm::DenseMap<llvm::Value*, s_namedBarrierInfo> m_MapInitToID;

        void HandleNamedBarrierInitHW(llvm::CallInst& NBarrierInitCall);
        void HandleNamedBarrierSyncHW(llvm::CallInst& NBarrierSyncCall);
    };

} // namespace IGC
