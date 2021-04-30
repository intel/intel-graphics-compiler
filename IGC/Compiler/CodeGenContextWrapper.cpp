/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

CodeGenContextWrapper::CodeGenContextWrapper(ShaderType _type, const CBTILayout* _bitLayout,
    const CPlatform* _platform, const CDriverInfo* driverInfo, bool _owner) : ImmutablePass(ID), m_ctxOwner(_owner)
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

