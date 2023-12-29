/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

    class DeviceEnqueueFuncsAnalysis : public llvm::ModulePass, public llvm::InstVisitor<DeviceEnqueueFuncsAnalysis>
    {
    public:
        static char ID;

        DeviceEnqueueFuncsAnalysis();

        virtual llvm::StringRef getPassName() const override
        {
            return "DeviceEnqueueFuncsAnalysis";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }


        virtual bool runOnModule(llvm::Module& M) override;


        bool runOnFunction(llvm::Function& F);


        virtual void visitCallInst(llvm::CallInst& CI);


    private:
        bool m_hasDeviceEnqueue;
        llvm::SmallVector< ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS > m_newImplicitArgs;
        ImplicitArg::ArgMap m_newNumberedImplicitArgs;
        IGCMD::MetaDataUtils* m_pMDUtils = nullptr;
    };




    class DeviceEnqueueFuncsResolution : public llvm::FunctionPass, public llvm::InstVisitor<DeviceEnqueueFuncsResolution>
    {
    public:
        static char ID;

        DeviceEnqueueFuncsResolution();

        virtual llvm::StringRef getPassName() const override
        {
            return "DeviceEnqueueFuncsResolution";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void visitCallInst(llvm::CallInst& CI);

    private:
        bool        m_Changed;
        ImplicitArgs m_implicitArgs;
    };

}

