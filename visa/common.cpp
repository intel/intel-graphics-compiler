/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common.h"
#include "G4_Opcode.h"
#include "IGC/common/StringMacros.hpp"
#include "PlatformInfo.h"
#include "visa_igc_common_header.h"
#include <cctype>
#include <vector>

using namespace vISA;
const PlatformInfo PlatformInfo::ALL_PLATFORMS[] = {
    PlatformInfo(GENX_BDW, PlatformGen::GEN8, 3, 32, "BDW", "GEN8"),
    PlatformInfo(GENX_CHV, PlatformGen::GEN8, 4, 32, "CHV", "GEN8LP"),
    PlatformInfo(GENX_SKL, PlatformGen::GEN9, 5, 32, "SKL", "GEN9", "KBL",
                 "CFL"),
    PlatformInfo(GENX_BXT, PlatformGen::GEN9, 6, 32, "BXT", "GEN9LP"),
    PlatformInfo(GENX_ICLLP, PlatformGen::GEN11, 10, 32, "ICLLP", "ICL",
                 "GEN11", "GEN11LP"),
    PlatformInfo(GENX_TGLLP, PlatformGen::XE, 12, 32, "TGLLP", "DG1", "GEN12LP",
                 "ADL"),
    PlatformInfo(Xe_XeHPSDV, PlatformGen::XE, 11, 32, "XeHP_SDV"),
    PlatformInfo(Xe_DG2, PlatformGen::XE, 13, 32, "DG2"),
    PlatformInfo(Xe_MTL, PlatformGen::XE, 13, 32, "MTL"),
    PlatformInfo(Xe_ARL, PlatformGen::XE, 13, 32, "ARL"),
    PlatformInfo(Xe_PVC, PlatformGen::XE, 14, 64, "PVC"),
    PlatformInfo(Xe_PVCXT, PlatformGen::XE, 15, 64, "PVCXT"),
    PlatformInfo(Xe2, PlatformGen::XE2, 18, 64, "XE2",
                 "LNL"),
    PlatformInfo(Xe3, PlatformGen::XE3, 19, 64, "XE3", "PTL"),
    PlatformInfo(Xe3P_CRI, PlatformGen::XE3, 21, 64, "XE3P_CRI", "CRI"),
}; // ALL_PLATFORMS

const PlatformInfo *PlatformInfo::LookupPlatformInfo(TARGET_PLATFORM p) {
  for (const auto &pi : ALL_PLATFORMS) {
    if (pi.platform == p)
      return &pi;
  }
  return nullptr;
}

TARGET_PLATFORM PlatformInfo::getVisaPlatformFromStr(const char *str) {
  auto toUpperStr = [](const char *str) {
    std::string upper;
    while (*str)
      upper += (char)toupper(*str++);
    return upper;
  };

  std::string upperStr = toUpperStr(str);
  auto platform = GENX_NONE;
  for (const auto &pi : ALL_PLATFORMS) {
    const char *const *syms = &pi.symbols[0];
    while (*syms) {
      if (upperStr == toUpperStr(*syms)) {
        platform = pi.platform;
        break;
      }
      syms++;
    }
    if (platform != GENX_NONE)
      break;
  }
  return platform;
}

PlatformGen PlatformInfo::getPlatformGeneration(TARGET_PLATFORM platform) {
  if (const auto *pi = LookupPlatformInfo(platform)) {
    return pi->family;
  } else {
    vISA_ASSERT_UNREACHABLE("invalid platform");
    return PlatformGen::GEN_UNKNOWN;
  }
}

const char *PlatformInfo::kUnknownPlatformStr = "???";

const char *PlatformInfo::getGenxPlatformString() const {
  return symbols[0] ? symbols[0] : kUnknownPlatformStr;
}

const char *PlatformInfo::getGenxPlatformString(TARGET_PLATFORM platform) {
  if (const auto *pi = LookupPlatformInfo(platform)) {
    return pi->getGenxPlatformString();
  } else {
    return kUnknownPlatformStr;
  }
}

// returns an array of all supported platforms
const TARGET_PLATFORM *PlatformInfo::getGenxAllPlatforms(int *num) {
  const static int N_PLATFORMS =
      sizeof(ALL_PLATFORMS) / sizeof(ALL_PLATFORMS[0]);
  static TARGET_PLATFORM s_platforms[N_PLATFORMS];
  int i = 0;
  for (const auto &pi : ALL_PLATFORMS) {
    s_platforms[i++] = pi.platform;
  }
  *num = N_PLATFORMS;
  return s_platforms;
}

// returns nullptr terminated string for a platform
const char *const *PlatformInfo::getGenxPlatformStrings(TARGET_PLATFORM p) {
  if (const auto *pi = LookupPlatformInfo(p)) {
    return pi->symbols;
  } else {
    vISA_ASSERT_UNREACHABLE("invalid platform");
    return nullptr;
  }
}

// The encoding of gen platform defined in vISA spec
// Note that encoding is not linearized.
int PlatformInfo::getGenxPlatformEncoding(TARGET_PLATFORM platform) {
  if (const auto *pi = LookupPlatformInfo(platform)) {
    return pi->encoding;
  } else {
    vISA_ASSERT_UNREACHABLE("invalid platform");
    return -1;
  }
}

#if !defined(NDEBUG) && !defined(DLL_MODE)
namespace vISA {
bool DebugFlag = false;
bool DebugAllFlag = false;

// This should set by each pass via setCurrentDebugPass()
static const char *CurrentDebugPass = nullptr;
// This is set when processing the vISA "-debug-only" option.
static std::vector<std::string> PassesToDebug;

void setCurrentDebugPass(const char *Name) { CurrentDebugPass = Name; }

void addPassToDebug(std::string Name) { PassesToDebug.push_back(Name); }

bool isCurrentDebugPass() {
  if (DebugAllFlag)
    return true;
  if (!CurrentDebugPass)
    return false;
  for (auto &pass : PassesToDebug) {
    if (pass.compare(CurrentDebugPass) == 0)
      return true;
  }
  return false;
}
} // namespace vISA
#endif // NDEBUG
