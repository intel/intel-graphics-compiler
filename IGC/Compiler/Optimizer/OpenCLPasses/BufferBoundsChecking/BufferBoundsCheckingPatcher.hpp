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
    class BufferBoundsCheckingPatcher : public llvm::FunctionPass, public llvm::InstVisitor<BufferBoundsCheckingPatcher>
    {
    public:
        static char ID;

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

        virtual bool runOnFunction(llvm::Function& function) override;

        void visitICmpInst(llvm::ICmpInst& icmp);
        void visitSub(llvm::BinaryOperator& sub);

        struct PatchInfo {
            uint32_t operandIndex;
            uint32_t implicitArgBufferSizeIndex;
        };

        static void addPatchInfo(llvm::Instruction* instruction, const PatchInfo& patchInfo);
        static bool hasPatchInfo(llvm::Instruction* instruction);
        static PatchInfo getPatchInfo(llvm::Instruction* instruction);

    private:
        bool modified;
        ImplicitArgs* implicitArgs;
        IGCMD::MetaDataUtils* metadataUtils;

        llvm::Argument* getBufferSizeArg(llvm::Function* function, uint32_t n);
        bool patchInstruction(llvm::Instruction* instruction);
    };
}
