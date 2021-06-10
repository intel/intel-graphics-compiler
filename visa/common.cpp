/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/common/StringMacros.hpp"
#include "visa_igc_common_header.h"
#include "common.h"
#include "G4_Opcode.h"
#include <cctype>

//for exception handling
//FIXME: potentially not thread safe, but should be ok since it's debugging code
std::stringstream errorMsgs;

static _THREAD TARGET_PLATFORM visaPlatform;

struct PlatformInfo {
    TARGET_PLATFORM  platform;
    PlatformGen      family;
    int              encoding;
    const char      *symbols[8];

    constexpr PlatformInfo(
        TARGET_PLATFORM p,
        PlatformGen f,
        int e,
        const char *str0,
        const char *str1 = nullptr,
        const char *str2 = nullptr,
        const char *str3 = nullptr,
        const char *str4 = nullptr)
        : platform(p), family(f), encoding(e), symbols{str0, str1, str2, str3, str4, nullptr}
    {
    }
}; // PlatformInfo

static const PlatformInfo ALL_PLATFORMS[] {
    PlatformInfo(GENX_BDW, PlatformGen::GEN8, 3, "BDW", "GEN8"),
    PlatformInfo(GENX_CHV, PlatformGen::GEN8, 4, "CHV", "GEN8LP"),
    PlatformInfo(GENX_SKL, PlatformGen::GEN9, 5, "SKL", "GEN9", "KBL", "CFL"),
    PlatformInfo(GENX_BXT, PlatformGen::GEN9, 6, "BXT", "GEN9LP"),
    PlatformInfo(GENX_ICLLP, PlatformGen::GEN11, 10,
        "ICLLP", "ICL", "GEN11", "GEN11LP"),
    PlatformInfo(GENX_TGLLP, PlatformGen::XE, 12,
        "TGLLP", "DG1", "GEN12LP"
    ),
    PlatformInfo(XE_HP, PlatformGen::XE, 11,
        "Xe_HP"),
}; // ALL_PLATFORMS

static const PlatformInfo *LookupPlatformInfo(TARGET_PLATFORM p)
{
    for (const auto &pi : ALL_PLATFORMS) {
        if (pi.platform == p)
            return &pi;
    }
    return nullptr;
}

int SetVisaPlatform(TARGET_PLATFORM vPlatform)
{
    assert(vPlatform >= GENX_BDW && "unsupported platform");
    visaPlatform = vPlatform;

    return VISA_SUCCESS;
}

int SetVisaPlatform(const char * str)
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
    visaPlatform = platform;
    return platform != GENX_NONE ? VISA_SUCCESS : VISA_FAILURE;
}

TARGET_PLATFORM getGenxPlatform()
{
    return visaPlatform;
}

PlatformGen getPlatformGeneration(TARGET_PLATFORM platform)
{
    if (const auto *pi = LookupPlatformInfo(platform)) {
        return pi->family;
    } else {
        assert(false && "invalid platform");
        return PlatformGen::GEN_UNKNOWN;
    }
}

const char *getGenxPlatformString(TARGET_PLATFORM platform)
{
    if (const auto *pi = LookupPlatformInfo(platform)) {
        return pi->symbols[0] ? pi->symbols[0] : "???";
    } else {
        return "???";
    }
}

// returns an array of all supported platforms
const TARGET_PLATFORM *getGenxAllPlatforms(int *num)
{
    const static int N_PLATFORMS =
        sizeof(ALL_PLATFORMS)/sizeof(ALL_PLATFORMS[0]);
    static TARGET_PLATFORM s_platforms[N_PLATFORMS];
    int i = 0;
    for (const auto &pi : ALL_PLATFORMS) {
        s_platforms[i++] = pi.platform;
    }
    *num = N_PLATFORMS;
    return s_platforms;
}

// returns nullptr terminated string for a platform
const char * const*getGenxPlatformStrings(TARGET_PLATFORM p)
{
    if (const auto *pi = LookupPlatformInfo(p)) {
        return pi->symbols;
    } else {
        assert(false && "invalid platform");
        return nullptr;
    }
}

unsigned char getGRFSize()
{
    unsigned int size = 32;


    return size;
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
// 11 XE_HP
// Note that encoding is not linearized.
int getGenxPlatformEncoding()
{
    if (const auto *pi = LookupPlatformInfo(getGenxPlatform())) {
        return pi->encoding;
    } else {
        assert(false && "invalid platform");
        return -1;
    }
}
