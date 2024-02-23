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
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryUsageAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakConstantExpr/BreakConstantExpr.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/Atomics/ResolveOCLAtomics.hpp"
#include "Compiler/CustomSafeOptPass.hpp"
#include "IGC/common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/CodeGen/Passes.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
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
        auto doSyncDispatchRays = false;


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

        if (IGC_IS_FLAG_DISABLED(DisableStatefulSWStackAccess) && !doSyncDispatchRays)
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

static constexpr uint32_t numRegkey()
{
    uint32_t i = 0;
#define RTMEMORY_STYLE_OPTION(Name, Val) i++;
#include "igc_regkeys_enums_defs.h"
    RTMEMORY_STYLE_OPTIONS
#undef RTMEMORY_STYLE_OPTION
#undef RTMEMORY_STYLE_OPTIONS
        return i;
}

static constexpr uint32_t numLayouts()
{
    uint32_t i = 0;
#define STYLE(X) i++;
#include "RayTracingMemoryStyle.h"
#undef STYLE
    return i;
}

static void setupRTMemoryStyle(CodeGenContext* pContext)
{
    static_assert(numRegkey() == numLayouts() + 1, "add the key!");
    enum class StyleOptions
    {
#define RTMEMORY_STYLE_OPTION(Name, Val) Name = Val,
#include "igc_regkeys_enums_defs.h"
        RTMEMORY_STYLE_OPTIONS
#undef RTMEMORY_STYLE_OPTION
#undef RTMEMORY_STYLE_OPTIONS
    };

    auto Style = StyleOptions(IGC_GET_FLAG_VALUE(RTMemoryStyleOptions));

    auto& rtInfo = pContext->getModuleMetaData()->rtInfo;

    switch (Style)
    {
        case StyleOptions::Auto:
            rtInfo.MemStyle = RTMemoryStyle::Xe;
            break;
#define STYLE(X)                                \
        case StyleOptions::X:                   \
            rtInfo.MemStyle = RTMemoryStyle::X; \
            break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
}


namespace IGC
{

void RayTracingInlineLowering(CodeGenContext* pContext)
{
    IGCPassManager mpm(pContext, "RayTracingInlineLowering");
    setupRegionBTIs(pContext);
    setupRTMemoryStyle(pContext);
    mpm.add(new CodeGenContextWrapper(pContext));

    if (IGC_IS_FLAG_ENABLED(OverrideTMax))
        mpm.add(createOverrideTMaxPass(IGC_GET_FLAG_VALUE(OverrideTMax)));


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
