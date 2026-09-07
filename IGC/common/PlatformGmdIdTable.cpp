/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/PlatformGmdIdTable.h"

#include "Probe/Assertion.h"

#include <cstdint>
#include <sstream>
#include <vector>

namespace IGC {

namespace {

struct GmdIdPlatformEntry {
  uint32_t GmdArch;
  uint32_t GmdRelease;
  PRODUCT_FAMILY Product;
  GFXCORE_FAMILY Core;
};

// clang-format off
const GmdIdPlatformEntry GmdIdPlatformTable[] = {
    // ---- GMDArch 12: ---------------------------------------------------------------------------------
    {GFX_GMD_ARCH_12,   0,                                        IGFX_TIGERLAKE_LP,  IGFX_GEN12LP_CORE},
    {GFX_GMD_ARCH_12,   1,                                        IGFX_ROCKETLAKE,    IGFX_GEN12LP_CORE},
    {GFX_GMD_ARCH_12,   2,                                        IGFX_ALDERLAKE_S,   IGFX_GEN12LP_CORE},
    {GFX_GMD_ARCH_12,   3,                                        IGFX_ALDERLAKE_P,   IGFX_GEN12LP_CORE},
    {GFX_GMD_ARCH_12,   4,                                        IGFX_ALDERLAKE_N,   IGFX_GEN12LP_CORE},
    {GFX_GMD_ARCH_12,   10,                                       IGFX_DG1,           IGFX_GEN12LP_CORE},
    {GFX_GMD_ARCH_12,   GFX_GMD_ARCH_12_RELEASE_XE_HPG_1255,      IGFX_DG2,           IGFX_XE_HPG_CORE},
    {GFX_GMD_ARCH_12,   GFX_GMD_ARCH_12_RELEASE_XE_HPG_1256,      IGFX_DG2,           IGFX_XE_HPG_CORE},
    {GFX_GMD_ARCH_12,   GFX_GMD_ARCH_12_RELEASE_XE_HPG_1257,      IGFX_DG2,           IGFX_XE_HPG_CORE},
    {GFX_GMD_ARCH_12,   GFX_GMD_ARCH_12_RELEASE_XE_HPC_1260,      IGFX_PVC,           IGFX_XE_HPC_CORE},
    {GFX_GMD_ARCH_12,   GFX_GMD_ARCH_12_RELEASE_XE_HPC_1261,      IGFX_PVC,           IGFX_XE_HPC_CORE},
    {GFX_GMD_ARCH_12,   GFX_GMD_ARCH_12_RELEASE_XE_LPG_PLUS_1274, IGFX_ARROWLAKE,     IGFX_XE_HPG_CORE},
    {GFX_GMD_ARCH_12,   GFX_GMD_ARCH_12_RELEASE_XE_LP_MD,         IGFX_ARROWLAKE,     IGFX_XE_HPG_CORE},
    {GFX_GMD_ARCH_12,   GFX_GMD_ARCH_12_RELEASE_XE_LP_LG,         IGFX_METEORLAKE,    IGFX_XE_HPG_CORE},
    {GFX_GMD_ARCH_12,   GFX_GMD_ARCH_12_RELEASE_XE_LP_MD,         IGFX_METEORLAKE,    IGFX_XE_HPG_CORE},

    // ---- GMDArch 20: Xe2 -----------------------------------------------------------------------------
    {GFX_GMD_ARCH_20,   GFX_GMD_ARCH_20_RELEASE_XE2_HPG_2001,     IGFX_BMG,           IGFX_XE2_HPG_CORE},
    {GFX_GMD_ARCH_20,   GFX_GMD_ARCH_20_RELEASE_XE2_HPG_2002,     IGFX_BMG,           IGFX_XE2_HPG_CORE},
    {GFX_GMD_ARCH_20,   GFX_GMD_ARCH_20_RELEASE_XE2_LPG,          IGFX_LUNARLAKE,     IGFX_XE2_HPG_CORE},

    // ---- GMDArch 30: Xe3 -----------------------------------------------------------------------------
    {GFX_GMD_ARCH_30,   GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3000,     IGFX_PTL,           IGFX_XE3_CORE},
    {GFX_GMD_ARCH_30,   GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3001,     IGFX_PTL,           IGFX_XE3_CORE},
    {GFX_GMD_ARCH_30,   GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3003,     IGFX_PTL /*WCL*/,   IGFX_XE3_CORE},
    {GFX_GMD_ARCH_30,   GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3005,     IGFX_NVL_XE3G,      IGFX_XE3_CORE},
    {GFX_GMD_ARCH_30,   GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3004,     IGFX_NVL_XE3G,      IGFX_XE3_CORE},

    // ---- GMDArch 35: Xe3p ----------------------------------------------------------------------------
    {GFX_GMD_ARCH_35,   GFX_GMD_ARCH_35_RELEASE_XE3P_XPC_3511,    IGFX_CRI,           IGFX_XE3P_CORE},
    {GFX_GMD_ARCH_35,   GFX_GMD_ARCH_35_RELEASE_XE3P_LPG_3510,    IGFX_NVL,           IGFX_XE3P_CORE},

    // --------------------------------------------------------------------------------------------------
};
// clang-format on

constexpr size_t NumGmdIdPlatformEntries = sizeof(GmdIdPlatformTable) / sizeof(GmdIdPlatformTable[0]);
static_assert(NumGmdIdPlatformEntries > 0, "The GMDID platform table must not be empty.");

#define RET_NAME(value)                                                                                                \
  case value:                                                                                                          \
    return #value;

const char *getProductFamilyName(PRODUCT_FAMILY Product) {
  switch (Product) {
    RET_NAME(IGFX_TIGERLAKE_LP)
    RET_NAME(IGFX_ROCKETLAKE)
    RET_NAME(IGFX_ALDERLAKE_S)
    RET_NAME(IGFX_ALDERLAKE_P)
    RET_NAME(IGFX_ALDERLAKE_N)
    RET_NAME(IGFX_DG1)
    RET_NAME(IGFX_DG2)
    RET_NAME(IGFX_PVC)
    RET_NAME(IGFX_ARROWLAKE)
    RET_NAME(IGFX_METEORLAKE)
    RET_NAME(IGFX_BMG)
    RET_NAME(IGFX_LUNARLAKE)
    RET_NAME(IGFX_PTL)
    RET_NAME(IGFX_NVL_XE3G)
    RET_NAME(IGFX_CRI)
    RET_NAME(IGFX_NVL)
  default:
    return nullptr;
  }
}

const char *getRenderCoreFamilyName(GFXCORE_FAMILY Core) {
  switch (Core) {
    RET_NAME(IGFX_GEN12_CORE)
    RET_NAME(IGFX_GEN12LP_CORE)
    RET_NAME(IGFX_XE_HP_CORE)
    RET_NAME(IGFX_XE_HPG_CORE)
    RET_NAME(IGFX_XE_HPC_CORE)
    RET_NAME(IGFX_XE2_HPG_CORE)
    RET_NAME(IGFX_XE3_CORE)
    RET_NAME(IGFX_XE3P_CORE)
  default:
    return nullptr;
  }
}

#undef RET_NAME

std::string formatGmdId(uint32_t Arch, uint32_t Release, uint32_t Revision) {
  std::ostringstream Out;
  Out << Arch << "." << Release << "." << Revision;
  return Out.str();
}

std::string formatProduct(PRODUCT_FAMILY Product) {
  const char *Name = getProductFamilyName(Product);
  std::ostringstream Out;
  Out << (Name ? Name : "<unrecognized>") << " (" << static_cast<unsigned>(Product) << ")";
  return Out.str();
}

std::string formatCore(GFXCORE_FAMILY Core) {
  const char *Name = getRenderCoreFamilyName(Core);
  std::ostringstream Out;
  Out << (Name ? Name : "<unrecognized>") << " (" << static_cast<unsigned>(Core) << ")";
  return Out.str();
}

std::string formatPair(PRODUCT_FAMILY Product, GFXCORE_FAMILY Core) {
  return formatProduct(Product) + " / " + formatCore(Core);
}

std::string buildUnvalidatedMessage(const PLATFORM &Platform, const std::string &Reason) {
  std::ostringstream Out;
  Out << "render GMDID " << Reason << ", so the reported platform could not be validated. Compiling for "
      << formatPair(Platform.eProductFamily, Platform.eRenderCoreFamily) << " as reported by the runtime.";
  return Out.str();
}

const char *describeMismatchScope(const PLATFORM &Platform, const std::vector<const GmdIdPlatformEntry *> &Candidates) {
  bool ProductMatches = false;
  bool CoreMatches = false;
  for (const GmdIdPlatformEntry *Entry : Candidates) {
    ProductMatches |= Entry->Product == Platform.eProductFamily;
    CoreMatches |= Entry->Core == Platform.eRenderCoreFamily;
  }

  IGC_ASSERT_MESSAGE(!(ProductMatches && CoreMatches), "table rows sharing a GMDID must share a render core family");

  if (ProductMatches)
    return "render core family";
  if (CoreMatches)
    return "product family";
  return "product family and render core family";
}

std::string buildMismatchMessage(const PLATFORM &Platform, uint32_t Arch, uint32_t Release, uint32_t Revision,
                                 const std::vector<const GmdIdPlatformEntry *> &Candidates) {
  std::ostringstream Out;
  Out << "platform mismatch (" << describeMismatchScope(Platform, Candidates) << "): render GMDID "
      << formatGmdId(Arch, Release, Revision) << " expects ";
  if (Candidates.size() > 1)
    Out << "one of ";

  for (size_t I = 0; I < Candidates.size(); ++I)
    Out << (I ? ", " : "") << formatPair(Candidates[I]->Product, Candidates[I]->Core);

  Out << ", but the runtime reported " << formatPair(Platform.eProductFamily, Platform.eRenderCoreFamily)
      << ". This usually means IGC and the runtime were built from different "
         "igfxfmid.h revisions, or the runtime populated PLATFORM inconsistently.";
  return Out.str();
}
} // namespace

PlatformCheckOutcome checkPlatformConsistency(const PLATFORM &Platform) {
  PlatformCheckOutcome Outcome;

  if (Platform.sRenderBlockID.Value == 0) {
    Outcome.Result = PlatformCheckResult::Unverifiable;
    Outcome.Message = buildUnvalidatedMessage(Platform, "was not reported");
    return Outcome;
  }

  const uint32_t Arch = GFX_GET_GMD_ARCH_VERSION_RENDER(Platform);
  const uint32_t Release = GFX_GET_GMD_RELEASE_VERSION_RENDER(Platform);
  const uint32_t Revision = GFX_GET_GMD_REV_ID_RENDER(Platform);

  std::vector<const GmdIdPlatformEntry *> Candidates;
  for (size_t I = 0; I < NumGmdIdPlatformEntries; ++I) {
    const GmdIdPlatformEntry &Entry = GmdIdPlatformTable[I];
    if (Entry.GmdArch != Arch || Entry.GmdRelease != Release)
      continue;

    if (Entry.Product == Platform.eProductFamily && Entry.Core == Platform.eRenderCoreFamily) {
      Outcome.Result = PlatformCheckResult::Match;
      return Outcome;
    }
    Candidates.push_back(&Entry);
  }

  if (Candidates.empty()) {
    Outcome.Result = PlatformCheckResult::UnknownGmdId;
    Outcome.Message =
        buildUnvalidatedMessage(Platform, formatGmdId(Arch, Release, Revision) + " is not known to this IGC") +
        " If this is a new platform, add it to PlatformGmdIdTable.cpp.";
    return Outcome;
  }

  Outcome.Result = PlatformCheckResult::Mismatch;
  Outcome.Message = buildMismatchMessage(Platform, Arch, Release, Revision, Candidates);
  return Outcome;
}

} // namespace IGC
