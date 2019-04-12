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

namespace IGC
{
    // A pass that walks over call instructions and replaces all __builtin_IB_work_group
    // with corresponding GenISA intrinsics.
    //
    class WGFuncResolution : public llvm::ModulePass, public llvm::InstVisitor<WGFuncResolution>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        WGFuncResolution();

        /// @brief  Destructor
        ~WGFuncResolution() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "WGFuncResolution";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
            AU.setPreservesCFG();
        }

        // Entry point of the pass.
        virtual bool runOnModule(llvm::Module &M) override;

        // Call instructions visitor.
        void visitCallInst(llvm::CallInst &callInst);

    private:
        llvm::Module *m_pModule;

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed;
    };

} // namespace IGC