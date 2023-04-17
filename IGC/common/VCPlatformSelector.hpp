/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// VC platform selection process need to be used fcl library to determine
// platform for sources compilation this is header-only because we do not want
// link dependencies from library with only two functions

#ifndef VC_PLATFORM_SELECTOR_H
#define VC_PLATFORM_SELECTOR_H

#include "llvm/Support/ErrorHandling.h"

#include "Probe/Assertion.h"
#include "StringMacros.hpp"
#include "igfxfmid.h"

#include <sstream>

namespace cmc {

constexpr int ComputeTileMaskPVC = 0x7;
inline const char *getPlatformStr(PLATFORM Platform, unsigned &RevId) {
  auto GmdId = Platform.sRenderBlockID;
  auto Core = Platform.eRenderCoreFamily;
  auto Product = Platform.eProductFamily;
  auto DeviceId = Platform.usDeviceID;
  IGC_ASSERT(RevId == Platform.usRevId);

  if (GmdId.Value != 0) {
    static std::string PlatformId;
    std::ostringstream Out;
    Out << GmdId.GmdID.GMDArch << '.' << GmdId.GmdID.GMDRelease << '.'
        << GmdId.GmdID.RevisionID;
    PlatformId = Out.str();
    return PlatformId.c_str();
  }

  // If GdmId is not defined, fallback to product id scheme
  switch (Product) {
  case IGFX_BROADWELL:
    return "bdw";
  case IGFX_SKYLAKE:
    return "skl";
  case IGFX_KABYLAKE:
    return "kbl";
  case IGFX_COFFEELAKE:
    return "cfl";
  case IGFX_BROXTON:
    return "bxt";
  case IGFX_GEMINILAKE:
    return "glk";
  case IGFX_ICELAKE:
  case IGFX_ICELAKE_LP:
    return "icllp";
  case IGFX_JASPERLAKE:
    return "jsl";
  case IGFX_TIGERLAKE_LP:
    return "tgllp";
  case IGFX_ROCKETLAKE:
    return "rkl";
  case IGFX_ALDERLAKE_S:
    return "adl-s";
  case IGFX_ALDERLAKE_P:
    return "adl-p";
  case IGFX_ALDERLAKE_N:
    return "adl-n";
  case IGFX_DG1:
    return "dg1";
  case IGFX_XE_HP_SDV:
    return "xehp-sdv";
  case IGFX_DG2:
    if (GFX_IS_DG2_G10_CONFIG(DeviceId))
      return "acm-g10";
    if (GFX_IS_DG2_G11_CONFIG(DeviceId))
      return "acm-g11";
    if (GFX_IS_DG2_G12_CONFIG(DeviceId))
      return "acm-g12";
    return "dg2";
  case IGFX_PVC:
    RevId &= cmc::ComputeTileMaskPVC;
    if (RevId <= 1)
      return "pvc-sdv";
    if (RevId == 3)
      return "12.60.3";
    return "pvc";
  case IGFX_METEORLAKE:
    return "mtl";
  default:
    break;
  }

  switch (Core) {
  case IGFX_GEN9_CORE:
    return "skl";
  case IGFX_GEN11_CORE:
    return "icllp";
  default:
    break;
  }

  return nullptr;
}

} // namespace cmc

#endif
