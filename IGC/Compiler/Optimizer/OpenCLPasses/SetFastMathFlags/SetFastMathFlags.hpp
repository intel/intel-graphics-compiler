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

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This pass sets llvm fast math flags to relevant instructions, according
    ///         to the present compiler options.
    ///         -no-signed-zeros and -unsafe-math-optimizations sets nsz flag
    ///         -finite-math-only sets nnan and ninf flags
    ///         -fast-relaxed-math sets fast flag which implies all others (including arcp)
    class SetFastMathFlags : public llvm::ModulePass
    {
    public:
        /// @brief  Pass identification.
        static char ID;

        SetFastMathFlags();

        ~SetFastMathFlags() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "SetFastMathFlags";
        }

        virtual bool runOnModule(llvm::Module& M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

    private:
        /// @brief  sets the given flags to all instruction supporting fast math flags in the given module.
        /// @param  M - the module
        /// @param  fmfs - the fast math flags
        /// @return true if made any changes to the module.
        static bool setFlags(llvm::Module& M, llvm::FastMathFlags fmfs);
    };

} // namespace IGC
