/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

#include <unordered_set>

namespace IGC
{
    /// @brief  ResolveSampledImageBuiltins pass is used for resolving getter builtins operating on VMEImageINTEL and SampledImage objects.
    ///         SPIR-V Friendly IR represents OpVMEImageINTEL and OpSampledImage opcodes as a functions returning global pointer to opaque type.
    ///         Since it's not convenient to allocate global memory within BiFModule, these builtins are just declared there and resolved in this pass.

    class ResolveSampledImageBuiltins : public llvm::ModulePass, public llvm::InstVisitor<ResolveSampledImageBuiltins>
    {
    public:
        static char ID;

        ResolveSampledImageBuiltins();
        ~ResolveSampledImageBuiltins() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "ResolveSampledImageBuiltins";
        }

        virtual bool runOnModule(llvm::Module& M) override;
        void visitCallInst(llvm::CallInst& CI);

        static const llvm::StringRef GET_IMAGE;
        static const llvm::StringRef GET_SAMPLER;

    private:
        llvm::Value* lowerGetImage(llvm::CallInst& CI);
        llvm::Value* lowerGetSampler(llvm::CallInst& CI);

        bool m_changed;
        std::unordered_set<llvm::CallInst*> m_builtinsToRemove;
    };

} // namespace IGC
