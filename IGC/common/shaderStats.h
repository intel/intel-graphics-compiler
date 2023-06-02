/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//                  enumName                              stringName
//                  --------                              ----------
DEFINE_SHADER_STAT( STATS_ISA_INST_COUNT,                 "ISA count simd8"  )
DEFINE_SHADER_STAT( STATS_ISA_INST_COUNT_SIMD16,          "ISA count simd16" )
DEFINE_SHADER_STAT( STATS_ISA_INST_COUNT_SIMD32,          "ISA count simd32" )
DEFINE_SHADER_STAT( STATS_ISA_SPILL8,                     "simd8 spill"      )
DEFINE_SHADER_STAT( STATS_ISA_SPILL16,                    "simd16 spill"     )
DEFINE_SHADER_STAT( STATS_ISA_SPILL32,                    "simd32 spill"     )
DEFINE_SHADER_STAT( STATS_ISA_EARLYEXIT8,                 "simd8  early exit")
DEFINE_SHADER_STAT( STATS_ISA_EARLYEXIT16,                "simd16 early exit")
DEFINE_SHADER_STAT( STATS_ISA_EARLYEXIT32,                "simd32 early exit")
DEFINE_SHADER_STAT( STATS_ISA_CYCLE_ESTIMATE8,            "simd8 cycle estimate")
DEFINE_SHADER_STAT( STATS_ISA_CYCLE_ESTIMATE16,           "simd16 cycle estimate")
DEFINE_SHADER_STAT( STATS_ISA_CYCLE_ESTIMATE32,           "simd32 cycle estimate")
DEFINE_SHADER_STAT( STATS_ISA_STALL_ESTIMATE8,            "simd8 stall estimate")
DEFINE_SHADER_STAT( STATS_ISA_STALL_ESTIMATE16,           "simd16 stall estimate")
DEFINE_SHADER_STAT( STATS_ISA_STALL_ESTIMATE32,           "simd32 stall estimate")
DEFINE_SHADER_STAT(STATS_GRF_USED_SIMD8,                  "GRF used simd8")
DEFINE_SHADER_STAT(STATS_GRF_USED_SIMD16,                 "GRF used simd16")
DEFINE_SHADER_STAT(STATS_GRF_USED_SIMD32,                 "GRF used simd32")
DEFINE_SHADER_STAT(STATS_GRF_PRESSURE_SIMD8,              "GRF pressure estimate simd8")
DEFINE_SHADER_STAT(STATS_GRF_PRESSURE_SIMD16,             "GRF pressure estimate simd16")
DEFINE_SHADER_STAT(STATS_GRF_PRESSURE_SIMD32,             "GRF pressure estimate simd32")
DEFINE_SHADER_STAT(STATS_SAMPLE_BALLOT_LOOPS,             "sample ballot-loops")
DEFINE_SHADER_STAT( STATS_MAX_SHADER_STATS_ITEMS,         ""                 )
