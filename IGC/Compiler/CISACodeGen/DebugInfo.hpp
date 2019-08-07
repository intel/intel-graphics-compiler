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

#include "llvm/Config/llvm-config.h"

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/DebugInfo/VISAModule.hpp"
#include "ShaderCodeGen.hpp"
#include "common/allocator.h"
#include "common/debug/Dump.hpp"
#include "common/igc_regkeys.hpp"
#include "common/Stats.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "common/secure_mem.h"
#include "iStdLib/File.h"
#include "Compiler/DebugInfo/VISAIDebugEmitter.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/IGCPassSupport.h"
#include "Types.hpp"
#include "ShaderCodeGen.hpp"
#include "llvm/IR/DIBuilder.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace std;

namespace IGC
{
    class DebugInfoData
    {
    public:
        std::map<llvm::Function*, VISAModule*> m_VISAModules;
        CShader* m_pShader = nullptr;
        IDebugEmitter* m_pDebugEmitter = nullptr;

        void markOutputVars(const llvm::Instruction* pInst);
        void markOutput(llvm::Function& F, CShader* m_currShader);

        void addVISAModule(llvm::Function* F, VISAModule* m)
        {
            auto it = m_VISAModules.find(F);
            if (it != m_VISAModules.end())
                assert(false && "Reinserting VISA module for function");

            m_VISAModules.insert(std::make_pair(F, m));
        }

        static bool hasDebugInfo(CShader* pShader)
        {
            return pShader->GetContext()->m_instrTypes.hasDebugInfo;
        }

    private:
        std::unordered_set<const CVariable*> m_outputVals;
    };

    class DebugInfoPass : public llvm::ModulePass
    {
    public:
        DebugInfoPass(CShaderProgram::KernelShaderMap&);

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

        void EmitDebugInfo(bool);
    };

    class CatchAllLineNumber : public llvm::FunctionPass
    {
    public:
        CatchAllLineNumber();
        virtual ~CatchAllLineNumber();

    private:
        static char ID;

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesAll();
        }
    };
};