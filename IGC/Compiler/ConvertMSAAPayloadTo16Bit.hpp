/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __CONVERT_MSAA_PAYLOAD_TO16BIT__
#define __CONVERT_MSAA_PAYLOAD_TO16BIT__

#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class ConvertMSAAPayloadTo16Bit : public llvm::FunctionPass, public llvm::InstVisitor<ConvertMSAAPayloadTo16Bit>
    {
        llvm::IRBuilder<>* m_builder = nullptr;
        CodeGenContextWrapper* m_pCtxWrapper = nullptr;
    public:

        static char ID;
        ConvertMSAAPayloadTo16Bit();

        ~ConvertMSAAPayloadTo16Bit() {}

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "ConvertMSAAPayloadTo16Bit";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
        }

        void findLdmsUsingInstDownInTree(llvm::Value* inst, std::vector<llvm::GenIntrinsicInst*>& ldmsUsing);

        void visitCallInst(llvm::CallInst& I);
    };
}

#endif
