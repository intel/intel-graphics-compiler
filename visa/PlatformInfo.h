/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _PLATFORMINFO_H_
#define _PLATFORMINFO_H_

#include "common.h"

namespace vISA {

class PlatformInfo {
private:
    static const PlatformInfo ALL_PLATFORMS[];
    static const char* kUnknownPlatformStr;
public:
    TARGET_PLATFORM platform;
    PlatformGen     family;
    int             encoding;
    const char*     symbols[8];

    constexpr PlatformInfo(
        TARGET_PLATFORM p,
        PlatformGen f,
        int e,
        const char* str0,
        const char* str1 = nullptr,
        const char* str2 = nullptr,
        const char* str3 = nullptr,
        const char* str4 = nullptr)
        : platform(p), family(f), encoding(e), symbols{str0, str1, str2, str3, str4, nullptr}
    {
    }

    const char* getGenxPlatformString() const;

    /*************** internal jitter functions ********************/
    static const PlatformInfo* LookupPlatformInfo(TARGET_PLATFORM p);

    // currently the supported arguments are listed in the common.cpp table
    // generally the symbol suffix is the preferred name:
    //   e.g. GENX_SKL  means "SKL"
    //             ^^^
    static TARGET_PLATFORM getVisaPlatformFromStr(const char* platformName);

    // returns an array of all supported platforms
    static const TARGET_PLATFORM* getGenxAllPlatforms(int* num);

    static const char* getGenxPlatformString(TARGET_PLATFORM);

    // returns nullptr terminated array of strings that can be parsed
    // for the platform name
    static const char* const* getGenxPlatformStrings(TARGET_PLATFORM);

    // returns the vISA encoding bits for encoding
    // NOTE: encoding values are not necessarily in order
    static int getGenxPlatformEncoding(TARGET_PLATFORM platform);

    // return the platform generation that can be used for comparison
    static PlatformGen getPlatformGeneration(TARGET_PLATFORM platform);
}; // PlatformInfo

} // namepsace vISA

#endif // _PLATFORMINFO_H_
