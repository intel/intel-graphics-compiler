/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include <unordered_map>

namespace IGC
{
    class PromoteStatelessToBindless : public llvm::FunctionPass, public llvm::InstVisitor<PromoteStatelessToBindless>
    {
    public:
        static char ID;

        PromoteStatelessToBindless();

        virtual llvm::StringRef getPassName() const override
        {
            return "PromoteStatelessToBindless";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.setPreservesCFG();
        }

        bool runOnFunction(llvm::Function& F) override;
        void visitInstruction(llvm::Instruction& I);

    private:
        void GetAccessInstToSrcPointerMap(llvm::Instruction* inst, llvm::Value* resourcePtr);
        void PromoteStatelessToBindlessBuffers(llvm::Function& F) const;

        std::unordered_map<llvm::Value*, llvm::Value*> m_AccessToSrcPtrMap;
    };

} // namespace IGC
