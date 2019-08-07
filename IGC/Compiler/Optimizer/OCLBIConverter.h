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
        int m_nextSampler;

        CBuiltinsResolver* m_pResolve;
    };

} // namespace IGC

extern "C" llvm::FunctionPass* createBuiltinsConverterPass(void);