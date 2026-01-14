/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2024 Intel Corporation

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

#define LSC_CACHE_CTRL_NUM 31
#ifdef LSC_CACHE_CTRL_OPTION
#define LSC_CACHE_CTRL_OPTIONS_WITH_CTRL_NUM(ctrl_num)                                                                 \
  LSC_CACHE_CTRL_OPTION(LSC_L1DEF_L3DEF, 0, "default")                                                                 \
  LSC_CACHE_CTRL_OPTION(LSC_L1UC_L3UC, 1, "Load: L1 uncached   L3 uncached # Store: L1 uncached      L3 uncached")     \
  LSC_CACHE_CTRL_OPTION(LSC_L1UC_L3C_WB, 2, "Load: L1 uncached   L3 cached   # Store: L1 uncached      L3 write-back") \
  LSC_CACHE_CTRL_OPTION(LSC_L1C_WT_L3UC, 3, "Load: L1 cached     L3 uncached # Store: L1 write-through L3 uncached")   \
  LSC_CACHE_CTRL_OPTION(LSC_L1C_WT_L3C_WB, 4,                                                                          \
                        "Load: L1 cached     L3 cached   # Store: L1 write-through L3 write-back")                     \
  LSC_CACHE_CTRL_OPTION(LSC_L1S_L3UC, 5, "Load: L1 streaming  L3 uncached # Store: L1 streaming     L3 uncached")      \
  LSC_CACHE_CTRL_OPTION(LSC_L1S_L3C_WB, 6, "Load: L1 streaming  L3 cached   # Store: L1 streaming     L3 write-back")  \
  LSC_CACHE_CTRL_OPTION(LSC_L1IAR_WB_L3C_WB, 7,                                                                        \
                        "Load: L1 invalidate after read L3 cached # Store: L1 write-back L3 write-back")               \
  LSC_CACHE_CTRL_OPTION(LSC_L1UC_L3CC, 8, "Load: L1 uncached   L3 const-cached # Store: N/A")                          \
  LSC_CACHE_CTRL_OPTION(LSC_L1C_L3CC, 9, "Load: L1 cached     L3 const-cached # Store: N/A")                           \
  LSC_CACHE_CTRL_OPTION(LSC_L1IAR_L3IAR, 10, "Load: L1 invalidate after read L3 invalidate after read # Store: N/A")   \
                                                                                                                       \
  LSC_CACHE_CTRL_OPTION(LSC_CC_INVALID, 11, "Invalid")                                                                 \
  LSC_CACHE_CTRL_OPTION(LSC_CC_NUM, ctrl_num, "Invalid")

#define LSC_CACHE_CTRL_OPTIONS LSC_CACHE_CTRL_OPTIONS_WITH_CTRL_NUM(LSC_CACHE_CTRL_NUM)
#endif // LSC_CACHE_CTRL_OPTION
#ifdef LSC_CACHE_CTRL_LOAD_OPTION
#define LSC_CACHE_CTRL_LOAD_OPTIONS                                                                                    \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_INVALID, 11, "Invalid")                                                          \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1_L2_L3_DEF, 16, "default")                                                     \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1UC_L2UC_L3UC, 18, "Load: L1 uncached  L2 uncached  L3 uncached")               \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1UC_L2UC_L3C, 19, "Load: L1 uncached  L2 uncached  L3 cached")                  \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1UC_L2C_L3UC, 20, "Load: L1 uncached  L2 cached    L3 uncached")                \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1UC_L2C_L3C, 21, "Load: L1 uncached  L2 cached    L3 cached")                   \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1C_L2UC_L3UC, 22, "Load: L1 cached    L2 uncached  L3 uncached")                \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1C_L2UC_L3C, 23, "Load: L1 cached    L2 uncached  L3 cached")                   \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1C_L2C_L3UC, 24, "Load: L1 cached    L2 cached    L3 uncached")                 \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1C_L2C_L3C, 25, "Load: L1 cached    L2 cached    L3 cached")                    \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1S_L2UC_L3UC, 26, "Load: L1 streaming L2 uncached  L3 uncached")                \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1S_L2UC_L3C, 27, "Load: L1 streaming L2 uncached  L3 cached")                   \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1S_L2C_L3UC, 28, "Load: L1 streaming L2 cached    L3 uncached")                 \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1S_L2C_L3C, 29, "Load: L1 streaming L2 cached    L3 cached")                    \
  LSC_CACHE_CTRL_LOAD_OPTION(LSC_LDCC_L1IAR_L2IAR_L3IAR, 30, "Load: L1 L2 L3 invalidate after read")
#endif // LSC_CACHE_CTRL_LOAD_OPTION
#ifdef LSC_CACHE_CTRL_STORE_OPTION
#define LSC_CACHE_CTRL_STORE_OPTIONS                                                                                   \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_INVALID, 11, "Invalid")                                                         \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1_L2_L3_DEF, 16, "default")                                                    \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1UC_L2UC_L3UC, 18, "Store: L1 uncached      L2 uncached   L3 uncached")        \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1UC_L2UC_L3WB, 19, "Store: L1 uncached      L2 uncached   L3 write-back")      \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1UC_L2WB_L3UC, 20, "Store: L1 uncached      L2 write-back L3 uncached")        \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1UC_L2WB_L3WB, 21, "Store: L1 uncached      L2 write-back L3 write-back")      \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1WT_L2UC_L3UC, 22, "Store: L1 write-through L2 uncached   L3 uncached")        \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1WT_L2UC_L3WB, 23, "Store: L1 write-through L2 uncached   L3 write-back")      \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1WT_L2WB_L3UC, 24, "Store: L1 write-through L2 write-back L3 uncached")        \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1WT_L2WB_L3WB, 25, "Store: L1 write-through L2 write-back L3 write-back")      \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1S_L2UC_L3UC, 26, "Store: L1 streaming     L2 uncached   L3 uncached")         \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1S_L2UC_L3WB, 27, "Store: L1 streaming     L2 uncached   L3 write-back")       \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1S_L2WB_L3UC, 28, "Store: L1 streaming     L2 write-back L3 uncached")         \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1WB_L2UC_L3UC, 29, "Store: L1 write-back    L2 uncached   L3 uncached")        \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1WB_L2WB_L3UC, 30, "Store: L1 write-back    L2 write-back L3 uncached")        \
  LSC_CACHE_CTRL_STORE_OPTION(LSC_STCC_L1WB_L2UC_L3WB, 31, "Store: L1 write-back    L2 uncached   L3 write-back")
#endif // LSC_CACHE_CTRL_STORE_OPTION

#ifdef EARLY_OUT_CS_PATTERN
#define EARLY_OUT_CS_PATTERNS                                                                                          \
  EARLY_OUT_CS_PATTERN(SamplePatternEnable, 0x1)                                                                       \
  EARLY_OUT_CS_PATTERN(DPMaxPatternEnable, 0x2)                                                                        \
  EARLY_OUT_CS_PATTERN(DPFSatPatternEnable, 0x4)                                                                       \
  EARLY_OUT_CS_PATTERN(NdotLPatternEnable, 0x8)                                                                        \
  EARLY_OUT_CS_PATTERN(SelectFcmpPatternEnable, 0x10)
#endif // EARLY_OUT_CS_PATTERN

#ifdef EARLY_OUT_PS_PATTERN
#define EARLY_OUT_PS_PATTERNS                                                                                          \
  EARLY_OUT_PS_PATTERN(SamplePatternEnable, 0x1)                                                                       \
  EARLY_OUT_PS_PATTERN(DPMaxPatternEnable, 0x2)                                                                        \
  EARLY_OUT_PS_PATTERN(DPFSatPatternEnable, 0x4)                                                                       \
  EARLY_OUT_PS_PATTERN(NdotLPatternEnable, 0x8)                                                                        \
  EARLY_OUT_PS_PATTERN(DirectOutputPatternEnable, 0x10)                                                                \
  EARLY_OUT_PS_PATTERN(MulMaxMatchEnable, 0x20)
#endif // EARLY_OUT_PS_PATTERN

// Not able to convert a C++ value (ShaderType::) back to preprocessor
// copy value by hand
#ifdef SHADER_TYPE_MASK
#define SHADER_TYPE_MASKS                                                                                              \
  SHADER_TYPE_MASK(VS, 0x2)                                                                                            \
  SHADER_TYPE_MASK(HS, 0x4)                                                                                            \
  SHADER_TYPE_MASK(DS, 0x8)                                                                                            \
  SHADER_TYPE_MASK(GS, 0x10)                                                                                           \
  SHADER_TYPE_MASK(TS, 0x20)                                                                                           \
  SHADER_TYPE_MASK(MS, 0x40)                                                                                           \
  SHADER_TYPE_MASK(PS, 0x80)                                                                                           \
  SHADER_TYPE_MASK(CS, 0x100)                                                                                          \
  SHADER_TYPE_MASK(OCL, 0x200)                                                                                         \
  SHADER_TYPE_MASK(RT, 0x400)
#endif // SHADER_TYPE_MASKS

#ifdef FP_BINOP_INSTRUCTION
#define FP_BINOP_INSTRUCTIONS                                                                                          \
  FP_BINOP_INSTRUCTION(FAdd, 0x1)                                                                                      \
  FP_BINOP_INSTRUCTION(FSub, 0x2)                                                                                      \
  FP_BINOP_INSTRUCTION(FMul, 0x4)                                                                                      \
  FP_BINOP_INSTRUCTION(FDiv, 0x8)
#endif // FP_BINOP_INSTRUCTION

#ifdef TRIBOOL_OPTION
#define TRIBOOL_OPTIONS                                                                                                \
  TRIBOOL_OPTION(Default, -1)                                                                                          \
  TRIBOOL_OPTION(Disabled, 0)                                                                                          \
  TRIBOOL_OPTION(Enabled, 1)
#endif // TRIBOOL_OPTION


#ifdef NEW_INLINE_RAYTRACING_FLAG
#define NEW_INLINE_RAYTRACING_MASK                                                                                     \
  NEW_INLINE_RAYTRACING_FLAG(NonRTShaders, 0x01, "Enable for all non-raytracing shaders")                              \
  NEW_INLINE_RAYTRACING_FLAG(RTShaders, 0x02, "Enable for all raytracing shaders")
#endif // NEW_INLINE_RAYTRACING_FLAG

#ifdef REMAT_FLAG
#define REMAT_MASK                                                                                                     \
  REMAT_FLAG(REMAT_NONE, 0x0, "Disable remat entirely")                                                                \
  REMAT_FLAG(REMAT_LOADS, 0x01, "Remat loads")                                                                         \
  REMAT_FLAG(REMAT_STORES, 0x02, "Remat stores")                                                                       \
  REMAT_FLAG(REMAT_ARGS, 0x04, "Remat call arguments")                                                                 \
  REMAT_FLAG(REMAT_COMPARISONS, 0x08, "Remat comparison instructions")
#endif // REMAT_FLAG

#ifdef INJECT_PRINTF_OPTION
#define INJECT_PRINTF_OPTIONS                                                                                          \
  INJECT_PRINTF_OPTION(InjectPrintfNone, 0)                                                                            \
  INJECT_PRINTF_OPTION(InjectPrintfLoads, 1)                                                                           \
  INJECT_PRINTF_OPTION(InjectPrintfStores, 2)                                                                          \
  INJECT_PRINTF_OPTION(InjectPrintfLoadsAndStores, 3)
#endif // INJECT_PRINTF_OPTION

#ifdef FILENAME_COLLISION_MODE
#define FILENAME_COLLISION_MODES                                                                                       \
  FILENAME_COLLISION_MODE(OVERRIDE, 0, "Override, the file will be the last one")                                      \
  FILENAME_COLLISION_MODE(LOG_SKIP, 1, "Log collisions and skip overriding, the file will be the first one with logs") \
  FILENAME_COLLISION_MODE(APPEND, 2, "Append whole content, the file will contain EVERY writes")
#endif // FILENAME_COLLISION_MODE


