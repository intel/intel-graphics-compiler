/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// All this does is invoke the inliner.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "Compiler/IGCPassSupport.h"
#include "iStdLib/utility.h"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

class InlineMergeCallsPass : public ModulePass
{
public:
    InlineMergeCallsPass() : ModulePass(ID)
    {
        initializeInlineMergeCallsPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module& M) override;
    StringRef getPassName() const override
    {
        return "InlineMergeCallsPass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
    }

    static char ID;
};

char InlineMergeCallsPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "raytracing-inline-merge-calls"
#define PASS_DESCRIPTION "Inline __mergeContinuation after inlining continuations"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InlineMergeCallsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(InlineMergeCallsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool InlineMergeCallsPass::runOnModule(Module& M)
{
    // Prior in the pipeline, we have run tail call elimination so that all
    // the recursive calls from __mergeContinuation to itself have been
    // converted into branches.  We can now inline it here given that all
    // recursion has been eliminated.
    if (auto * mergedContinuationFunc = M.getFunction(RTBuilder::MergeFuncName))
    {
        mergedContinuationFunc->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
        mergedContinuationFunc->setLinkage(GlobalValue::InternalLinkage);
        legacy::PassManager PM;
        PM.add(createAlwaysInlinerLegacyPass());
        PM.run(M);
    }

    return true;
}

namespace IGC
{

    Pass* createInlineMergeCallsPass(void)
    {
        return new InlineMergeCallsPass();
    }

} // namespace IGC
