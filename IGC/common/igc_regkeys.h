/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/EmUtils.h"

#ifndef DECLARE_IGC_GROUP
#define DECLARE_IGC_GROUP(groupName)
#endif

#ifndef DECLARE_IGC_REGKEY_UMD
#define DECLARE_IGC_REGKEY_UMD(dataType, regkeyName, defaultValue, description, flagAvailability)                      \
  DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, flagAvailability)
#define UNDEF_DECLARE_IGC_REGKEY_UMD
#endif

// If these enum and bitmask regkeys have not been explicitly opted-in to,
// default back to using DWORD which is understood by all users.
#ifndef DECLARE_IGC_REGKEY_ENUM
#define DECLARE_IGC_REGKEY_ENUM(regkeyName, defaultValue, description, values, flagAvailability)                       \
  DECLARE_IGC_REGKEY(DWORD, regkeyName, defaultValue, description ". " values, flagAvailability)
#endif

#ifndef DECLARE_IGC_REGKEY_ENUM_UMD
#define DECLARE_IGC_REGKEY_ENUM_UMD(regkeyName, defaultValue, description, values, flagAvailability)                   \
  DECLARE_IGC_REGKEY_ENUM(regkeyName, defaultValue, description, values, flagAvailability)
#define UNDEF_DECLARE_IGC_REGKEY_ENUM_UMD
#endif

#ifndef DECLARE_IGC_REGKEY_BITMASK
#define DECLARE_IGC_REGKEY_BITMASK(regkeyName, defaultValue, description, values, flagAvailability)                    \
  DECLARE_IGC_REGKEY(DWORD, regkeyName, defaultValue, description ". " values, flagAvailability)
#endif

#ifndef DECLARE_IGC_REGKEY_BITMASK_UMD
#define DECLARE_IGC_REGKEY_BITMASK_UMD(regkeyName, defaultValue, description, values, flagAvailability)                \
  DECLARE_IGC_REGKEY_BITMASK(regkeyName, defaultValue, description, values, flagAvailability)
#define UNDEF_DECLARE_IGC_REGKEY_BITMASK_UMD
#endif

#define IGC_REGKEY_STRINGIFY_IMPL(Value) #Value
#define IGC_REGKEY_STRINGIFY(Value) IGC_REGKEY_STRINGIFY_IMPL(Value)

#define LSC_CACHE_CTRL_OPTION(Name, Val, Description) #Name " [" Description "]=" IGC_REGKEY_STRINGIFY(Val) ","
#define LSC_CACHE_CTRL_LOAD_OPTION(Name, Val, Description) #Name " [" Description "]=" IGC_REGKEY_STRINGIFY(Val) ","
#define LSC_CACHE_CTRL_STORE_OPTION(Name, Val, Description) #Name " [" Description "]=" IGC_REGKEY_STRINGIFY(Val) ","
#define EARLY_OUT_CS_PATTERN(Name, Val) #Name "=" IGC_REGKEY_STRINGIFY(Val) ","
#define EARLY_OUT_PS_PATTERN(Name, Val) #Name "=" IGC_REGKEY_STRINGIFY(Val) ","
#define FP_BINOP_INSTRUCTION(Name, Val) #Name "=" IGC_REGKEY_STRINGIFY(Val) ","
#define SHADER_TYPE_MASK(Name, Val) #Name "=" IGC_REGKEY_STRINGIFY(Val) ","
#define TRIBOOL_OPTION(Name, Val) #Name "=" IGC_REGKEY_STRINGIFY(Val) ","
#define RTMEMORY_STYLE_OPTION(Name, Val) #Name "=" IGC_REGKEY_STRINGIFY(Val) ","
#define FILENAME_COLLISION_MODE(Name, Val, Description) #Name " [" Description "]=" IGC_REGKEY_STRINGIFY(Val) ","
#define NEW_INLINE_RAYTRACING_FLAG(Name, Val, Description) #Name " [" Description "]=" IGC_REGKEY_STRINGIFY(Val) ","
#define REMAT_FLAG(Name, Val, Description) #Name " [" Description "]=" IGC_REGKEY_STRINGIFY(Val) ","

#define INJECT_PRINTF_OPTION(Name, Val) #Name "=" IGC_REGKEY_STRINGIFY(Val) ","


#include "igc_regkeys_enums_defs.h"


#include "igc_flags.h"

#undef NEW_INLINE_RAYTRACING_FLAG
#undef REMAT_FLAG
#undef LSC_CACHE_CTRL_OPTION
#undef LSC_CACHE_CTRL_OPTIONS
#undef LSC_CACHE_CTRL_LOAD_OPTION
#undef LSC_CACHE_CTRL_LOAD_OPTIONS
#undef LSC_CACHE_CTRL_STORE_OPTION
#undef LSC_CACHE_CTRL_STORE_OPTIONS

#undef EARLY_OUT_CS_PATTERN
#undef EARLY_OUT_CS_PATTERNS
#undef EARLY_OUT_PS_PATTERN
#undef EARLY_OUT_PS_PATTERNS

#undef FP_BINOP_INSTRUCTION
#undef FP_BINOP_INSTRUCTIONS

#undef TRIBOOL_OPTION
#undef TRIBOOL_OPTIONS

#undef RTMEMORY_STYLE_OPTION
#undef RTMEMORY_STYLE_OPTIONS
#undef FILENAME_COLLISION_MODE
#undef FILENAME_COLLISION_MODES
#undef INJECT_PRINTF_OPTION
#undef INJECT_PRINTF_OPTIONS

#undef DECLARE_IGC_GROUP
#undef DECLARE_IGC_REGKEY_ENUM
#undef DECLARE_IGC_REGKEY_BITMASK
#undef IGC_REGKEY_STRINGIFY
#undef IGC_REGKEY_STRINGIFY_IMPL

#ifdef UNDEF_DECLARE_IGC_REGKEY_UMD
#undef DECLARE_IGC_REGKEY_UMD
#undef UNDEF_DECLARE_IGC_REGKEY_UMD
#endif
#ifdef UNDEF_DECLARE_IGC_REGKEY_ENUM_UMD
#undef DECLARE_IGC_REGKEY_ENUM_UMD
#undef UNDEF_DECLARE_IGC_REGKEY_ENUM_UMD
#endif
#ifdef UNDEF_DECLARE_IGC_REGKEY_BITMASK_UMD
#undef DECLARE_IGC_REGKEY_BITMASK_UMD
#undef UNDEF_DECLARE_IGC_REGKEY_BITMASK_UMD
#endif

