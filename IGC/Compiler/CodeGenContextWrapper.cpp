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

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "CISACodeGen/Platform.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-code-gen-context-wrapper"
#define PASS_DESCRIPTION "CodeGenContext Wrapper"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CodeGenContextWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CodeGenContextWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CodeGenContextWrapper::ID = 0;

CodeGenContextWrapper::CodeGenContextWrapper(CodeGenContext* pCtx) : ImmutablePass(ID), m_ctx(pCtx), m_ctxOwner(false)
{
    initializeCodeGenContextWrapperPass(*PassRegistry::getPassRegistry());
}


CodeGenContextWrapper::CodeGenContextWrapper() : ImmutablePass(ID), m_ctx(nullptr), m_ctxOwner(false)
{
    initializeCodeGenContextWrapperPass(*PassRegistry::getPassRegistry());
    IGC_ASSERT_MESSAGE(0, "CodeGenContextWrapper shouldn't get here in runtime");
}

CodeGenContextWrapper::CodeGenContextWrapper(ShaderType _type, CBTILayout* _bitLayout, CPlatform* _platform, CDriverInfo* driverInfo, bool _owner) : ImmutablePass(ID), m_ctxOwner(_owner)
{
    initializeCodeGenContextWrapperPass(*PassRegistry::getPassRegistry());
    m_ctx = new CodeGenContext(_type, *_bitLayout, *_platform, *driverInfo);
}

CodeGenContextWrapper::~CodeGenContextWrapper()
{
    if (m_ctxOwner)
    {
        delete m_ctx;
    }
}
CodeGenContext* CodeGenContextWrapper::getCodeGenContext()
{
    return m_ctx;
}

