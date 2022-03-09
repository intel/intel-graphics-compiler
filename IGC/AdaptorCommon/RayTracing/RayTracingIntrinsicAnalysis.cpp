/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass and its Resolution equivalent follow the familar AdaptorOCL
/// pattern of an analysis followed by a resolution pass.  The idea early
/// on was that continuations would be invoked often as function calls so we
/// need to promote the below intrinsics to implicit args so we could later call
/// BuiltinCallGraphAnalysis to propagate the arguments through all calls.  It
/// is not currently clear how much we'll use this approach: it may be that
/// BTD + inlining of small continuations may is the way to go.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "AdaptorCommon/ImplicitArgs.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

//////////////////////////////////////////////////////////////////////////
//
// Pass detects the use of ray tracing intrinsics and prepares them for
// addition as implicit arguments
class RayTracingIntrinsicAnalysis : public ModulePass, public InstVisitor<RayTracingIntrinsicAnalysis>
{
public:
    RayTracingIntrinsicAnalysis();
    bool runOnModule(Module& M) override;
    bool runOnFunction(Function &F);
    StringRef getPassName() const override
    {
        return "RayTracingIntrinsicAnalysis";
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<MetaDataUtilsWrapper>();
    }

    void visitCallInst(CallInst &CI);

    static char ID;
private:
    bool hasGlobalPointer;
    bool hasLocalPointer;
    bool hasStackID;
    bool hasInlinedData;
    IGCMD::MetaDataUtils* pMdUtils;
};

#define PASS_FLAG "raytracing-intrinsic-analysis"
#define PASS_DESCRIPTION "Mark raytracing intrinsics as implicit args"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayTracingIntrinsicAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(RayTracingIntrinsicAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

RayTracingIntrinsicAnalysis::RayTracingIntrinsicAnalysis() : ModulePass(ID) {
    initializeRayTracingIntrinsicAnalysisPass(*PassRegistry::getPassRegistry());
}

char RayTracingIntrinsicAnalysis::ID = 0;

void RayTracingIntrinsicAnalysis::visitCallInst(CallInst &CI)
{
    if (auto *GII = dyn_cast<GenIntrinsicInst>(&CI))
    {
        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_GlobalBufferPointer:
            hasGlobalPointer = true;
            return;
        case GenISAIntrinsic::GenISA_LocalBufferPointer:
            hasLocalPointer = true;
            return;
        case GenISAIntrinsic::GenISA_AsyncStackID:
            hasStackID = true;
            return;
        case GenISAIntrinsic::GenISA_InlinedData:
            hasInlinedData = true;
            return;
        default:
            break;
        }
    }
}

bool RayTracingIntrinsicAnalysis::runOnModule(Module& M)
{
    bool changed = false;
    pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    // Run on all functions defined in this module
    for (Function& F : M)
    {
        if (F.isDeclaration())
        {
            continue;
        }
        if (runOnFunction(F))
        {
            changed = true;
        }
    }

    // Update LLVM metadata based on IGC MetadataUtils
    if (changed)
        pMdUtils->save(M.getContext());

    return changed;
}

bool RayTracingIntrinsicAnalysis::runOnFunction(Function &F)
{
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
        return false;

    hasGlobalPointer = false;
    hasLocalPointer  = false;
    hasStackID       = false;
    hasInlinedData   = false;

    visit(F);

    bool changed = false;
    SmallVector<ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS> implicitArgs;

    if (hasGlobalPointer)
        implicitArgs.push_back(ImplicitArg::RT_GLOBAL_BUFFER_POINTER);

    if (hasLocalPointer)
        implicitArgs.push_back(ImplicitArg::RT_LOCAL_BUFFER_POINTER);

    if (hasStackID)
        implicitArgs.push_back(ImplicitArg::RT_STACK_ID);

    if (hasInlinedData)
        implicitArgs.push_back(ImplicitArg::RT_INLINED_DATA);

    if (!implicitArgs.empty())
    {
        // Create IGC metadata representing the implicit args needed by this function
        ImplicitArgs::addImplicitArgs(
            F,
            implicitArgs,
            pMdUtils);
        changed = true;
    }
    return changed;
}
namespace IGC
{
    Pass* createRayTracingIntrinsicAnalysisPass()
    {
        return new RayTracingIntrinsicAnalysis();
    }
}
