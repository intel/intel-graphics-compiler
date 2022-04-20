/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/EmUtils.h"

#ifndef DECLARE_IGC_GROUP
#define DECLARE_IGC_GROUP(groupName)
#endif

// If these enum and bitmask regkeys have not been explicitly opted-in to,
// default back to using DWORD which is understood by all users.
#ifndef DECLARE_IGC_REGKEY_ENUM
#define DECLARE_IGC_REGKEY_ENUM(regkeyName, defaultValue, description, values, releaseMode) \
    DECLARE_IGC_REGKEY(DWORD, regkeyName, defaultValue, description ". " values, releaseMode)
#endif

#ifndef DECLARE_IGC_REGKEY_BITMASK
#define DECLARE_IGC_REGKEY_BITMASK(regkeyName, defaultValue, description, values, releaseMode) \
    DECLARE_IGC_REGKEY(DWORD, regkeyName, defaultValue, description ". " values, releaseMode)
#endif

#define LSC_CACHE_CTRL_OPTION(Name, Val, Description) #Name " [" Description "]=" #Val ","
#define EARLY_OUT_CS_PATTERN(Name, Val) #Name "=" #Val ","
#define EARLY_OUT_PS_PATTERN(Name, Val) #Name "=" #Val ","
#include "igc_regkeys_enums_defs.h"


#include "igc_flags.h"

#undef LSC_CACHE_CTRL_OPTION
#undef LSC_CACHE_CTRL_OPTIONS

#undef EARLY_OUT_CS_PATTERN
#undef EARLY_OUT_CS_PATTERNS
#undef EARLY_OUT_PS_PATTERN
#undef EARLY_OUT_PS_PATTERNS

#undef DECLARE_IGC_GROUP
#undef DECLARE_IGC_REGKEY_ENUM
#undef DECLARE_IGC_REGKEY_BITMASK
