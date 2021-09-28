/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-metadata-utils-wrapper"
#define PASS_DESCRIPTION "Metadata Utils Wrapper"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS(MetaDataUtilsWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char MetaDataUtilsWrapper::ID = 0;

MetaDataUtilsWrapper::MetaDataUtilsWrapper(MetaDataUtils* pMdUtils, ModuleMetaData* moduleMD) : ImmutablePass(ID), m_pMdUtils(pMdUtils), modMD(moduleMD), m_isUtilsOwner(false)
{
    initializeMetaDataUtilsWrapperPass(*PassRegistry::getPassRegistry());
}

MetaDataUtilsWrapper::MetaDataUtilsWrapper() : ImmutablePass(ID), m_isUtilsOwner(true)
{
    initializeMetaDataUtilsWrapperPass(*PassRegistry::getPassRegistry());
    m_pMdUtils = new MetaDataUtils();
    modMD = new ModuleMetaData();
}

MetaDataUtilsWrapper::~MetaDataUtilsWrapper()
{
    if (m_isUtilsOwner)
    {
        delete m_pMdUtils;
        delete modMD;
    }
}

MetaDataUtils* MetaDataUtilsWrapper::getMetaDataUtils()
{
    return m_pMdUtils;
}

ModuleMetaData* MetaDataUtilsWrapper::getModuleMetaData()
{
    return modMD;
}
