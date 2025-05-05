/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass will visit each load/store and determine, according to memory
/// region and regkeys, what the per message LSC cache control should be. It
/// does this by marking an instruction with !lsc.cache.ctrl metadata and relies
/// on EmitVISA to read the metadata and generate the proper controls.
///
/// This pass is targeted at raytracing right now but could potentially be
/// extended to other shader types if desired.
///
//===----------------------------------------------------------------------===//

#include "LSCControlsAnalysisPass.h"
#include "IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "getCacheOpts.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include <optional>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char LSCControlsAnalysisPass::ID = 0;

bool LSCControlsAnalysisPass::runOnFunction(Function& F)
{
    Changed = false;
    m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    visit(F);

    return Changed;
}

static void markCacheCtrl(Instruction& I, LSC_L1_L3_CC Ctrl)
{
    MDNode* node = MDNode::get(
        I.getContext(),
        ConstantAsMetadata::get(
            ConstantInt::get(Type::getInt32Ty(I.getContext()), Ctrl)));
    I.setMetadata("lsc.cache.ctrl", node);
}

void LSCControlsAnalysisPass::visitStoreInst(StoreInst& I)
{
    auto cacheOpts = getCacheOptsStorePolicy(I, *m_CGCtx);

    if (cacheOpts)
    {
        MDNode* node = MDNode::get(
            I.getContext(),
            ConstantAsMetadata::get(
                ConstantInt::get(Type::getInt32Ty(I.getContext()), 0)));
        I.setMetadata("enable.vmask", node);
        markCacheCtrl(I, *cacheOpts);
        Changed = true;
    }
}

void LSCControlsAnalysisPass::visitLoadInst(LoadInst& I)
{
    auto cacheOpts = getCacheOptsLoadPolicy(I, *m_CGCtx);

    if (cacheOpts)
    {
        markCacheCtrl(I, *cacheOpts);
        Changed = true;
    }
}

void LSCControlsAnalysisPass::visitCallInst(CallInst& CI)
{
    auto* GII = dyn_cast<GenIntrinsicInst>(&CI);
    if (!GII)
        return;

    std::optional<LSC_L1_L3_CC> cacheOpts;

    switch (GII->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_ldraw_indexed:
    case GenISAIntrinsic::GenISA_ldrawvector_indexed:
    {
        auto* LRI = cast<LdRawIntrinsic>(GII);
        cacheOpts = getCacheOptsLoadPolicy(LRI->getResourceValue(), *m_CGCtx);
        break;
    }
    case GenISAIntrinsic::GenISA_storeraw_indexed:
    case GenISAIntrinsic::GenISA_storerawvector_indexed:
    {
        auto* SRI = cast<StoreRawIntrinsic>(GII);
        cacheOpts = getCacheOptsStorePolicy(SRI->getResourceValue(), *m_CGCtx);
        break;
    }
    default:
        break;
    }

    if (cacheOpts)
    {
        markCacheCtrl(CI, *cacheOpts);
        Changed = true;
    }
}

namespace IGC {

#define PASS_FLAG "LSC-Controls-Analysis-pass"
#define PASS_DESCRIPTION "Load/Store cache controls analysis pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LSCControlsAnalysisPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LSCControlsAnalysisPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

FunctionPass* CreateLSCControlsAnalysisPass()
{
    return new LSCControlsAnalysisPass();
}

}
