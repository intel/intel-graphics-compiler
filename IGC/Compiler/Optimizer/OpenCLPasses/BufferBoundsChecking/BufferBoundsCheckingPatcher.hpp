/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/Module.h"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class BufferBoundsCheckingPatcher : public llvm::ModulePass, public llvm::InstVisitor<BufferBoundsCheckingPatcher>
    {
    public:
        static char ID;
        static constexpr const char* BUFFER_SIZE_PLACEHOLDER_FUNCTION_NAME = "__bufferboundschecking.bufferSizePlaceholder";

        BufferBoundsCheckingPatcher();
        ~BufferBoundsCheckingPatcher() = default;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& analysisUsage) const override
        {
            analysisUsage.addRequired<MetaDataUtilsWrapper>();
        }

        virtual llvm::StringRef getPassName() const override
        {
            return "BufferBoundsCheckingPatcher";
        }

        virtual bool runOnModule(llvm::Module& M) override;

        void visitCallInst(llvm::CallInst& icmp);

    private:
        ImplicitArgs* implicitArgs = nullptr;
        IGCMD::MetaDataUtils* metadataUtils = nullptr;
        llvm::SmallVector<llvm::CallInst*, 8> toRemove;

        llvm::Argument* getBufferSizeArg(llvm::Function* function, uint32_t n);
    };
}
