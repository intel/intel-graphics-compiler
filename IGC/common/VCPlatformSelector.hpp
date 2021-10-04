/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// VC platform selection process need to be used in two places:
// * igcdeps library to determine platform for SPIRV compilation
// * fcl library to determine platform for sources compilation
// this is header-only because we do not want link dependencies
// from library with only two functions

#ifndef VC_PLATFORM_SELECTOR_H
#define VC_PLATFORM_SELECTOR_H

#include "Probe/Assertion.h"
#include "StringMacros.hpp"
#include "igfxfmid.h"

namespace cmc {

inline const char *getPlatformStr(PLATFORM Platform, unsigned &RevId) {
  // after some consultations with Wndows KMD folks,
  // only render core shall be used in all cases
  auto Core = Platform.eRenderCoreFamily;
  auto Product = Platform.eProductFamily;
  IGC_ASSERT(RevId == Platform.usRevId);

  switch (Core) {
  case IGFX_GEN9_CORE:
    return "SKL";
  case IGFX_GEN10_CORE:
    return "CNL";
  case IGFX_GEN11_CORE:
    if (Product == IGFX_ICELAKE_LP || Product == IGFX_LAKEFIELD)
      return "ICLLP";
    return "ICL";
  case IGFX_GEN12_CORE:
  case IGFX_GEN12LP_CORE:
  case IGFX_XE_HP_CORE:
    if (Product == IGFX_TIGERLAKE_LP)
      return "TGLLP";
    if (Product == IGFX_DG1)
      return "DG1";
    if (Product == IGFX_ROCKETLAKE)
      return "RKL";
    if (Product == IGFX_ALDERLAKE_S)
      return "ADLS";
    if (Product == IGFX_ALDERLAKE_P)
      return "ADLP";
    else if (Product == IGFX_XE_HP_SDV)
      return "XEHP";
  default:
    IGC_ASSERT_MESSAGE(0, "unsupported platform");
    break;
  }
  return "SKL";
}

} // namespace cmc

#endif
