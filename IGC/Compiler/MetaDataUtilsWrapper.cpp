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