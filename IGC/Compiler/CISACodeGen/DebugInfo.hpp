/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"
#include "DebugInfo/VISAModule.hpp"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "llvm/IR/DIBuilder.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace std;

namespace IGC
{
    class DbgDecoder;
    class CVariable;

    class DebugInfoPass : public llvm::ModulePass
    {
    public:
        DebugInfoPass(CShaderProgram::KernelShaderMap&);
        virtual llvm::StringRef getPassName() const  override { return "DebugInfoPass"; }
        virtual ~DebugInfoPass();

    private:
        static char ID;
        CShaderProgram::KernelShaderMap& kernels;
        CShader* m_currShader = nullptr;
        IDebugEmitter* m_pDebugEmitter = nullptr;

        virtual bool runOnModule(llvm::Module& M) override;
        virtual bool doInitialization(llvm::Module& M) override;
        virtual bool doFinalization(llvm::Module& M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.setPreservesAll();
        }

        void EmitDebugInfo(bool, DbgDecoder*, const std::vector<llvm::DISubprogram*>&);
    };

    class CatchAllLineNumber : public llvm::FunctionPass
    {
    public:
        CatchAllLineNumber();
        virtual ~CatchAllLineNumber();
        static char ID;

    private:

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesAll();
        }
    };
};
