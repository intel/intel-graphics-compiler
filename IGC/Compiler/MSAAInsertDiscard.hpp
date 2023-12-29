/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __MSAA_INSERT_DISCARD__
#define __MSAA_INSERT_DISCARD__

#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class MSAAInsertDiscard : public llvm::FunctionPass, public llvm::InstVisitor<MSAAInsertDiscard>
    {
    private:
        bool done{};
        llvm::IRBuilder<>* m_builder = nullptr;
        CodeGenContextWrapper* m_pCtxWrapper = nullptr;
        unsigned int m_kernelSize{};

        int getiCMPValue();

    public:

        static char ID;
        MSAAInsertDiscard();

        ~MSAAInsertDiscard() {}

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "MSAAInsertDiscard";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
        }

        void visitCallInst(llvm::CallInst& I);
    };
}

#endif
