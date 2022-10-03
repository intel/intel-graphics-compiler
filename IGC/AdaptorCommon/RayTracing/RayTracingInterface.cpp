/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// The main pipeline of raytracing passes.  This will be run by all APIs
/// (currently DX and Vulkan).  The idea is to not put anything too API
/// specific here if possible.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "AdaptorCommon/RayTracing/RayTracingInterface.h"
#include "AdaptorCommon/RayTracing/RayTracingPasses.hpp"
#include "AdaptorCommon/RayTracing/RTBuilder.h"
#include "AdaptorCommon/RayTracing/RayTracingAddressSpaceAliasAnalysis.h"
#include "AdaptorCommon/AddImplicitArgs.hpp"
#include "AdaptorCommon/ProcessFuncAttributes.h"
#include "Compiler/CISACodeGen/CodeSinking.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryUsageAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakConstantExpr/BreakConstantExpr.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/Atomics/ResolveOCLAtomics.hpp"
#include "IGC/common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/CodeGen/Passes.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// This must be run prior to any raytracing passes so the BTI slots are
// allocated for them to use.
static void setupRegionBTIs(CodeGenContext* pContext)
{
    if (!pContext->m_DriverInfo.supportsRaytracingStatefulAccesses())
        return;

    SIMDMode Mode = SIMDMode::UNKNOWN;
    if (auto SIMDSize = pContext->knownSIMDSize())
        Mode = *SIMDSize;

    // We need to take advantage of the LSC messages to do bindless through
    // the surface state heap.
    if (!pContext->platform.LSCEnabled(Mode))
        return;

    auto getAddrspace = [&]()
    {
        BufferType BufType =
            IGC_IS_FLAG_ENABLED(DisableRTBindlessAccess) ?
                IGC::UAV :
                IGC::SSH_BINDLESS;

        // There's nothing special about using UndefValue here. We just need
        // to encode the address space as indirect.
        return EncodeAS4GFXResource(
            *UndefValue::get(Type::getInt32Ty(*pContext->getLLVMContext())),
            BufType,
            pContext->getUniqueIndirectIdx());
    };

    auto& rtInfo = pContext->getModuleMetaData()->rtInfo;

    // bump the index once in case the index is already in use
    pContext->getUniqueIndirectIdx();

    uint32_t BaseOffset = 0;
    if (pContext->type == ShaderType::RAYTRACING_SHADER)
    {
        if (IGC_IS_FLAG_DISABLED(DisableStatefulRTStackAccess))
        {
            rtInfo.RTAsyncStackAddrspace = getAddrspace();
            rtInfo.RTAsyncStackSurfaceStateOffset = BaseOffset++;
        }

        if (IGC_IS_FLAG_DISABLED(DisableStatefulSWHotZoneAccess))
        {
            rtInfo.SWHotZoneAddrspace = getAddrspace();
            rtInfo.SWHotZoneSurfaceStateOffset = BaseOffset++;
        }

        if (IGC_IS_FLAG_DISABLED(DisableStatefulSWStackAccess))
        {
            rtInfo.SWStackAddrspace = getAddrspace();
            rtInfo.SWStackSurfaceStateOffset = BaseOffset++;
        }

        if (IGC_IS_FLAG_DISABLED(DisableStatefulRTSyncStackAccess4RTShader))
        {
            rtInfo.RTSyncStackAddrspace = getAddrspace();
            rtInfo.RTSyncStackSurfaceStateOffset = BaseOffset++;
        }
    }
    else
    {
        if (IGC_IS_FLAG_DISABLED(DisableStatefulRTSyncStackAccess4nonRTShader))
        {
            rtInfo.RTSyncStackAddrspace = getAddrspace();
            rtInfo.RTSyncStackSurfaceStateOffset = BaseOffset++;
        }
    }
}

namespace IGC {

void RayTracingLowering(RayDispatchShaderContext* pContext)
{
    setupRegionBTIs(pContext);

    IGCPassManager mpm(pContext, "RayTracingLowering");

    COMPILER_TIME_START(pContext, TIME_RT_SETUP);
    COMPILER_TIME_START(pContext, TIME_RT_PASSES);

    // If we call RayTracingLowering() without calling a Unify* layer first
    // which can happen if we've passed in an empty module to compile the
    // callStackHandler, this exposes a bug in TailCallElimination as it has
    // not listed all of its dependencies. The fact that we typically call
    // instcombine in UnifyIRDXIL() hides this issue.
    if (pContext->getModule()->empty())
        mpm.add(createAAResultsWrapperPass());

    mpm.add(new CodeGenContextWrapper(pContext));
    mpm.add(new MetaDataUtilsWrapper(pContext->getMetaDataUtils(), pContext->getModuleMetaData()));


    // DCE away shaders that are not to be exported.
    mpm.add(createGlobalDCEPass());
    mpm.add(createTraceRayInlinePrepPass());
    if (IGC_IS_FLAG_ENABLED(EnableRQHideLatency)) {
        mpm.add(createTraceRayInlineLatencySchedulerPass());
        mpm.add(createCFGSimplificationPass());
    }

    // Decompose TraceRayInline related intrinsics first before continuation
    // splitting.  This may allow us to do less spill/fills to the RTStack.
    // Will investigate further as we have more workloads.
    mpm.add(CreateTraceRayInlineLoweringPass());
    // Eliminate any obvious dead stores from rayquery
    mpm.add(createDeadStoreEliminationPass());
    // convert to global pointers first since all downstream passes will be
    // doing operations to the RTStack which will involve A64 stateless
    // operations.  If we enable stateful SWStacks, then this will convert
    // to the address space associated with the SWStack.
    mpm.add(createPrivateToGlobalPointerPass());
    // Inject BTD Stack ID release calls before every return in raygen shaders.
    mpm.add(CreateStackIDRetirement());
    if (IGC_IS_FLAG_DISABLED(DisableEarlyRemat))
    {
        // greedily remat obviously profitable values to avoid spilling.
        // Some will be CSE'd back together later on.
        mpm.add(createEarlyRematPass());
    }
    // Emit GlobalBufferPointer and LocalBufferPointer intrinsics.
    mpm.add(CreateBindlessKernelArgLoweringPass());
    // We run this before splitting but, currently, any-hit and intersection
    // shaders will not have continuations.  It could be moved lower as long
    // as it runs before intrinsic lowering.
    mpm.add(createLowerIntersectionAnyHitPass());
    if (IGC_IS_FLAG_DISABLED(DisableDPSE))
    {
        mpm.add(createDeadPayloadStoreEliminationPass());
    }
    if (IGC_IS_FLAG_DISABLED(DisablePreSplitOpts))
    {
        // Sink values closer to uses.  In some cases, this will sync across
        // TraceRay() calls which will eliminate some spills.
        mpm.add(new CodeSinking(true));
    }
    // This must be run before SplitAsync for correctness.
    mpm.add(createSplitPreparePass());
    if (IGC_IS_FLAG_DISABLED(DisablePreSplitOpts))
    {
        // coalesce duplicate values to prevent spilling the same value
        // multiple times.
        mpm.add(createRayInfoCSEPass());
    }
    // After this pass, all shaders with TraceRay() or CallShader() calls will
    // be split into continuations at those call sites.
    mpm.add(createSplitAsyncPass());
    mpm.add(createCFGSimplificationPass());
    if (IGC_IS_FLAG_DISABLED(DisablePromoteToScratch) &&
        pContext->m_DriverInfo.supportsRTScratchSpace())
    {
        // Prior to intrinsic lowering, allocas that we can prove don't
        // escape the shader can be allocated in scratch space rather than
        // burning space on the RTStack.
        mpm.add(createPromoteToScratchPass());
    }
    // Last cleanup of duplicated nomem intrinsics prior to intrinsic lowering.
    mpm.add(createEarlyCSEPass());
    if (IGC_IS_FLAG_DISABLED(DisableLateRemat) &&
        IGC_IS_FLAG_DISABLED(DisableCompactifySpills))
    {
        // Do more complex cases of rematerialization and rearrangment of
        // spills/fills that were not handled earlier.
        mpm.add(createLateRematPass());
    }
    if (pContext->tryPayloadSinking() && pContext->hasUnsupportedPayloadSinkingCaseWAenabled)
    {
        mpm.add(createPayloadSinkingAnalysisPass());
    }
    // Lower intrinsics to RTStack operations.
    mpm.add(createRayTracingIntrinsicLoweringPass());
    {
        // When we arrive here, the continuations will all be marked as
        // 'alwaysinline' but __mergeContinuations will not be.
        // __mergeContinuations and the continuations form a mutually recursive
        // call graph.
        mpm.add(createAlwaysInlinerLegacyPass());
        // Now, we only have calls to __mergeContinuations tail recursive to
        // itself.  This will leave us with just branches back to the top of
        // the function.
        mpm.add(createTailCallEliminationPass());
    }
    // inline __mergeContinuations.
    mpm.add(createInlineMergeCallsPass());
    if (pContext->tryPayloadSinking())
    {
        // attempt to sink payload writes into inlined continuations in order
        // to eliminate stores/loads to/from the payload in some cases.
        mpm.add(createPayloadSinkingPass());
    }
    // Do finalization (right now, alignment adjust and intrinsic moving).
    mpm.add(createRayTracingFinalizePass());

    // if BTD continuations are enabled, need to run ProcessFuncAttribute pass
    // to set IndirectCall attribute
    mpm.add(createProcessFuncAttributesPass());
    mpm.add(new PrivateMemoryUsageAnalysis());
    mpm.add(createGlobalDCEPass());
    mpm.add(new BreakConstantExpr());

    bool supportPrintf = (pContext->m_DriverInfo.SupportsRTPrintf() && IGC_IS_FLAG_ENABLED(EnableRTPrintf));
    if (supportPrintf)
    {
        mpm.add(new OpenCLPrintfAnalysis());
    }
    mpm.add(new AddImplicitArgs());
    if (supportPrintf)
    {
        mpm.add(new OpenCLPrintfResolution());
        mpm.add(new ResolveOCLAtomics());
        mpm.add(createRayTracingPrintfPostProcessPass());
    }

    mpm.run(*pContext->getModule());

    // Run verification after generating continuation functions to ensure
    // that we still have well formed IR with all the spill/fill code generated.
    IGC_ASSERT(false == llvm::verifyModule(*pContext->getModule(), &dbgs()));

    COMPILER_TIME_END(pContext, TIME_RT_PASSES);
    COMPILER_TIME_END(pContext, TIME_RT_SETUP);

    DumpLLVMIR(pContext, "AfterRTLowering");
}

void RayTracingInlineLowering(CodeGenContext* pContext)
{
    IGCPassManager mpm(pContext, "RayTracingInlineLowering");
    setupRegionBTIs(pContext);
    mpm.add(new CodeGenContextWrapper(pContext));


    mpm.add(createTraceRayInlinePrepPass());
    if (IGC_IS_FLAG_ENABLED(EnableRQHideLatency)) {
        mpm.add(createTraceRayInlineLatencySchedulerPass());
        mpm.add(createCFGSimplificationPass());
    }
    mpm.add(CreateTraceRayInlineLoweringPass());
    mpm.add(CreateRTGlobalsPointerLoweringPass());

#ifdef _DEBUG
    // Run verification after generating continuation functions to ensure
    // that we still have well formed IR
    mpm.add(createVerifierPass(false));
#endif // _DEBUG

    mpm.run(*pContext->getModule());
}

} // namespace IGC
