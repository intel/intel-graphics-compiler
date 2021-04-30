/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/MetaDataUtilsWrapperInitializer.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-metadata-utils-wrapper-initializer"
#define PASS_DESCRIPTION "Metadata Utils Wrapper Initializer"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MetaDataUtilsWrapperInitializer, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(MetaDataUtilsWrapperInitializer, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char MetaDataUtilsWrapperInitializer::ID = 0;

MetaDataUtilsWrapperInitializer::MetaDataUtilsWrapperInitializer() : ModulePass(ID)
{
    initializeMetaDataUtilsWrapperInitializerPass(*PassRegistry::getPassRegistry());
}

bool MetaDataUtilsWrapperInitializer::runOnModule(Module& M)
{
    MetaDataUtilsWrapper& mduw = getAnalysis<MetaDataUtilsWrapper>();
    mduw.getMetaDataUtils()->setModule(&M);
    return true;
}
