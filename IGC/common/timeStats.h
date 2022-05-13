/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// We have two ways to measure pass time. For LLVM pass, we will create a
// module pass before and after the pass, while this will impact the pass
// manager on pass scheduling.  For IGC pass, we will measure it inside the
// pass.  For IGC pass, the timer need to be declared as coarse timer.  When
// adding pass timer, we will check that flag to avoid adding the timer pass.
//
// 'isCoarseTimer'    - indicate whether this stat will be shown in reduced display mode
// 'isDashBoardTimer' - indicate whether this stat will be shown in default display mode
//
//                enumName                                      stringName                                 parentEnum                          isVISATimer    isUnaccounted   isCoarseTimer   isDashBoardTimer
//                --------                                      ----------                                 ----------                          -----------    -------------   -------------   ----------------
DEFINE_TIME_STAT(  TIME_NONE,                                    "None",                                   MAX_COMPILE_TIME_INTERVALS,         false,         false,          false,          false )
DEFINE_TIME_STAT(  TIME_TOTAL,                                   "Total",                                  MAX_COMPILE_TIME_INTERVALS,         false,         false,          true,           true )
DEFINE_TIME_STAT(    TIME_ASMToLLVMIR,                           "ASMToLLVMIR",                            TIME_TOTAL,                         false,         false,          true,           true )
DEFINE_TIME_STAT(    TIME_OCL_LazyBiFLoading,                    "OCL LazyBiFLoading",                     TIME_TOTAL,                         false,         false,          true,           true )
DEFINE_TIME_STAT(    TIME_UnificationPasses,                     "UnificationPasses",                      TIME_TOTAL,                         false,         false,          true,           true )
DEFINE_TIME_STAT(      TIME_Unify_BuiltinImport,                 "UnifyBuiltinImport",                     TIME_UnificationPasses,             false,         false,          false,          true )
DEFINE_TIME_STAT(    TIME_OptimizationPasses,                    "OptimizationPasses",                     TIME_TOTAL,                         false,         false,          true,           true )
DEFINE_TIME_STAT(    TIME_CodeGen,                               "CodeGen",                                TIME_TOTAL,                         false,         false,          false,          true )
DEFINE_TIME_STAT(      TIME_CG_Add_Passes,                       "CodeGen Add Passes",                     TIME_CodeGen,                       false,         false,          false,          true )
DEFINE_TIME_STAT(        TIME_CG_Add_Legalization_Passes,        "CodeGen Add Legalization Passes",        TIME_CG_Add_Passes,                 false,         false,          false,          true )
DEFINE_TIME_STAT(        TIME_CG_Add_Analysis_Passes,            "CodeGen Add Analysis Passes",            TIME_CG_Add_Passes,                 false,         false,          false,          true )
DEFINE_TIME_STAT(        TIME_CG_Add_CodeGen_Passes,             "CodeGen Add CodeGen Passes",             TIME_CG_Add_Passes,                 false,         false,          false,          true )
DEFINE_TIME_STAT(      TIME_CG_Legalization,                     "CodeGen Legalization",                   TIME_CodeGen,                       false,         false,          true,           true )
DEFINE_TIME_STAT(      TIME_CG_Analysis,                         "CodeGen Analysis",                       TIME_CodeGen,                       false,         false,          true,           true )
DEFINE_TIME_STAT(      TIME_CG_SaveIR,                           "CodeGen SaveIR",                         TIME_CodeGen,                       false,         false,          true,           true )
DEFINE_TIME_STAT(      TIME_CG_RestoreIR,                        "CodeGen RestoreIR",                      TIME_CodeGen,                       false,         false,          true,           true )
DEFINE_TIME_STAT(      TIME_CG_vISACompile,                      "vISACompile (by IGC)",                   TIME_CodeGen,                       false,         false,          false,          true )
DEFINE_TIME_STAT(         TIME_VISA_TOTAL,                       "VISA Total",                             TIME_CG_vISACompile,                true,          false,          false,          true )
DEFINE_TIME_STAT(           TIME_VISA_BUILDER,                   "VISA Builder",                           TIME_VISA_TOTAL,                    true,          false,          true,           true )
DEFINE_TIME_STAT(           TIME_VISA_CFG,                       "VISA CFG",                               TIME_VISA_TOTAL,                    true,          false,          true,           true )
DEFINE_TIME_STAT(           TIME_VISA_OPTIMIZER,                 "VISA Optimizer",                         TIME_VISA_TOTAL,                    true,          false,          false,          true )
DEFINE_TIME_STAT(           TIME_VISA_HW_CONFORMITY,             "VISA HW Conformity",                     TIME_VISA_TOTAL,                    true,          false,          true,           true )
DEFINE_TIME_STAT(           TIME_VISA_MISC_OPTS,                 "VISA Misc opts",                         TIME_VISA_TOTAL,                    true,          false,          false,          true )
DEFINE_TIME_STAT(           TIME_VISA_TOTAL_RA,                  "VISA Total RA",                          TIME_VISA_TOTAL,                    true,          false,          true,           true )
DEFINE_TIME_STAT(             TIME_VISA_ADDR_FLAG_RA,            "VISA Addr Flag RA",                      TIME_VISA_TOTAL_RA,                 true,          false,          false,          true )
DEFINE_TIME_STAT(             TIME_VISA_LOCAL_RA,                "VISA Local RA",                          TIME_VISA_TOTAL_RA,                 true,          false,          false,          true )
DEFINE_TIME_STAT(             TIME_VISA_HYBRID_RA,               "VISA Hybrid RA",                         TIME_VISA_TOTAL_RA,                 true,          false,          false,          true )
DEFINE_TIME_STAT(             TIME_VISA_LINEARSCAN_RA,           "VISA Linear Scan RA",                    TIME_VISA_TOTAL_RA,                 true,          false,          false,          true )
DEFINE_TIME_STAT(             TIME_VISA_GRF_GLOBAL_RA,           "VISA GRF Global RA0",                    TIME_VISA_TOTAL_RA,                 true,          false,          false,          true )
DEFINE_TIME_STAT(               TIME_VISA_INTERFERENCE,          "VISA Interference",                      TIME_VISA_GRF_GLOBAL_RA,            true,          false,          false,          false )
DEFINE_TIME_STAT(               TIME_VISA_COLORING,              "VISA Coloring",                          TIME_VISA_GRF_GLOBAL_RA,            true,          false,          false,          false )
DEFINE_TIME_STAT(               TIME_VISA_SPILL,                 "VISA Spill",                             TIME_VISA_GRF_GLOBAL_RA,            true,          false,          false,          true )
DEFINE_TIME_STAT(           TIME_VISA_PRERA_SCHEDULING,          "VISA PreRA Scheduling",                  TIME_VISA_TOTAL,                    true,          false,          true,           true )
DEFINE_TIME_STAT(           TIME_VISA_SCHEDULING,                "VISA Scheduling",                        TIME_VISA_TOTAL,                    true,          false,          true,           true )
DEFINE_TIME_STAT(           TIME_VISA_ENCODE_AND_EMIT,           "VISA Encode and Emit",                   TIME_VISA_TOTAL,                    true,          false,          true,           true )
DEFINE_TIME_STAT(             TIME_VISA_ENCODE_COMPACTION,       "VISA Encode Compaction",                 TIME_VISA_ENCODE_AND_EMIT,          true,          false,          false,          true )
DEFINE_TIME_STAT(             TIME_VISA_IGA_ENCODER,             "VISA IGA Encoding",                      TIME_VISA_ENCODE_AND_EMIT,          true,          false,          false,          true )
DEFINE_TIME_STAT(           TIME_VISA_BUILDER_APPEND_INST,       "VISA Builder Append Instruction",        TIME_VISA_BUILDER,                    true,          false,          false,          true )
DEFINE_TIME_STAT(           TIME_VISA_BUILDER_CREATE_VAR,        "VISA Builder Create Var",                TIME_VISA_BUILDER,                    true,          false,          false,          true )
DEFINE_TIME_STAT(           TIME_VISA_BUILDER_CREATE_OPND,       "VISA Builder Create Operand",            TIME_VISA_BUILDER,                    true,          false,          false,          true )
DEFINE_TIME_STAT(           TIME_VISA_BUILDER_IR_CONSTRUCTION,   "VISA Builder IR Construction",           TIME_VISA_BUILDER,                    true,          false,          false,          true )
DEFINE_TIME_STAT(             TIME_VISA_Liveness,                "VISA Liveness",                          TIME_VISA_TOTAL_RA,                 true,          false,          false,          false )
DEFINE_TIME_STAT(             TIME_VISA_RPE,                     "VISA Reg Pressure Estimate",             TIME_VISA_TOTAL_RA,                 true,          false,          false,          false )
DEFINE_TIME_STAT(           TIME_VISA_Unaccounted,               "VISA Total Unaccounted",                 TIME_VISA_TOTAL,                    false,         true,           false,          true )
DEFINE_TIME_STAT(         TIME_vISACompile_Unaccounted,          "vISACompile Unaccounted",                TIME_CG_vISACompile,                false,         true,           false,          true )
DEFINE_TIME_STAT(      TIME_CG_Unaccounted,                      "CodeGen Unaccounted",                    TIME_CodeGen,                       false,         true,           false,          true )
DEFINE_TIME_STAT(    TIME_RT_SETUP,                              "Raytracing Setup Total",                 TIME_TOTAL,                         false,         false,          true,           true )
DEFINE_TIME_STAT(      TIME_RT_LINKING,                          "Raytracing Linking",                     TIME_RT_SETUP,                      false,         false,          true,           true )
DEFINE_TIME_STAT(      TIME_RT_PASSES,                           "Raytracing Passes",                      TIME_RT_SETUP,                      false,         false,          true,           true )
DEFINE_TIME_STAT(    TIME_TOTAL_Unaccounted,                     "Total Unaccounted",                      TIME_TOTAL,                         false,         true,           false,          true )

// This must be the last one in the list
DEFINE_TIME_STAT( MAX_COMPILE_TIME_INTERVALS,                    "",                                       MAX_COMPILE_TIME_INTERVALS,         false,         false,          false,          false )
