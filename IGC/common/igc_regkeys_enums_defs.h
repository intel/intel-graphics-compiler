/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This header contains enums that we want to expose via registry keys.
/// By including this at the point of enum definition and using the results
/// in igc_flags.h, we can guarantee that the implementation does not get out
/// of sync with the advertised options.
///
/// Here are the steps to add a new entry (see below for examples):
/// 1. Come up with a name that represents a single entry in your enum (for
///    example, LSC_CACHE_CTRL_OPTION).
/// 2. wrap it in an #ifdef so that only the given value is selected at an
///    include site.
/// 3. Make a define with the plural of the name that lists out the options
///    (for example, LSC_CACHE_CTRL_OPTIONS).
/// 4. At enum definition:
///        a. define the singular macro
///        b. include this file
///        c. invoke the plural macro
///        d. undef both the singular and plural macros
/// 5. Do the same in igc_regkeys.h so the plural macro can be referenced in
///    igc_flags.h.
///
//===----------------------------------------------------------------------===//

#ifdef LSC_CACHE_CTRL_OPTION
    #define LSC_CACHE_CTRL_OPTIONS                                                                                                      \
        LSC_CACHE_CTRL_OPTION(LSC_L1DEF_L3DEF,     0,  "default")                                                                       \
        LSC_CACHE_CTRL_OPTION(LSC_L1UC_L3UC,       1,  "Load: L1 uncached   L3 uncached # Store: L1 uncached      L3 uncached")         \
        LSC_CACHE_CTRL_OPTION(LSC_L1UC_L3C_WB,     2,  "Load: L1 uncached   L3 cached   # Store: L1 uncached      L3 write-back")       \
        LSC_CACHE_CTRL_OPTION(LSC_L1C_WT_L3UC,     3,  "Load: L1 cached     L3 uncached # Store: L1 write-through L3 uncached")         \
        LSC_CACHE_CTRL_OPTION(LSC_L1C_WT_L3C_WB,   4,  "Load: L1 cached     L3 cached   # Store: L1 write-through L3 write-back")       \
        LSC_CACHE_CTRL_OPTION(LSC_L1S_L3UC,        5,  "Load: L1 streaming  L3 uncached # Store: L1 streaming     L3 uncached")         \
        LSC_CACHE_CTRL_OPTION(LSC_L1S_L3C_WB,      6,  "Load: L1 streaming  L3 cached   # Store: L1 streaming     L3 write-back")       \
        LSC_CACHE_CTRL_OPTION(LSC_L1IAR_WB_L3C_WB, 7,  "Load: L1 invalidate after read L3 cached # Store: L1 write-back L3 write-back") \
                                                                                                                                        \
        LSC_CACHE_CTRL_OPTION(LSC_CC_INVALID,      8,  "Invalid")
#endif // LSC_CACHE_CTRL_OPTION

#ifdef EARLY_OUT_CS_PATTERN
    #define EARLY_OUT_CS_PATTERNS                           \
        EARLY_OUT_CS_PATTERN(SamplePatternEnable,     0x1)  \
        EARLY_OUT_CS_PATTERN(DPMaxPatternEnable,      0x2)  \
        EARLY_OUT_CS_PATTERN(DPFSatPatternEnable,     0x4)  \
        EARLY_OUT_CS_PATTERN(NdotLPatternEnable,      0x8)  \
        EARLY_OUT_CS_PATTERN(SelectFcmpPatternEnable, 0x10)
#endif // EARLY_OUT_CS_PATTERN

#ifdef EARLY_OUT_PS_PATTERN
    #define EARLY_OUT_PS_PATTERNS                             \
        EARLY_OUT_PS_PATTERN(SamplePatternEnable,       0x1)  \
        EARLY_OUT_PS_PATTERN(DPMaxPatternEnable,        0x2)  \
        EARLY_OUT_PS_PATTERN(DPFSatPatternEnable,       0x4)  \
        EARLY_OUT_PS_PATTERN(NdotLPatternEnable,        0x8)  \
        EARLY_OUT_PS_PATTERN(DirectOutputPatternEnable, 0x10) \
        EARLY_OUT_PS_PATTERN(MulMaxMatchEnable,         0x20)
#endif // EARLY_OUT_PS_PATTERN

#ifdef FP_BINOP_INSTRUCTION
    #define FP_BINOP_INSTRUCTIONS                  \
            FP_BINOP_INSTRUCTION(FAdd,       0x1)  \
            FP_BINOP_INSTRUCTION(FSub,       0x2)  \
            FP_BINOP_INSTRUCTION(FMul,       0x4)  \
            FP_BINOP_INSTRUCTION(FDiv,       0x8)
#endif // FP_BINOP_INSTRUCTION

#ifdef TRIBOOL_OPTION
#define TRIBOOL_OPTIONS         \
    TRIBOOL_OPTION(Default, -1) \
    TRIBOOL_OPTION(Disabled, 0) \
    TRIBOOL_OPTION(Enabled,  1)
#endif // TRIBOOL_OPTION

#ifdef RTMEMORY_STYLE_OPTION
#define RTMEMORY_STYLE_OPTIONS      \
    RTMEMORY_STYLE_OPTION(Auto, -1) \
    RTMEMORY_STYLE_OPTION(Xe,    0)
#endif // RTMEMORY_STYLE_OPTION
