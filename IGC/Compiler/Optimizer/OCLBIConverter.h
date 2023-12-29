/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"


namespace IGC
{
    /// @brief BuiltinsConverter pass used for converting calls from OpenCL function call Built-ins
    ///  to common llvm GenISA intrinsics
    class BuiltinsConverter : public llvm::FunctionPass, public llvm::InstVisitor<BuiltinsConverter>
    {
    public:
        static char ID; // Pass identification, replacement for typeid

        BuiltinsConverter();

        ~BuiltinsConverter() {}

        /// @brief Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "BuiltinsConverterFunction";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        // @brief iterate on all the functions in the module and replace calls from __builtin_IB_* to the match
        //        GenISA intrinsics
        virtual bool runOnFunction(llvm::Function& F) override;
        void visitCallInst(llvm::CallInst& CI);

    protected:
        bool fillIndexMap(llvm::Function& F);
        unsigned int getResourceIndex(llvm::MDNode* argResourceTypes, unsigned int argNo);

        CImagesBI::ParamMap m_argIndexMap;
        CImagesBI::InlineMap m_inlineIndexMap;
        int m_nextSampler{};

        CBuiltinsResolver* m_pResolve = nullptr;
    };

} // namespace IGC

extern "C" llvm::FunctionPass* createBuiltinsConverterPass(void);
