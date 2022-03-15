/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/common/StringMacros.hpp"
#include "visa_igc_common_header.h"
#include "common.h"
#include "G4_Opcode.h"
#include "PlatformInfo.h"
#include <cctype>

//for exception handling
//FIXME: potentially not thread safe, but should be ok since it's debugging code
std::stringstream errorMsgs;

using namespace vISA;
const PlatformInfo PlatformInfo::ALL_PLATFORMS[] = {
    PlatformInfo(GENX_BDW, PlatformGen::GEN8, 3, 32, "BDW", "GEN8"),
    PlatformInfo(GENX_CHV, PlatformGen::GEN8, 4, 32, "CHV", "GEN8LP"),
    PlatformInfo(GENX_SKL, PlatformGen::GEN9, 5, 32, "SKL", "GEN9", "KBL", "CFL"),
    PlatformInfo(GENX_BXT, PlatformGen::GEN9, 6, 32, "BXT", "GEN9LP"),
    PlatformInfo(GENX_ICLLP, PlatformGen::GEN11, 10, 32,
        "ICLLP", "ICL", "GEN11", "GEN11LP"),
    PlatformInfo(GENX_TGLLP, PlatformGen::XE, 12, 32,
        "TGLLP", "DG1", "GEN12LP"
    ),
    PlatformInfo(Xe_XeHPSDV, PlatformGen::XE, 11, 32,
        "XeHP_SDV"),
    PlatformInfo(Xe_DG2, PlatformGen::XE, 13, 32,
        "DG2"),
    PlatformInfo(Xe_PVC, PlatformGen::XE, 14, 64,
        "PVC"),
    PlatformInfo(Xe_PVCXT, PlatformGen::XE, 15, 64,
        "PVCXT"),

}; // ALL_PLATFORMS

const PlatformInfo* PlatformInfo::LookupPlatformInfo(TARGET_PLATFORM p)
{
    for (const auto &pi : ALL_PLATFORMS) {
        if (pi.platform == p)
            return &pi;
    }
    return nullptr;
}

TARGET_PLATFORM PlatformInfo::getVisaPlatformFromStr(const char * str)
{
    auto toUpperStr = [](const char *str) {
        std::string upper;
        while (*str)
            upper += (char)toupper(*str++);
        return upper;
    };

    std::string upperStr = toUpperStr(str);
    auto platform = GENX_NONE;
    for (const auto &pi : ALL_PLATFORMS) {
        const char * const* syms = &pi.symbols[0];
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

PlatformGen PlatformInfo::getPlatformGeneration(TARGET_PLATFORM platform)
{
    if (const auto *pi = LookupPlatformInfo(platform)) {
        return pi->family;
    } else {
        assert(false && "invalid platform");
        return PlatformGen::GEN_UNKNOWN;
    }
}

const char* PlatformInfo::kUnknownPlatformStr = "???";

const char* PlatformInfo::getGenxPlatformString() const
{
    return symbols[0] ? symbols[0] : kUnknownPlatformStr;
}

const char* PlatformInfo::getGenxPlatformString(TARGET_PLATFORM platform)
{
    if (const auto *pi = LookupPlatformInfo(platform)) {
        return pi->getGenxPlatformString();
    } else {
        return kUnknownPlatformStr;
    }
}

// returns an array of all supported platforms
const TARGET_PLATFORM* PlatformInfo::getGenxAllPlatforms(int *num)
{
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
const char * const* PlatformInfo::getGenxPlatformStrings(TARGET_PLATFORM p)
{
    if (const auto *pi = LookupPlatformInfo(p)) {
        return pi->symbols;
    } else {
        assert(false && "invalid platform");
        return nullptr;
    }
}

// The encoding of gen platform defined in vISA spec:
// 3 BDW
// 4 CHV
// 5 SKL
// 6 BXT
// 7 CNL
// 8 ICL
// 10 ICLLP
// 12 TGLLP
// 11 XeHP_SDV
// 13 DG2
// 14 PVC
// 15 PVC_XT
// Note that encoding is not linearized.
int PlatformInfo::getGenxPlatformEncoding(TARGET_PLATFORM platform)
{
    if (const auto *pi = LookupPlatformInfo(platform)) {
        return pi->encoding;
    } else {
        assert(false && "invalid platform");
        return -1;
    }
}
