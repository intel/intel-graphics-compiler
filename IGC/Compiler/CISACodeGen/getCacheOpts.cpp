/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/**
 * @file getCacheOpts.cpp
 *
 * @section DESCRIPTION
 *
 * This file contains definitions of utility functions which return the cache options
 * for a given load or store instruction.
 */

#include "visa_igc_common_header.h"                             // for LSC_L1_L3_CC
#include "common/igc_regkeys.hpp"                               // for IGC_IS_FLAG_ENABLED, IGC_GET_FLAG_VALUE
#include "Probe/Assertion.h"                                    // for IGC_ASSERT_MESSAGE
#include "AdaptorCommon/RayTracing/MemRegionAnalysis.h"         // for getRTRegion(), RTMemRegion

#include "common/LLVMWarningsPush.hpp"                          // for suppressing LLVM warnings
#include "llvm/IR/Instructions.h"                               // for llvm::StoreInst, llvm::LoadInst
#include "common/LLVMWarningsPop.hpp"                           // for suppressing LLVM warnings

#include "getCacheOpts.h"
#include <optional>

using namespace llvm;

namespace IGC {
/**
 * @return the store cache policy for the RTStack
 */
LSC_L1_L3_CC RTStackStorePolicy(const CodeGenContext& Ctx)
{
    LSC_L1_L3_CC cacheOpts = LSC_L1IAR_WB_L3C_WB;
    // Ctx.platform.preferLSCCache() also prefers LSC_L1IAR_WB_L3C_WB
    if (IGC_IS_FLAG_ENABLED(ForceRTStackStoreCacheCtrl))
    {
        cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(RTStackStoreCacheCtrl);

        IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
    }

    return cacheOpts;
}

/**
 * @return the store cache policy for the SWHotZone
 */
LSC_L1_L3_CC SWHotZoneStorePolicy(const CodeGenContext& Ctx)
{
    LSC_L1_L3_CC cacheOpts = LSC_L1IAR_WB_L3C_WB;
    if (IGC_IS_FLAG_ENABLED(ForceSWHotZoneStoreCacheCtrl))
    {
        cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(SWHotZoneStoreCacheCtrl);

        IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
    }

    return cacheOpts;
}

/**
 * @return the store cache policy for the SWStack
 */
LSC_L1_L3_CC SWStackStorePolicy(const CodeGenContext &Ctx)
{
    LSC_L1_L3_CC cacheOpts = Ctx.platform.NeedsLSCFenceUGMBeforeEOT() ?
        LSC_L1S_L3C_WB :
        (Ctx.platform.preferLSCCache() ? LSC_L1IAR_WB_L3C_WB : LSC_L1UC_L3C_WB);
    if (IGC_IS_FLAG_ENABLED(ForceSWStackStoreCacheCtrl))
    {
        cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(SWStackStoreCacheCtrl);

        IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
    }

    return cacheOpts;
}

/**
 * @return the load cache policy for the RTStack
 */
LSC_L1_L3_CC RTStackLoadPolicy(const CodeGenContext& Ctx)
{
    LSC_L1_L3_CC cacheOpts = LSC_L1C_WT_L3C_WB;
    // Ctx.platform.preferLSCCache() also prefers LSC_L1C_WT_L3C_WB
    if (IGC_IS_FLAG_ENABLED(ForceRTStackLoadCacheCtrl))
    {
        cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(RTStackLoadCacheCtrl);

        IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
    }

    return cacheOpts;
}

/**
 * @return the load cache policy for the SWHotZone
 */
LSC_L1_L3_CC SWHotZoneLoadPolicy(const CodeGenContext& Ctx)
{
    LSC_L1_L3_CC cacheOpts = LSC_L1C_WT_L3C_WB;
    if (IGC_IS_FLAG_ENABLED(ForceSWHotZoneLoadCacheCtrl))
    {
        cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(SWHotZoneLoadCacheCtrl);

        IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
    }

    return cacheOpts;
}

/**
 * @return the load cache policy for the SWStack
 */
LSC_L1_L3_CC SWStackLoadPolicy(const CodeGenContext& Ctx)
{
    LSC_L1_L3_CC cacheOpts = Ctx.platform.preferLSCCache() ? LSC_L1C_WT_L3C_WB : LSC_L1UC_L3C_WB;
    if (IGC_IS_FLAG_ENABLED(ForceSWStackLoadCacheCtrl))
    {
        cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(SWStackLoadCacheCtrl);

        IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
    }

    return cacheOpts;
}

std::optional<LSC_L1_L3_CC> getCacheOptsStorePolicy(
    const Value* Ptr,
    const CodeGenContext &Ctx)
{
    auto Region = getRTRegion(Ptr, *Ctx.getModuleMetaData());

    if (!Region)
        return std::nullopt;

    std::optional<LSC_L1_L3_CC> cacheOpts;

    switch (*Region)
    {
    case RTMemRegion::RTAsyncStack:
    case RTMemRegion::RTSyncStack:
        cacheOpts = RTStackStorePolicy(Ctx);
        break;
    case RTMemRegion::SWStack:
        cacheOpts = SWStackStorePolicy(Ctx);
        break;
    case RTMemRegion::SWHotZone:
        cacheOpts = SWHotZoneStorePolicy(Ctx);
        break;
    default:
        break;
    }

    return cacheOpts;
}

std::optional<LSC_L1_L3_CC> getCacheOptsStorePolicy(
    const StoreInst& storeInst,
    const CodeGenContext &Ctx)
{
    return getCacheOptsStorePolicy(storeInst.getPointerOperand(), Ctx);
}

std::optional<LSC_L1_L3_CC> getCacheOptsLoadPolicy(
    const Value* Ptr,
    const CodeGenContext &Ctx)
{
    auto Region = getRTRegion(Ptr, *Ctx.getModuleMetaData());

    if (!Region)
        return std::nullopt;

    std::optional<LSC_L1_L3_CC> cacheOpts;

    switch (*Region)
    {
    case RTMemRegion::RTAsyncStack:
    case RTMemRegion::RTSyncStack:
        cacheOpts = RTStackLoadPolicy(Ctx);
        break;
    case RTMemRegion::SWStack:
        cacheOpts = SWStackLoadPolicy(Ctx);
        break;
    case RTMemRegion::SWHotZone:
        cacheOpts = SWHotZoneLoadPolicy(Ctx);
        break;
    default:
        break;
    }

    return cacheOpts;
}

std::optional<LSC_L1_L3_CC> getCacheOptsLoadPolicy(
    const LoadInst& loadInst,
    const CodeGenContext &Ctx)
{
    return getCacheOptsLoadPolicy(loadInst.getPointerOperand(), Ctx);
}

} // namespace IGC

