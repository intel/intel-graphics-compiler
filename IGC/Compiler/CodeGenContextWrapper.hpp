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

#include "common/Types.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class CodeGenContext;
    class CBTILayout;
    class CPlatform;
    class CDriverInfo;
    // This pass provides access to  CodeGenContext.
    //
    // To use this from within another pass:
    //  1. Add CodeGenContextWrapper to the pass manager.
    //  2. Use getAnalysisIfAvailable on CodeGenContextWrapper:
    //       CodeGenContextWrapper* pCtxWrapper = getAnalysis<CodeGenContextWrapper>();
    //  3. Get the CodeGenContext:
    //      CodeGenContext* ctx = pCtxWrapper->getCodeGenContext();

    class CodeGenContextWrapper : public llvm::ImmutablePass
    {
    public:
        static char ID;

        // Constructs a wrapper to the given CodeGenContext instance.
        CodeGenContextWrapper(CodeGenContext* pCtx);
        CodeGenContextWrapper();
        // param ShaderType _type
        // param CBTILayout* _bitLayout
        // param CPlatform* _platform
        //             all of the above params are needed in order to create new CodeGenContext.
        // param bool _owner - true if the pass is the owner of the context and responsible to delete it.
        CodeGenContextWrapper(ShaderType _type, CBTILayout* _bitLayout, CPlatform* _platform, CDriverInfo* driverInfo, bool _owner);
        ~CodeGenContextWrapper();

        // return the Context
        CodeGenContext* getCodeGenContext();

        virtual llvm::StringRef getPassName() const override
        {
            return "CodeGen Context Wrapper";
        }

    private:
        CodeGenContext* m_ctx;
        bool m_ctxOwner;

    };

} // namespace IGC
