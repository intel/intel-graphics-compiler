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

#include "visa_igc_common_header.h"                     // for LSC_L1_L3_CC
#include "common/igc_regkeys.hpp"                       // for IGC_IS_FLAG_ENABLED, IGC_GET_FLAG_VALUE
#include "Probe/Assertion.h"                            // for IGC_ASSERT_MESSAGE
#include "AdaptorCommon/RayTracing/MemRegionAnalysis.h" // for getRTRegion(), RTMemRegion

#include "common/LLVMWarningsPush.hpp" // for suppressing LLVM warnings
#include "llvm/IR/Instructions.h"      // for llvm::StoreInst, llvm::LoadInst
#include "common/LLVMWarningsPop.hpp"  // for suppressing LLVM warnings

#include "getCacheOpts.h"
#include <optional>

using namespace llvm;

namespace IGC {
/**
 * @return the store cache policy for the RTStack
 */
LSC_L1_L3_CC RTStackStorePolicy(const CodeGenContext &Ctx) {
  LSC_L1_L3_CC cacheOpts = LSC_L1IAR_WB_L3C_WB;
  // Ctx.platform.preferLSCCache() also prefers LSC_L1IAR_WB_L3C_WB
  if (IGC_IS_FLAG_ENABLED(ForceRTStackStoreCacheCtrl)) {
    cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(RTStackStoreCacheCtrl);

    IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
  }

  return cacheOpts;
}

/**
 * @return the store cache policy for the SWHotZone
 */
LSC_L1_L3_CC SWHotZoneStorePolicy(const CodeGenContext &Ctx) {
  LSC_L1_L3_CC cacheOpts = LSC_L1IAR_WB_L3C_WB;
  if (IGC_IS_FLAG_ENABLED(ForceSWHotZoneStoreCacheCtrl)) {
    cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(SWHotZoneStoreCacheCtrl);

    IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
  }

  return cacheOpts;
}

/**
 * @return the store cache policy for the SWStack
 */
LSC_L1_L3_CC SWStackStorePolicy(const CodeGenContext &Ctx) {
  LSC_L1_L3_CC cacheOpts = Ctx.platform.NeedsLSCFenceUGMBeforeEOT()
                               ? LSC_L1S_L3C_WB
                               : (Ctx.platform.preferLSCCache() ? LSC_L1IAR_WB_L3C_WB : LSC_L1UC_L3C_WB);
  if (IGC_IS_FLAG_ENABLED(ForceSWStackStoreCacheCtrl)) {
    cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(SWStackStoreCacheCtrl);

    IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
  }

  return cacheOpts;
}

/**
 * @return the load cache policy for the RTStack
 */
LSC_L1_L3_CC RTStackLoadPolicy(const CodeGenContext &Ctx) {
  LSC_L1_L3_CC cacheOpts = LSC_L1C_WT_L3C_WB;
  // Ctx.platform.preferLSCCache() also prefers LSC_L1C_WT_L3C_WB
  if (IGC_IS_FLAG_ENABLED(ForceRTStackLoadCacheCtrl)) {
    cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(RTStackLoadCacheCtrl);

    IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
  }

  return cacheOpts;
}

/**
 * @return the load cache policy for the SWHotZone
 */
LSC_L1_L3_CC SWHotZoneLoadPolicy(const CodeGenContext &Ctx) {
  LSC_L1_L3_CC cacheOpts = LSC_L1C_WT_L3C_WB;
  if (IGC_IS_FLAG_ENABLED(ForceSWHotZoneLoadCacheCtrl)) {
    cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(SWHotZoneLoadCacheCtrl);

    IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
  }

  return cacheOpts;
}

/**
 * @return the load cache policy for the SWStack
 */
LSC_L1_L3_CC SWStackLoadPolicy(const CodeGenContext &Ctx) {
  LSC_L1_L3_CC cacheOpts = Ctx.platform.preferLSCCache() ? LSC_L1C_WT_L3C_WB : LSC_L1UC_L3C_WB;
  if (IGC_IS_FLAG_ENABLED(ForceSWStackLoadCacheCtrl)) {
    cacheOpts = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(SWStackLoadCacheCtrl);

    IGC_ASSERT_MESSAGE(cacheOpts < LSC_CC_INVALID, "Invalid Custom LSC Cache Control Set");
  }

  return cacheOpts;
}

std::optional<LSC_L1_L3_CC> getCacheOptsStorePolicy(const Value *Ptr, const CodeGenContext &Ctx) {
  auto Region = getRTRegion(Ptr, *Ctx.getModuleMetaData());

  if (!Region)
    return std::nullopt;

  std::optional<LSC_L1_L3_CC> cacheOpts;

  switch (*Region) {
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

std::optional<LSC_L1_L3_CC> getCacheOptsStorePolicy(const StoreInst &storeInst, const CodeGenContext &Ctx) {
  return getCacheOptsStorePolicy(storeInst.getPointerOperand(), Ctx);
}

std::optional<LSC_L1_L3_CC> getCacheOptsLoadPolicy(const Value *Ptr, const CodeGenContext &Ctx) {
  auto Region = getRTRegion(Ptr, *Ctx.getModuleMetaData());

  if (!Region)
    return std::nullopt;

  std::optional<LSC_L1_L3_CC> cacheOpts;

  switch (*Region) {
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

std::optional<LSC_L1_L3_CC> getCacheOptsLoadPolicy(const LoadInst &loadInst, const CodeGenContext &Ctx) {
  return getCacheOptsLoadPolicy(loadInst.getPointerOperand(), Ctx);
}

LSC_CACHE_OPTS translateLSCCacheControlsEnum(LSC_L1_L3_CC l1l3cc, bool isLoad, const llvm::Value *warningContextValue,
                                             CodeGenContext &Ctx) {
  if (!Ctx.platform.isSupportedLSCCacheControlsEnum(l1l3cc, isLoad)) {
    Ctx.EmitWarning("Unsupported cache controls configuration requested. "
                    "Applying default configuration.",
                    warningContextValue);
    l1l3cc = LSC_L1DEF_L3DEF;
  }

  bool hasNewCacheEncoding = Ctx.platform.hasNewLSCCacheEncoding();
  bool hasEfficient64bEnabled = Ctx.platform.hasEfficient64bEnabled();
  if (hasNewCacheEncoding && l1l3cc > LSC_L1_L3_CC::LSC_CC_INVALID) {
    LSC_CACHE_OPTS cacheOpts{LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
    if (isLoad) {
      LSC_LDCC_L1_L2_L3 l1l2l3cc = static_cast<LSC_LDCC_L1_L2_L3>(l1l3cc);
      IGC_ASSERT_EXIT_MESSAGE(l1l2l3cc <= LSC_LDCC_L1IAR_L2IAR_L3IAR, "unsupported caching option");
      switch (l1l2l3cc) {
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1_L2_L3_DEF:
        cacheOpts = {LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1UC_L2UC_L3UC:
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1UC_L2UC_L3C:
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED, LSC_CACHING_CACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1UC_L2C_L3UC:
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_CACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1UC_L2C_L3C:
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_CACHED, LSC_CACHING_CACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1C_L2UC_L3UC:
        cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1C_L2UC_L3C:
        cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_UNCACHED, LSC_CACHING_CACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1C_L2C_L3UC:
        cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_CACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1C_L2C_L3C:
        cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_CACHED, LSC_CACHING_CACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1S_L2UC_L3UC:
        cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1S_L2UC_L3C:
        cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_UNCACHED, LSC_CACHING_CACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1S_L2C_L3UC:
        cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_CACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1S_L2C_L3C:
        cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_CACHED, LSC_CACHING_CACHED};
        break;
      case LSC_LDCC_L1_L2_L3::LSC_LDCC_L1IAR_L2IAR_L3IAR:
        cacheOpts = {LSC_CACHING_READINVALIDATE, LSC_CACHING_READINVALIDATE, LSC_CACHING_READINVALIDATE};
        break;
      default:
        IGC_ASSERT_EXIT_MESSAGE(0, "unsupported caching option");
        break;
      }
      return cacheOpts;
    } else // store and atomics (atomics up to and including
           // LSC_STCC_L1UC_L2WB_L3UC)
    {
      LSC_STCC_L1_L2_L3 l1l2l3cc = static_cast<LSC_STCC_L1_L2_L3>(l1l3cc);
      IGC_ASSERT_EXIT_MESSAGE(l1l2l3cc <= LSC_STCC_L1WB_L2UC_L3WB, "unsupported caching option");
      switch (l1l2l3cc) {
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1_L2_L3_DEF:
        cacheOpts = {LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1UC_L2UC_L3UC:
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1UC_L2UC_L3WB:
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED, LSC_CACHING_WRITEBACK};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1UC_L2WB_L3UC:
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_WRITEBACK, LSC_CACHING_UNCACHED};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1UC_L2WB_L3WB:
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_WRITEBACK, LSC_CACHING_WRITEBACK};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1WT_L2UC_L3UC:
        cacheOpts = {LSC_CACHING_WRITETHROUGH, LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1WT_L2UC_L3WB:
        cacheOpts = {LSC_CACHING_WRITETHROUGH, LSC_CACHING_UNCACHED, LSC_CACHING_WRITEBACK};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1WT_L2WB_L3UC:
        cacheOpts = {LSC_CACHING_WRITETHROUGH, LSC_CACHING_WRITEBACK, LSC_CACHING_UNCACHED};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1WT_L2WB_L3WB:
        cacheOpts = {LSC_CACHING_WRITETHROUGH, LSC_CACHING_WRITEBACK, LSC_CACHING_WRITEBACK};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1S_L2UC_L3UC:
        cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1S_L2UC_L3WB:
        cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_UNCACHED, LSC_CACHING_WRITEBACK};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1S_L2WB_L3UC:
        cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_WRITEBACK, LSC_CACHING_UNCACHED};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1WB_L2UC_L3UC:
        cacheOpts = {LSC_CACHING_WRITEBACK, LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1WB_L2WB_L3UC:
        cacheOpts = {LSC_CACHING_WRITEBACK, LSC_CACHING_WRITEBACK, LSC_CACHING_UNCACHED};
        break;
      case LSC_STCC_L1_L2_L3::LSC_STCC_L1WB_L2UC_L3WB:
        cacheOpts = {LSC_CACHING_WRITEBACK, LSC_CACHING_UNCACHED, LSC_CACHING_WRITEBACK};
        break;
      default:
        IGC_ASSERT_EXIT_MESSAGE(0, "unsupported caching option");
        break;
      }
    }
    if (!hasEfficient64bEnabled) {
      // Workaround to keep backward compatibility in case sendg with L2 is not
      // available.
      cacheOpts.l3 = cacheOpts.l2;
      cacheOpts.l2 = LSC_CACHING_DEFAULT;
    }
    return cacheOpts;
  }
  LSC_CACHE_OPTS cacheOpts{LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
  switch (l1l3cc) {
  case LSC_L1DEF_L3DEF:
    cacheOpts = {LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
    break;
  case LSC_L1UC_L3UC:
    cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
    break;
  case LSC_L1UC_L3C_WB:
    cacheOpts = {LSC_CACHING_UNCACHED, isLoad ? LSC_CACHING_CACHED : LSC_CACHING_WRITEBACK};
    break;
  case LSC_L1C_WT_L3UC:
    cacheOpts = {isLoad ? LSC_CACHING_CACHED : LSC_CACHING_WRITETHROUGH, LSC_CACHING_UNCACHED};
    break;
  case LSC_L1C_WT_L3C_WB:
    if (isLoad)
      cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_CACHED};
    else
      cacheOpts = {LSC_CACHING_WRITETHROUGH, LSC_CACHING_WRITEBACK};
    break;
  case LSC_L1S_L3UC:
    cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_UNCACHED};
    break;
  case LSC_L1S_L3C_WB:
    cacheOpts = {LSC_CACHING_STREAMING, isLoad ? LSC_CACHING_CACHED : LSC_CACHING_WRITEBACK};
    break;
  case LSC_L1IAR_WB_L3C_WB:
    if (isLoad)
      cacheOpts = {LSC_CACHING_READINVALIDATE, LSC_CACHING_CACHED};
    else
      cacheOpts = {LSC_CACHING_WRITEBACK, LSC_CACHING_WRITEBACK};
    break;
  case LSC_L1UC_L3CC:
    if (isLoad) {
      if (hasNewCacheEncoding) {
        // Const cached not available, but it corresponds to
        // L2=cached L3=cached in new cache encoding.
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_CACHED, LSC_CACHING_CACHED};
        break;
      }
      cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_CONSTCACHED};
      break;
    }
  case LSC_L1C_L3CC:
    if (isLoad) {
      if (hasNewCacheEncoding) {
        // Const cached not available, but it corresponds to
        // L2=cached L3=cached in new cache encoding.
        cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_CACHED, LSC_CACHING_CACHED};
        break;
      }
      cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_CONSTCACHED};
      break;
    }
  case LSC_L1IAR_L3IAR:
    if (isLoad) {
      if (hasNewCacheEncoding) {
        // only IAR, IAR, IAR allowed
        cacheOpts = {LSC_CACHING_READINVALIDATE, LSC_CACHING_READINVALIDATE, LSC_CACHING_READINVALIDATE};
        break;
      }
      cacheOpts = {LSC_CACHING_READINVALIDATE, LSC_CACHING_READINVALIDATE};
      break;
    }
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "unsupported caching option");
    break;
  }
  if (hasEfficient64bEnabled && hasNewCacheEncoding) {
    // Correct L2 settings of L1L3 input for scenario
    // when Efficient64b is enabled and we will need valid L2
    // for correct encoding combination.
    // In most cases we will assign L2 caching and set L3 to uncached.
    // All special cases (IAR, constant cache) are handled earlier in switch
    // l1l3cc above.
    if (cacheOpts.l2 == LSC_CACHING_DEFAULT) {
      cacheOpts.l2 = cacheOpts.l3;
      if (cacheOpts.l3 != LSC_CACHING_DEFAULT) {
        cacheOpts.l3 = LSC_CACHING_UNCACHED;
      }
    }
  }
  return cacheOpts;
}

} // namespace IGC
