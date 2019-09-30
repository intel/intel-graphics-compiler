/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#pragma once
#include "iStdLib/types.h"

namespace USC
{

// Forward declarations
class CShader;

/*****************************************************************************\
MACRO: GHAL3D_OPTIMIZATION_STAT_INCREMENT
If Optimization Stats are enabled, this macro increments given stat by value.

Input:
    pointer - pointer to stats struct.
    stat    - name of stat to be incremented
    value   - amount of incrementation
\*****************************************************************************/
#ifndef GHAL3D_OPTIMIZATION_STAT_INCREMENT

#if defined(_DEBUG) || GHAL3D_OPTIMIZATION_STATS_ENABLE
#define GHAL3D_OPTIMIZATION_STAT_INCREMENT(pointer, stat, value) \
    if( g_DebugVariables.OptimizationStatsEnable && pointer != NULL ) \
    { \
        pointer->OptCounter[stat] += value; \
    }
#else
#define GHAL3D_OPTIMIZATION_STAT_INCREMENT(pointer, stat, value)
#endif

#endif //GHAL3D_OPTIMIZATION_STAT_INCREMENT


/*****************************************************************************\
MACRO: RESET_OPT_STATS
\*****************************************************************************/
#ifndef RESET_SHADER_COUNTERS
#if GHAL3D_OPTIMIZATION_STATS_ENABLE
#define RESET_SHADER_COUNTERS( pointer, resetCommon, resetPerKernel ) \
    if( g_DebugVariables.OptimizationStatsEnable && pointer != NULL ) \
    { \
        pointer->Reset( resetCommon, resetPerKernel ); \
    }
#else // GHAL3D_OPTIMIZATION_STATS_ENABLE
#define RESET_SHADER_COUNTERS( pointer, resetILStats, resetIsaStats )
#endif // GHAL3D_OPTIMIZATION_STATS_ENABLE
#endif //RESET_OPT_STATS

/*****************************************************************************\
MACRO: DUMP_AND_RESET_OPT_STATS
\*****************************************************************************/
#ifndef DUMP_AND_RESET_SHADER_COUNTERS
#if GHAL3D_OPTIMIZATION_STATS_ENABLE
#define DUMP_AND_RESET_SHADER_COUNTERS( shader, simd, succesfulCompilation ) \
    if( g_DebugVariables.OptimizationStatsEnable && \
        shader != NULL && shader->GetOptStats() ) \
    { \
        shader->GetOptStats()->SIMD = simd; \
        shader->DumpAndResetShaderCounters( succesfulCompilation ); \
    }
#else // GHAL3D_OPTIMIZATION_STATS_ENABLE
#define DUMP_AND_RESET_SHADER_COUNTERS( shader, simd, succesfulCompilation )
#endif // GHAL3D_OPTIMIZATION_STATS_ENABLE
#endif //DUMP_AND_RESET_SHADER_COUNTERS

#if GHAL3D_OPTIMIZATION_STATS_ENABLE

/*****************************************************************************\
ENUM: STATS_SIMD_TYPE
Different SIMD types used by ShaderStats. Note - SIMD4 is used for all CP
kernels.
\*****************************************************************************/
enum STATS_SIMD_TYPE
{
    STATS_SIMD_4,           // Used for all CP kernels.
    STATS_SIMD_8,
    STATS_SIMD_16,
    STATS_SIMD_32,
    NUM_STATS_SIMD_TYPES,
};

extern const DWORD g_cStatsSimdTypeValues[];

extern const char* g_cShaderTypeNames[];

/*****************************************************************************\
ENUM: STAT_COUNTER_GROUP
Group of counters - different groups are disjoint sets and can be tracked 
separately. This enum has according table - g_cStatCounterGroupNames defined
in ShaderStats.cpp.
\*****************************************************************************/
enum STAT_COUNTER_GROUP
{
    SCG_NO_CNT,         // Special value - not a counter and shouldn't be summarized
    SCG_OPTIMIZATION,   // Standard optimization counter
    SCG_COMPILATION,    // Compilation statistic
    SCG_ISA,            // Isa statistic, both optimization and instruction count
    SCG_INPUT,          // IL information
    SCG_CUSTOM,         // Shouldn't be used in g_cOptimizationStat, special map designed for it.
};

extern const char* g_cStatCounterGroupNames[];

/*****************************************************************************\
ENUM: OPTIMIZATION_COUNTER_TYPE
Types of conuters - this enum has also according table - g_cOptimizationStat
defined in ShaderStats.cpp. It should be updated whenever this enum is updated.
\*****************************************************************************/
enum OPTIMIZATION_COUNTER_TYPE
{
    OCT_IL_PRECOMPILE_INSTRUCTIONS,
    OCT_IL_LOW_PREC_INSTRUCTIONS,
    OCT_LOOPS_UNROLLED,
    OCT_LOOPS_GATED,
    OCT_CB_READS_OPTIMIZED,
    OCT_INDIRECT_TEMPS_REMOVED,
    OCT_TRIVIAL_SWITCHES_REMOVED,
    OCT_TRIVIAL_IFS_REMOVED,
    OCT_IL_PATTERN_MATCHED,
    OCT_IFS_CONVERTED,
    OCT_ILCF_INDEXES_INLINED,
    OCT_INDEXED_TEMP_GRF_CACHED,
    OCT_GRAPH_COLORING,
    OCT_LINEAR_SCAN,
    OCT_TREE_SCAN,
    OCT_TREE_SCAN_ROLLBACK,
    OCT_CHANNELS_PROPAGATED,
    OCT_DP_JOINS_SPLITS,
    OCT_INSERTED_CONVERT_32_TO_64,
    OCT_SPLIT_ALU_64_OP_TO_32,    
    OCT_REDUCE_ALU_64_OP_TO_32,
    OCT_REDUCE_ALU_32_OP_TO_8,
    OCT_REDUCE_ALU_32_OP_SRC_TO_8,    
    OCT_FOLDED_UNPACKS,
    OCT_HOISTED_UNPACKS,
    OCT_MOVS_PROPAGATED,
    OCT_BITCAST_PROPAGATED,
    OCT_COND_MOD_PROPAGATED,
    OCT_IMM2CB_MOVS,
    OCT_IMM2CB_PAYLOAD,
    OCT_SIMDS_REDUCED,
    OCT_DEPENDENCY_BREAK_MOVS_INSERTED,
    OCT_BLOCK_LOADS,
    OCT_DUAL_LOADS,
    OCT_RESOURCE_LOADS,
    OCT_MERGED_CONSTANT_BUFFER_LOADS,
    OCT_SCALAR_ATOMICS,
    OCT_MEDIA_BLOCK_READ_PACK_PATTERN_MATCHED,
    OCT_AVERAGE_PATTERN_MACTHED,
    OCT_PROPAGATE_REDUNDANT_PACK_MATCHED,
    OCT_PROPAGATE_LOW_PREC,
    OCT_HOIST_SAT_MATCHED,
    OCT_CHANNEL_PATTERN_MATCHED,
    OCT_POWER_PATTERN_MATCHED,
    OCT_EUBYPASS_PATTERN_MATCHED,
    OCT_COMPARISON_PATTERN_MATCHED,
    OCT_FLOWCONTROL_PATTERN_MATCHED,
    OCT_MULTIPLY_PATTERN_MATCHED,
    OCT_VECIMMSCALAR_PATTERN_MATCHED,
    OCT_SQRT_PATTERN_MATCHED,
    OCT_FDIV_PATTERN_MATCHED,
    OCT_SELECT_PATTERN_MATCHED,
    OCT_MINMAX_PATTERN_MATCHED,
    OCT_MULDIV_PATTERN_MATCHED,
    OCT_MUL_MAD_PATTERN_MATCHED_3,
    OCT_FDP3_PATTERN_MATCHED,
    OCT_FDP4TOH_PATTERN_MATCHED,
    OCT_MOV0FDP_PATTERN_MATCHED,
    OCT_MAD_PATTERN_MATCHED,
    OCT_INT_MAD_PATTERN_MATCHED,
    OCT_BFE_PATTERN_MATCHED,
    OCT_LRP_PATTERN_MATCHED,
    OCT_TRIVIAL_LRP_PATTERN_MATCHED,
    OCT_MOVLRP_TO_ADDMAD_PATTERN_MATCHED,
    OCT_BFI_PATTERN_MATCHED,
    OCT_SHR_SHL_PATTERN_MATCHED,
    OCT_ADD_SHL_PATTERN_MATCHED,
    OCT_ADD_ADD_PATTERN_MATCHED,
    OCT_SHR_AND_PATTERN_MATCHED,
    OCT_AND_SHIFT_PATTERN_MATCHED,
    OCT_LADD_PATTERN_MATCHED,
    OCT_JOINDP_PATTERN_MATCHED,
    OCT_GET_VALUE_FROM_ACTIVE_CHANNEL_PATTERN_MATCHED,
    OCT_REPLICATECOMPONENT_PATTERN_MATCHED,
    OCT_CSEL_PATTERN_MATCHED,
    OCT_FDPH_PATTERN_MATCHED,
    OCT_PACK_PATTERN_MATCHED,
    OCT_FFRC_PATTERN_MATCHED,
    OCT_MUL_STRENGTH_REDUCED_PATTERN_MATCHED,
    OCT_REDUNDANT_PHI_PATTERN_MATCHED,    
    OCT_ATOMIC_DST_REMOVAL_PATTERN_MATCHED,
    OCT_CONSTANTS_PROPAGATION_PATTERN_MATCHED,
    OCT_POW_UNWIND_PATTERN_MATCHED,
    OCT_MUL_ADD_TO_MUL_PATTERN_MATCHED,
    OCT_MOV_TWO_LOW_PREC_IMM_PATTERN_MATCHED,
    OCT_INT_CONVERT_TO_BITCAST_PATTERN_MATCHED,
    OCT_MUL_CMP_PATTERN_MATCHED,
    OCT_MOV_SEL_ADD_MAD_PATTERN_MATCHED,
    OCT_LOOP_INVARIANT_PATTERN_MATCHED,
    OCT_DEAD_BBLOCKS,
    OCT_LLIR_INST_IN_DEAD_BLOCKS,
    OCT_OPTIMIZED_REPLICATES,
    OCT_CONSTANTS_FOLDED,
    OCT_REG_COPY_PROPAGATED,
    OCT_INDUC_VARS_ELIMINATED,
    OCT_VALUES_NUMBERED,
    OCT_VN_CANDIDATES,
    OCT_VN_HASH_MISSES,
    OCT_COALESCED_THREAD_LDS,
    OCT_COALESCED_LOADS,
    OCT_COALESCED_STORES,
    OCT_MERGE_LOAD_BUFFER_LOADS,
    OCT_OPTIMIZE_SIMD8_MOVS,
    OCT_EMPTY_GOTO,
    OCT_REDUNDANT_JOIN,
    OCT_GOTO_AROUND_GOTO,
    OCT_DEAD_JOIN,
    OCT_BITCASTS_INSERTED,
    OCT_LOW_PREC_MATCHED,
    OCT_LOW_PREC_CONVERTING_MOVS_HIGH_TO_LOW,
    OCT_LOW_PREC_CONVERTING_MOVS_LOW_TO_HIGH,
    OCT_LOW_PREC_CONVERTING_MOVS_PACKING,
    OCT_LOW_PREC_CONVERTING_MOVS_UNPACKING,
    OCT_LOW_PREC_VALUES_PROCESSED,
    OCT_LOW_PREC_ANY_INSTRUCTIONS_BEFORE_PROCESSING,
    OCT_LOW_PREC_ANY_INSTRUCTIONS,
    OCT_LOW_PREC_ALL_INSTRUCTIONS,
    OCT_LOW_PREC_ALL_PACKED_INSTRUCTIONS,
    OCT_LOW_PREC_OPERANDS,
    OCT_LOW_PREC_DESTINATIONS,
    OCT_LOW_PREC_PACKED_OPERANDS,
    OCT_LOW_PREC_PACKED_DESTINATIONS,
    OCT_DISCARDS_OPTIMIZED,
    OCT_CODE_SINKING_COUNT,
    OCT_LOOP_INVARIANT_CODE_MOTION_COUNT,
    OCT_REORDERED_INSTRUCTIONS,
    OCT_SAMPLES_CLUSTERED,
    OCT_PRESCHED_CYCLES_SAVED,
    OCT_PRESCHED_CYCLES_ORIGINAL,
    OCT_PRESCHED_INSTRUCTIONS,
    //OCT_ALLOCATE_ACCUMULATOR,
    OCT_PRUNING_CS,
    OCT_PRUNING_CP,
    OCT_COALESCED_COPIES,
    OCT_COALESCED_SPLITS,
    OCT_COALESCED_JOINS,
    OCT_COALESCED_PREDICATES,
    OCT_COALESCED_GENERICS,
    OCT_COALESCED_COPIES_1,
    OCT_COALESCED_SPLITS_1,
    OCT_COALESCED_JOINS_1,
    OCT_COALESCED_PREDICATES_1,
    OCT_COALESCED_GENERICS_1,
    OCT_COALESCED_COPIES_2,
    OCT_COALESCED_SPLITS_2,
    OCT_COALESCED_JOINS_2,
    OCT_COALESCED_PREDICATES_2,
    OCT_COALESCED_GENERICS_2,
    OCT_COALESCED_COPIES_3,
    OCT_COALESCED_SPLITS_3,
    OCT_COALESCED_JOINS_3,
    OCT_COALESCED_PREDICATES_3,
    OCT_COALESCED_GENERICS_3,
    OCT_GRF_BANKS_ALIGNED,
    OCT_GRF_BANKS_NOT_ALIGNED,
    OCT_GRF_BANKS_NOT_MOVABLE,
    OCT_REG_PRESSURE,
    OCT_REG_PRESSURE_DIFF,
    OCT_SKIP_SIMPLIFY,
    OCT_CAC_SUCCESSFUL,
    OCT_DECREASE_GRF_PRESSURE_COUNT,
    OCT_PRESET_DECREASED_GRF_PRESSURE,
    OCT_CUT_LIVE_RANGES,
    OCT_LIR_GLOBAL_SCALAR_INSTRUCTIONS,
    OCT_LIR_LOCAL_SCALAR_INSTRUCTIONS,
    OCT_LIR_VECTOR_INSTRUCTIONS,
    OCT_LIR_UNKNOWN_SCALAR_INSTRUCTIONS,
    OCT_LIR_SPILL_FILL_COUNT,
    OCT_SPILL_FILL_COUNT,
    OCT_SPILL_COST,
    OCT_REGALLOC_BUILD,
    OCT_REGALLOC_COALESCE,
    OCT_TREESCAN_COMPACTION_COUNT,
    OCT_TREESCAN_COMPACTION_COPIES,
    OCT_TREESCAN_COALESCE_MAX,
    OCT_TREESCAN_COALESCED_COUNT,
    OCT_TREESCAN_MAX_AFFINITY_GROUP,
    OCT_TREESCAN_PHI_COLOR_SUCCESS,
    OCT_TREESCAN_PHI_COLOR_FAIL,
    OCT_TREESCAN_INTERVAL_SPLIT,
    OCT_TREESCAN_INTERVAL_SPLIT_PHIS,
    OCT_TREESCAN_PHI_POOL_CHANGE_GRF,
    OCT_TREESCAN_PHI_POOL_CHANGE_MEMORY,
    OCT_TREESCAN_RESERVATION_SPILL,
    OCT_TREESCAN_PHI_CYCLES,
    OCT_TREESCAN_PHI_CYCLE_COPIES,    
    OCT_CFG_LIVENESS_ITERATIONS,
    OCT_STRUCTURAL_ANALYSIS_ITERATIONS,
    OCT_ISA_CYCLES_SAVED,
    OCT_ISA_CYCLES_ORIGINAL,
    OCT_ISA_INSTRUCTIONS,
    OCT_DROP_FAILED_COMPILATION,
    OCT_DROP_OUT_OF_MEMORY,
    OCT_DROP_LOWER_SIMD_NOT_COMPILED,
    OCT_DROP_SCRATCH_REG_COUNT,
    OCT_DROP_REGISTER_PRESSURE,
    OCT_DROP_UNSUPPORTED_INSTRUCTION,
    OCT_DROP_INSTRUCTION_COUNT,
    OCT_DROP_INDIRECT_INPUTS,
    OCT_DROP_RENDER_CACHE_SIZE,
    OCT_DROP_THREAD_GROUP_SIZE,
    OCT_DROP_FUNCTIONS,
    OCT_DROP_SPILL_FILL,
    OCT_DROP_SPILL_FILL_PREDICTED,
    OCT_PROMOTE_TPM,
    OCT_SHADER_INPUT_PACKING,
    OCT_POINTER_SLM,
    OCT_POINTER_NON_SLM,
    OCT_POINTER_UNKNOWN,
    OCT_POINTER_BYTE_SCATTERED_READWRITE_BYTE,
    OCT_POINTER_BYTE_SCATTERED_READWRITE_WORD,
    OCT_POINTER_BYTE_SCATTERED_READWRITE_DWORD,
    OCT_POINTER_UNTYPED_READWRITE_SCALAR,
    OCT_POINTER_UNTYPED_READWRITE_VEC2,
    OCT_POINTER_UNTYPED_READWRITE_VEC3,
    OCT_POINTER_UNTYPED_READWRITE_VEC4,
    OCT_POINTER_BLOCK_READ,
    OCT_POINTER_ATOMICS,    

    OCT_CODE_STATS_MARKER_DO_NOT_MOVE,

    OCT_DEBUG_ISA_ILLEGAL_INSTRUCTIONS,
    OCT_DEBUG_ISA_MOV_INSTRUCTIONS,
    OCT_DEBUG_ISA_SEL_INSTRUCTIONS,
    OCT_DEBUG_ISA_NOT_INSTRUCTIONS,
    OCT_DEBUG_ISA_AND_INSTRUCTIONS,
    OCT_DEBUG_ISA_OR_INSTRUCTIONS,
    OCT_DEBUG_ISA_XOR_INSTRUCTIONS,
    OCT_DEBUG_ISA_SHL_INSTRUCTIONS,
    OCT_DEBUG_ISA_SHR_INSTRUCTIONS,
    OCT_DEBUG_ISA_DIM_INSTRUCTIONS,
    OCT_DEBUG_ISA_SMOV_INSTRUCTIONS,
    OCT_DEBUG_ISA_ASR_INSTRUCTIONS,
    OCT_DEBUG_ISA_CMP_INSTRUCTIONS,
    OCT_DEBUG_ISA_CMPN_INSTRUCTIONS,
    OCT_DEBUG_ISA_CSEL_INSTRUCTIONS,
    OCT_DEBUG_ISA_F32TO16_INSTRUCTIONS,
    OCT_DEBUG_ISA_F16TO32_INSTRUCTIONS,
    OCT_DEBUG_ISA_BFREV_INSTRUCTIONS,
    OCT_DEBUG_ISA_BFE_INSTRUCTIONS,
    OCT_DEBUG_ISA_BFI1_INSTRUCTIONS,
    OCT_DEBUG_ISA_BFI2_INSTRUCTIONS,
    OCT_DEBUG_ISA_JMPI_INSTRUCTIONS,
    OCT_DEBUG_ISA_BRD_INSTRUCTIONS,
    OCT_DEBUG_ISA_IF_INSTRUCTIONS,
    OCT_DEBUG_ISA_BRC_INSTRUCTIONS,
    OCT_DEBUG_ISA_ELSE_INSTRUCTIONS,
    OCT_DEBUG_ISA_ENDIF_INSTRUCTIONS,
    OCT_DEBUG_ISA_CASE_INSTRUCTIONS,
    OCT_DEBUG_ISA_WHILE_INSTRUCTIONS,
    OCT_DEBUG_ISA_BREAK_INSTRUCTIONS,
    OCT_DEBUG_ISA_CONTINUE_INSTRUCTIONS,
    OCT_DEBUG_ISA_HALT_INSTRUCTIONS,
    OCT_DEBUG_ISA_CALLA_INSTRUCTIONS,
    OCT_DEBUG_ISA_CALL_INSTRUCTIONS,
    OCT_DEBUG_ISA_RET_INSTRUCTIONS,
    OCT_DEBUG_ISA_FORK_INSTRUCTIONS,
    OCT_DEBUG_ISA_GOTO_INSTRUCTIONS,
    OCT_DEBUG_ISA_JOIN_INSTRUCTIONS,
    OCT_DEBUG_ISA_WAIT_INSTRUCTIONS,
    OCT_DEBUG_ISA_SEND_INSTRUCTIONS,
    OCT_DEBUG_ISA_SENDC_INSTRUCTIONS,
    OCT_DEBUG_ISA_MATH_INSTRUCTIONS,
    OCT_DEBUG_ISA_ADD_INSTRUCTIONS,
    OCT_DEBUG_ISA_MUL_INSTRUCTIONS,
    OCT_DEBUG_ISA_AVG_INSTRUCTIONS,
    OCT_DEBUG_ISA_FRC_INSTRUCTIONS,
    OCT_DEBUG_ISA_RNDU_INSTRUCTIONS,
    OCT_DEBUG_ISA_RNDD_INSTRUCTIONS,
    OCT_DEBUG_ISA_RNDE_INSTRUCTIONS,
    OCT_DEBUG_ISA_RNDZ_INSTRUCTIONS,
    OCT_DEBUG_ISA_MAC_INSTRUCTIONS,
    OCT_DEBUG_ISA_MACH_INSTRUCTIONS,
    OCT_DEBUG_ISA_LZD_INSTRUCTIONS,
    OCT_DEBUG_ISA_FBH_INSTRUCTIONS,
    OCT_DEBUG_ISA_FBL_INSTRUCTIONS,
    OCT_DEBUG_ISA_CBIT_INSTRUCTIONS,
    OCT_DEBUG_ISA_ADDC_INSTRUCTIONS,
    OCT_DEBUG_ISA_SUBB_INSTRUCTIONS,
    OCT_DEBUG_ISA_SAD2_INSTRUCTIONS,
    OCT_DEBUG_ISA_SADA2_INSTRUCTIONS,
    OCT_DEBUG_ISA_DP4_INSTRUCTIONS,
    OCT_DEBUG_ISA_DPH_INSTRUCTIONS,
    OCT_DEBUG_ISA_DP3_INSTRUCTIONS,
    OCT_DEBUG_ISA_DP2_INSTRUCTIONS,
    OCT_DEBUG_ISA_LINE_INSTRUCTIONS,
    OCT_DEBUG_ISA_PLANE_INSTRUCTIONS,
    OCT_DEBUG_ISA_MAD_INSTRUCTIONS,
    OCT_DEBUG_ISA_LRP_INSTRUCTIONS, 
    OCT_DEBUG_ISA_MADM_INSTRUCTIONS, 
    OCT_DEBUG_ISA_NOP_INSTRUCTIONS,

    MAX_OPTIMIZATION_COUNTER_TYPE,
};

/*****************************************************************************\
STRUCT: SShaderCounterInfo

Description:
Information about single ShaderCounter.
\*****************************************************************************/
struct SShaderCounterInfo
{
    const char*         Name;
    STAT_COUNTER_GROUP  Group;
    bool                PerShader;
    const char*         Description;
};

extern const SShaderCounterInfo g_cOptimizationStat[];


/*****************************************************************************\
STRUCT: SOptStats

Description:
Used when optimization statistics are enabled.
\*****************************************************************************/
struct SOptStats
{
    // Should be set after first dump of full successfull compilation with all
    // per-shader and per-kernel stats (and resetting per-shader data).
    bool  perShaderDumped;

    // In which SIMD shader was compiled.
    STATS_SIMD_TYPE SIMD;

    DWORD OptCounter[MAX_OPTIMIZATION_COUNTER_TYPE];
    DWORD FailedOptCounter[MAX_OPTIMIZATION_COUNTER_TYPE];

    void Reset( bool resetCommon, bool resetPerKernel );
};

#endif // GHAL3D_OPTIMIZATION_STATS_ENABLE

#if USC_QUALITY_METRICS_ENABLE
/*****************************************************************************\
STRUCT: SQualityMetrics

Description:
Used to quantify compiler output quality.
\*****************************************************************************/
struct SQualityMetrics
{
    DWORD   m_NumInstAsm;
    DWORD   m_NumInstAsmMOV;
    DWORD   m_NumInstAsmIO;
    DWORD   m_NumInstAsmALU; 
    DWORD   m_NumInstAsmCTL; 

    DWORD   m_NumInstIr;
    DWORD   m_NumInstIrMOV;
    DWORD   m_NumInstIrIO;
    DWORD   m_NumInstIrALU; 
    DWORD   m_NumInstIrCTL; 

    DWORD   m_NumDeclaredInputs;
    DWORD   m_MaxDeclaredInput;
    DWORD   m_BitSetUsedInputs; 
    DWORD   m_NumInputChannels;

    DWORD   m_NumExpectedInstIsaInCP;
    DWORD   m_NumExpectedInstIsaMOVinCP;
    DWORD   m_NumExpectedInstIsaIOinCP;
    DWORD   m_NumExpectedInstIsaALUinCP; 
    DWORD   m_NumExpectedInstIsaCTLinCP; 

    DWORD   m_NumExpectedInstIsaInCS;
    DWORD   m_NumExpectedInstIsaMOVinCS;
    DWORD   m_NumExpectedInstIsaIOinCS;
    DWORD   m_NumExpectedInstIsaALUinCS; 
    DWORD   m_NumExpectedInstIsaCTLinCS; 

    DWORD   m_NumInstIsa;
    DWORD   m_NumInstIsaMOV;
    DWORD   m_NumInstIsaMOVSimd8;
    DWORD   m_NumInstIsaIO;
    DWORD   m_NumInstIsaIOSimd8;    
    DWORD   m_NumInstIsaIOSimd8Required;    // required simd8, subset of IO
    DWORD   m_NumInstIsaCTL; 
    DWORD   m_NumInstIsaCTLSimd8; 
    DWORD   m_NumInstIsaALU; 
    DWORD   m_NumInstIsaALUSimd8; 
    DWORD   m_NumInstIsaALUProlog;              // subset of ALU
    DWORD   m_NumInstIsaTernary;                // subset of ALU
    DWORD   m_NumInstIsaTernarySimd8;           // subset of Ternary
    DWORD   m_NumInstIsaTernaryBankCollision;   // subset of Ternary
    DWORD   m_NumInstIsaMATH;                   // subset of ALU
    DWORD   m_NumInstIsaMATHSimd8;              // subset of MATH
    DWORD   m_NumInstIsaCoissue;                // subset of ALU
    
    DWORD   m_ExecutionSize; 
    DWORD   m_NumScratchRegisters; 
    DWORD   m_NumConcurrentLiveRangesSimd8; 
    DWORD   m_NumConcurrentLiveRangesSimd16; 
    DWORD   m_NumConcurrentLiveRangesSimd32;
    DWORD   m_CyclesNonScheduled;
    DWORD   m_CyclesScheduled;
    DWORD   m_CyclesExecuted;

    DWORD   m_UsedOutputsMask;

    DWORD   m_NumQualityMetricsUpdates;

    void    AverageQualityMetrics();
};

/*****************************************************************************\
STRUCT: SQualityMetricsExpectedIsa

Description:
Used to tabularize expected ISA instruction count / type for input ASM.
\*****************************************************************************/
struct SQualityMetricsExpectedIsa
{
    DWORD   m_Opcode;                   // DX asm or IL opcode 
    DWORD   m_NumExpectedInstIsaMOV;
    DWORD   m_NumExpectedInstIsaALU; 
    DWORD   m_NumExpectedInstIsaCTL; 
    DWORD   m_NumExpectedInstIsaIO;
    DWORD   m_ChannelSerialConst;       // # of instr regardless of enabled channels
    float   m_ChannelSerialMultiplier;  // # of instr per one enabled channel
};

/*****************************************************************************\
STRUCT: SQualityMetricsIsa

Description:
Used to tabularize actual ISA instruction count / type for kernel stream.
\*****************************************************************************/
struct SQualityMetricsActualIsa
{
    DWORD   m_Opcode;                   // ISA opcode
    DWORD   m_NumInstIsaALU; 
    DWORD   m_NumInstIsaCTL; 
    DWORD   m_NumInstIsaIO;
};
#endif // USC_QUALITY_METRICS_ENABLE

#if USC_MEMORY_STATS_ENABLE

enum SHADER_MEMORY_ALLOCS_TYPE
{
    SMAT_TINY = 0,
    SMAT_SMALL,
    SMAT_MEDIUM,
    SMAT_BIG,
    SMAT_LARGE,
    SMAT_NUM_OF_TYPES
};

struct SShaderMemoryAllocsType
{
    const char* Name;
    size_t max_size;
};

/*****************************************************************************\
g_cShaderMemoryAllocsType
Debug texts and sizes for different types of memory allocation sizes.
\*****************************************************************************/
static const SShaderMemoryAllocsType g_cShaderMemoryAllocsType[] = 
{
    {"Size < 4B", 4},
    {"Size < 32B", 32},
    {"Size < 256B", 256},
    {"Size < 1kB", 1024},
    {"Size > 1kB", size_t(~0)}
};

enum SHADER_MEMORY_BUCKET
{
    SMS_GLOBAL_SUMMARY = 0,
    SMS_API_COMPILER,
    SMS_IL_PRECOMPILE,
    SMS_IL_OPTIMIZATIONS,
    SMS_IL_COMPILER,
    SMS_DECOMPOSER,
    SMS_VR2R_CREATE_REG_INST_MAPPING,
    SMS_OPTIMIZER_ANALYSIS,
    SMS_VR2R_CHANNEL_PROPAGATION,
    SMS_VR2R_DETECT_SCALARS,
    SMS_VR2R_DO_SIMD_REDUCTION,
    SMS_VR2R_OPTIMIZE_INDIRECT_CONSTANT_BUFFER_LOADS,
    SMS_VR2R_OPTIMIZE_RESOURCE_LOADS,
    SMS_VR2R_MERGE_REGISTER_COPIES,
    SMS_VR2R_MOV_PROPAGATION,
    SMS_VR2R_COND_MOD_PROPAGATION,
    SMS_VR2R_KILL_AFTER_DISCARD,
    SMS_VR2R_PATTERN_MATCH_REPLACE,
    SMS_VR2R_DEAD_BRANCH_REMOVAL,
    SMS_VR2R_OPTIMIZE_CONSTANT_BUFFER_LOADS_1,
    SMS_VR2R_OPTIMIZE_REPLICATE_INSTRUCTIONS_0,
    SMS_VR2R_CONSTANT_FOLDING,
    SMS_VR2R_MEMORY_ALIASING,
    SMS_VR2R_VALUE_NUMBERING,
    SMS_VR2R_IMM2CR,
    SMS_VR2R_SIMD_EXPANSION,
    SMS_VR2R_OPTIMIZE_REPLICATE_INSTRUCTIONS_1,
    SMS_VR2R_OPTIMIZE_CONSTANT_BUFFER_LOADS_2,
    SMS_VR2R_OPTIMIZE_CONSTANT_BUFFER_LOADS_3,
    SMS_VR2R_OPTIMIZE_REPLICATE_INSTRUCTIONS_2,
    SMS_VR2R_MERGE_LOAD_BUFFER_INSTRUCTION,
    SMS_VR2R_REORDER_INSTRUCTIONS_1,
    SMS_VR2R_TPM_PROMOTION,
    SMS_VR2R_CODE_SINKING,
    SMS_VR2R_LOOP_INVARIANT_CODE_MOTION,
    SMS_OPTIMIZER_PASSES,
    SMS_VR2R_REGION_PRE_REGALLOC_SCHEDULING_1,
    SMS_VR2R_RENUMBER,
    SMS_VR2R_SPILL_ALLOCATE_LIVE_RANGES,
    SMS_VR2R_ALLOCATE_INTERFERENCE,
    SMS_VR2R_BUILD_INTERFERENCE,
    SMS_VR2R_RENAME_WITH_LIVERANGES,
    SMS_VR2R_FREE_LIVE_RANGES,
    SMS_VR2R_SPILL_COST,
    SMS_VR2R_SETUP_GRF_BANK_ALIGNMENT,
    SMS_VR2R_COLOR_GRAPH,
    SMS_VR2R_RENAME_WITH_REGISTERS,
    SMS_VR2R_SPILL_CODE_CLEANUP,
    SMS_VR2R_SPILL_CODE,
    SMS_VR2R_CUT_NONSPILLABLE_LIVERANGES,
    SMS_VR2R_DELETE_INTERFERENCE,
    SMS_VR2R_ATOMIC_REORDER_INSTRUCTIONS_2,
    SMS_VR2R_OPTIMIZE_ALIGNMENT_DETECTION,
    SMS_OPTIMIZER_REGISTER_ALLOCATOR,
    SMS_CONVERTERS,
    SMS_ISA_OPTIMIZATIONS,
    SMS_ISA_SCHEDULER,
    SMS_UNACCOUNTED,

    MAX_SHADER_MEMORY_BUCKET,
};

struct SShaderMemoryBucketInfo
{
    const char* Name;
    bool IsMilestone;
};

/*****************************************************************************\
g_cShaderMemoryBucket
Memory Bucket information.
\*****************************************************************************/
static const SShaderMemoryBucketInfo g_cShaderMemoryBucket[] = 
{
    { "Total", true },
    { "D3D10 Compiler", true },
    { "IL Compiler: PreCompile", false },
    { "IL Compiler: Optimizations", false },
    { "IL Compiler", true },
    { "Decomposer", true },
    { "VR2R: After opt.: CreateRegInstMapping", false },
    { "Optimizer analysis", true },
    { "VR2R: After opt.: ChannelPropagation", false },
    { "VR2R: DetectScalars", false },
    { "VR2R: After opt.: DoSimdReduction", false },
    { "VR2R: After opt.: OptimizeIndirectConstantBufferLoads", false },
    { "VR2R: After opt.: OptimizeResourceLoads", false },
    { "VR2R: After opt.: MergeRegisterCopies", false },
    { "VR2R: After opt.: MovPropagation", false },
    { "VR2R: After opt.: CondModPropagation", false },
    { "VR2R: After opt.: KillAfterUnconditionalDiscard", false },
    { "VR2R: After opt.: PatternMatchReplace", false },
    { "VR2R: After opt.: DeadBranchRemoval", false },
    { "VR2R: After opt.: OptimizeConstantBufferLoads #1", false },
    { "VR2R: After opt.: OptimizeReplicateInstructions #1", false },
    { "VR2R: After opt.: ConstantFolding", false },
    { "VR2R: After opt.: MemoryAliasing", false },
    { "VR2R: After opt.: ValueNumbering", false },
    { "VR2R: After opt.: ImmediatesToConstantBuffer", false },
    { "VR2R: After opt.: SimdExpansion", false },
    { "VR2R: After opt.: OptimizeReplicateInstructions #2", false },
    { "VR2R: After opt.: OptimizeConstantBufferLoads #2", false },
    { "VR2R: After opt.: OptimizeConstantBufferLoads #3", false },
    { "VR2R: After opt.: OptimizeReplicateInstructions #3", false },
    { "VR2R: After opt.: MergeLoadBufferInstruction", false },
    { "VR2R: After opt.: ReorderInstructions #1", false },
    { "VR2R: After opt.: TPMPromotion", false },
    { "VR2R: After opt.: CodeSinking", false },
    { "VR2R: After opt.: LoopInvariantCodeMotion", false },
    { "Optimizer passes", true },
    { "VR2R: After opt.: RegionPreScheduling", true },
    { "VR2R: After opt.: Renumber", false },
    { "VR2R: Spill iter != 0; Allocate live ranges", false },
    { "VR2R: Allocate the interference graph", false },
    { "VR2R: Build the interference graph", false },
    { "VR2R: After opt.: RenameWithLiveRanges", false },
    { "VR2R: Free the array of live range sets", false },
    { "VR2R: Spill costs", false },
    { "VR2R: SetupGrfBankAlignment", false },
    { "VR2R: Color the graph", false },
    { "VR2R: After opt.: RenameWithRegisters", false },
    { "VR2R: After opt.: SpillCodeCleanup", false },
    { "VR2R: After opt.: SpillCode", false },
    { "VR2R: After opt.: CutNonspillableLiveRanges", false },
    { "VR2R: Interference graph deleted", false },
    { "VR2R: After opt.: AtomicReorderInstructions #2", false },
    { "VR2R: After opt.: AlignmentDection", false },
    { "VR2R: Optimizer Register Allocator", true },
    { "Converts", true },
    { "ISA optimizations", true },
    { "ISA Scheduler", true },
    { "Unaccounted", true },
};

/*****************************************************************************\
CLASS: CMemReport

Description:
Class used for marking blocks of code assigned to certain compilation
memory stat.
Usage:
Creating one global object of this class allow to use function to calculate
allocated and free memory per shader.
Before compilation using usageReset method is required.
During allocation or freeing we want to calculate we should use also
InstrMalloc or InstrFree method.
\*****************************************************************************/
class CMemReport
{
public:
    struct SBucket
    {
        unsigned int NumAllocations;
        unsigned int NumReleases;
        unsigned int NumCurrAllocations;
        unsigned int NumCurrAllocationsPeak;
        int          HeapUsed;
        unsigned int HeapUsedPeak;
        unsigned int NumSnapAllocationsType[ SMAT_NUM_OF_TYPES ];

        void MallocBucket( size_t size );
        void FreeBucket( size_t size );
    };

    static const int        sc_MaxBuckets = 10;
    bool                    m_BucketsEnable;
    SBucket*                m_pGlobalBucket;
    SBucket                 m_Buckets[ MAX_SHADER_MEMORY_BUCKET ];
    SHADER_MEMORY_BUCKET    m_OpenBucketsList[ sc_MaxBuckets ];

    void OpenBucket( SHADER_MEMORY_BUCKET phase );
    void CloseBucket( SHADER_MEMORY_BUCKET phase );
    void InitializeBuckets();

    CMemReport( void );
    ~CMemReport( void );

    static void MallocInstrumentation( size_t size );
    static void FreeInstrumentation( size_t size );

    void CreateDumpFiles( char &dumpDirectory );
    void UsageReset();
    void DumpMemoryStats();
    void DumpStats( const char *shaderName );
    void SetDetailed( bool Enable );
    void SetSharedDumpBaseFileName( const char *shaderName );

    char m_CsvNameUsage[ 1024 ];
    char m_CsvNameAllocs[ 1024 ];
    char m_CsvNameAllocsSubset[ 1024 ];
    char m_DumpDirectory[ 1024 ];
    char m_DumpMemoryStatsFileName[ 1024 ];
    bool m_GrabDetailed;
};

extern CMemReport g_MemoryReport;

C_ASSERT( ( sizeof( g_cShaderMemoryBucket ) / sizeof( *g_cShaderMemoryBucket ) ) == MAX_SHADER_MEMORY_BUCKET );

extern int g_ILTokens;

#define OPEN_BUCKET( phase ) g_MemoryReport.OpenBucket( phase )
#define CLOSE_BUCKET( phase ) g_MemoryReport.CloseBucket( phase )

#else // USC_MEMORY_STATS_ENABLE
#define OPEN_BUCKET( phase )
#define CLOSE_BUCKET( phase )
#endif // USC_MEMORY_STATS_ENABLE

#if USC_COMPILE_TIME_METRICS_ENABLE 

enum USC_COMPILE_TIME_INTERVALS
{
    USC_COMPILE_TIME_TOTAL,
    USC_COMPILE_TIME_IL_CREATE,
    USC_COMPILE_TIME_IL_OPTIMIZE,
    USC_COMPILE_TIME_IL_OPTIMIZE_PRECOMPILE,
    USC_COMPILE_TIME_IL_OPTIMIZE_PARSEOPCODESTOPRECMODIFIER,
    USC_COMPILE_TIME_IL_OPTIMIZE_FINDFUNCTIONS,
    USC_COMPILE_TIME_IL_OPTIMIZE_BUILDCALLGRAPH,
    USC_COMPILE_TIME_IL_OPTIMIZE_REMOVEDEADOUTPUTS,
    USC_COMPILE_TIME_IL_OPTIMIZE_PREPROCESS,
    USC_COMPILE_TIME_IL_OPTIMIZE_CALLOPTS,
    USC_COMPILE_TIME_IL_OPTIMIZE_INLINING,
    USC_COMPILE_TIME_IL_OPTIMIZE_CALLTOSUBROUTINECALL,
    USC_COMPILE_TIME_IL_OPTIMIZE_PATTERNMATCH,
    USC_COMPILE_TIME_IL_OPTIMIZE_POINTERLOADEXPANSION,
    USC_COMPILE_TIME_IL_OPTIMIZE_CONSTANTFOLDING,
    USC_COMPILE_TIME_IL_OPTIMIZE_CPIDFOLDING,
    USC_COMPILE_TIME_IL_OPTIMIZE_LOOPUNROLLING,
    USC_COMPILE_TIME_IL_OPTIMIZE_CONDITIONALEXPRESSIONSIMPLIFICATION,
    USC_COMPILE_TIME_IL_OPTIMIZE_IFCONVERSION,
    USC_COMPILE_TIME_IL_OPTIMIZE_PRINTFTRANSLATION,
    USC_COMPILE_TIME_IL_OPTIMIZE_CONSTANTBUFFERTOCONSTANTREGISTER,
    USC_COMPILE_TIME_IL_OPTIMIZE_EXTERNALOPTIMIZER,
    USC_COMPILE_TIME_IL_OPTIMIZE_POSTCOMPILE,
    USC_COMPILE_TIME_IL_OUTPUT,
    USC_COMPILE_TIME_LIR_DECOMPOSE,
    USC_COMPILE_TIME_LIR_ANALYZE,
    USC_COMPILE_TIME_LIR_ANALYZE_SSA,
    USC_COMPILE_TIME_LIR_ANALYZE_LIVENESS_1,
    USC_COMPILE_TIME_LIR_ANALYZE_LIVENESS_3,
    USC_COMPILE_TIME_LIR_ANALYZE_LIVENESS_4,
    USC_COMPILE_TIME_LIR_ANALYZE_LIVENESS_5,
    USC_COMPILE_TIME_LIR_ANALYZE_LIVENESS_6,
    USC_COMPILE_TIME_LIR_ANALYZE_LIVENESS_7,
    USC_COMPILE_TIME_LIR_OPTIMIZE,
    USC_COMPILE_TIME_LIR_OPTIMIZE_PREPROC,
    USC_COMPILE_TIME_LIR_OPTIMIZE_CONSTANT_FOLDING,
    USC_COMPILE_TIME_LIR_OPTIMIZE_PATTERN_MATCH,
    USC_COMPILE_TIME_LIR_OPTIMIZE_VALUE_NUMBERING,
    USC_COMPILE_TIME_LIR_OPTIMIZE_DEAD_CODE_REMOVAL,
    USC_COMPILE_TIME_LIR_OPTIMIZE_REMOVE_REPLICATES,
    USC_COMPILE_TIME_LIR_OPTIMIZE_DETECT_SCALARS,
    USC_COMPILE_TIME_LIR_OPTIMIZE_SIMD_REDUCTION,
    USC_COMPILE_TIME_LIR_OPTIMIZE_DCB,
    USC_COMPILE_TIME_LIR_OPTIMIZE_OPT_RES_LOADS,
    USC_COMPILE_TIME_LIR_OPTIMIZE_COALESCE_IO,
    USC_COMPILE_TIME_LIR_OPTIMIZE_MOV_PROPAGATION,
    USC_COMPILE_TIME_LIR_OPTIMIZE_COND_MOD_PROPAGATION,
    USC_COMPILE_TIME_LIR_OPTIMIZE_CHANNEL_PROPAGATION,
    USC_COMPILE_TIME_LIR_OPTIMIZE_LICM,
    USC_COMPILE_TIME_LIR_OPTIMIZE_REORDER_INST,
    USC_COMPILE_TIME_LIR_OPTIMIZE_CODE_SINKING,
    USC_COMPILE_TIME_LIR_OPTIMIZE_RESTORE_CSSA,
    USC_COMPILE_TIME_LIR_OPTIMIZE_PRUNING,
    USC_COMPILE_TIME_LIR_OPTIMIZE_PS_OUTPUT_DETECTION,
    USC_COMPILE_TIME_LIR_OPTIMIZE_BTI_USAGE_REPORT,
    USC_COMPILE_TIME_LIR_OPTIMIZE_SHADER_INPUT_PACKING,
    USC_COMPILE_TIME_LIR_OPTIMIZE_OPT_SIMD8_MOVS,
    USC_COMPILE_TIME_LIR_OPTIMIZE_OPT_IMM2CR,
    USC_COMPILE_TIME_LIR_OPTIMIZE_CALC_MAX_VALUES,
    USC_COMPILE_TIME_LIR_OPTIMIZE_LOWPREC,
    USC_COMPILE_TIME_LIR_OPTIMIZE_SIMDEXP,
    USC_COMPILE_TIME_LIR_OPTIMIZE_MEMORY_ALIASING,
    USC_COMPILE_TIME_LIR_OPTIMIZE_FOLD_UNPACKS,
    USC_COMPILE_TIME_LIR_OPTIMIZE_REDUCE_ALU_PRECISION,
    USC_COMPILE_TIME_LIR_OPTIMIZE_MERGE_SPLIT_JOIN_DP,
    USC_COMPILE_TIME_LIR_OPTIMIZE_COALESCING_PREPARE_IO,
    USC_COMPILE_TIME_LIR_PRESCHED,
    USC_COMPILE_TIME_LIR_OPTIMIZE_TPMPROMO,
    USC_COMPILE_TIME_LIR_REGALLOC,
    USC_COMPILE_TIME_LIR_REGALLOC_GC,
    USC_COMPILE_TIME_LIR_REGALLOC_LS,
    USC_COMPILE_TIME_LIR_REGALLOC_TS,
    USC_COMPILE_TIME_LIR_REGALLOC_RENAMELIVERANGE,
    USC_COMPILE_TIME_LIR_REGALLOC_RENUMBER,
    USC_COMPILE_TIME_LIR_REGALLOC_BUILDCOALESCE,
    USC_COMPILE_TIME_LIR_REGALLOC_PREPROCESS_IG,
    USC_COMPILE_TIME_LIR_REGALLOC_BUILD_IG,
    USC_COMPILE_TIME_LIR_REGALLOC_COALESCE,
    USC_COMPILE_TIME_LIR_REGALLOC_SPILLCOST,
    USC_COMPILE_TIME_LIR_REGALLOC_SIMPLIFYSELECT,
    USC_COMPILE_TIME_LIR_REGALLOC_SIMPLIFY,
    USC_COMPILE_TIME_LIR_REGALLOC_COMPUTE_DEGREES,
    USC_COMPILE_TIME_LIR_REGALLOC_RELAX_DEGREES,
    USC_COMPILE_TIME_LIR_REGALLOC_SELECT,
    USC_COMPILE_TIME_LIR_REGALLOC_COLOR_NODE,
    USC_COMPILE_TIME_LIR_REGALLOC_SELECT_SPILL,
    USC_COMPILE_TIME_LIR_REGALLOC_RENAMEREGISTER,
    USC_COMPILE_TIME_LIR_REGALLOC_SPILLCODE,
    USC_COMPILE_TIME_LIR_REGALLOC_GRFBA,
    USC_COMPILE_TIME_LIR_REGALLOC_CUTLIVERANGES,
    USC_COMPILE_TIME_LIR_REGALLOC_REMAT,
    USC_COMPILE_TIME_LIR_REGALLOC_LS_PREPROCESS,
    USC_COMPILE_TIME_LIR_REGALLOC_LS_BUILD_INTERVALS,
    USC_COMPILE_TIME_LIR_REGALLOC_LS_COLOR,
    USC_COMPILE_TIME_LIR_REGALLOC_LS_CREATE_DELETE,
    USC_COMPILE_TIME_LIR_CLEANUP,
    USC_COMPILE_TIME_ISA_CONVERT,
    USC_COMPILE_TIME_ISA_OPTIMIZE,
    USC_COMPILE_TIME_ISA_SCHEDULE,
    USC_COMPILE_TIME_PATCH,
    USC_COMPILE_TIME_UNACCOUNTED,

    MAX_USC_COMPILE_TIME_INTERVALS
};

extern const char* g_cCompTimeIntervals[];

/*****************************************************************************\
CLASS: CCompileTimeInterval

Description:
Simple class used for marking blocks of code assigned to certain compilation
time stat.
Usage:
Define object of this class on the beginning of the block of code. On exit
of the block class will be destroyed and measured time will be added to 
pTimeMetrics.
\*****************************************************************************/
class CCompileTimeInterval
{
public:
    CCompileTimeInterval( const CShader* pShader, USC_COMPILE_TIME_INTERVALS interval );
    CCompileTimeInterval( const CShader* pShader, USC_COMPILE_TIME_INTERVALS interval, bool fixupAtEnd );
    ~CCompileTimeInterval( void );

private:
    const CShader*              m_pShader;
    USC_COMPILE_TIME_INTERVALS  m_interval;
    QWORD                       m_timestampStart;
    bool                        m_bFixupAtEnd;
};

/*****************************************************************************\
MACRO: USC_COMPILE_TIME_SENTRY

Description:
Define object of class CCompileTimeInterval for measuring compilation time.
\*****************************************************************************/
#define USC_COMPILE_TIME_SENTRY( name, shader, interval ) \
    CCompileTimeInterval name( shader, interval )

#define USC_COMPILE_TIME_SENTRY_FIXUP( name, shader, interval ) \
    CCompileTimeInterval name( shader, interval, true )

#else

/*****************************************************************************\
MACRO: USC_COMPILE_TIME_SENTRY

Description:
For disabled TimeStats define USC_COMPILE_TIME_SENTRY as empty macro
\*****************************************************************************/
#define USC_COMPILE_TIME_SENTRY( name, shader, interval )

#define USC_COMPILE_TIME_SENTRY_FIXUP( name, shader, interval )

#endif // USC_COMPILE_TIME_METRICS_ENABLE

#if USC_QUALITY_METRICS_ENABLE

enum USC_QUALITY_METRICS_TYPES
{
    USC_QUALITY_METRICS_INSTRUCTION_COUNT,      
    USC_QUALITY_METRICS_SHADER_IPC, 
    USC_QUALITY_METRICS_SHADER_QUALITY_INDEX,
    USC_QUALITY_METRICS_PENALTY_MOV,
    USC_QUALITY_METRICS_PENALTY_ALU,
    USC_QUALITY_METRICS_PENALTY_IO,
    USC_QUALITY_METRICS_PENALTY_CTL,
    USC_QUALITY_METRICS_PENALTY_BANK,
    USC_QUALITY_METRICS_PENALTY_COISSUE,
    USC_QUALITY_METRICS_PENALTY_SIMD,
    USC_QUALITY_METRICS_PENALTY_SCHEDULING,
    
    MAX_USC_QUALITY_METRICS_TYPES
};

#endif // USC_QUALITY_METRICS_ENABLE

#if defined( _DEBUG ) || defined( _INTERNAL) || GHAL3D_DEBUG_VARIABLES
/*****************************************************************************\

Sruct:
    SCompilerDebugState

Description:
    Structure that holds all debug data that should be preserved between
    different invocations of CShaderCompiler.

\*****************************************************************************/

struct SCompilerDebugState
{
#if USC_QUALITY_METRICS_ENABLE 
    char SQIFileName[ 1024 ];

    DWORD ShaderCount[NUM_SHADER_TYPES];      
    float InstructionCount[NUM_SHADER_TYPES];      
    float ShaderIPC[NUM_SHADER_TYPES];      
    float ShaderQualityIndex[NUM_SHADER_TYPES];      
    float PenaltyMOV[NUM_SHADER_TYPES];      
    float PenaltyALU[NUM_SHADER_TYPES];   
    float PenaltyIO[NUM_SHADER_TYPES];   
    float PenaltyCTL[NUM_SHADER_TYPES];   
    float PenaltyBank[NUM_SHADER_TYPES];   
    float PenaltyCoissue[NUM_SHADER_TYPES];   
    float PenaltySimd[NUM_SHADER_TYPES];   
    float PenaltyScheduling[NUM_SHADER_TYPES];
#endif 

#if GHAL3D_OPTIMIZATION_STATS_ENABLE
    char ShaderCountersFileName[ 1024 ];

    SOptStats TotalSuccOpStatsPerType[NUM_SHADER_TYPES][NUM_STATS_SIMD_TYPES];
    SOptStats TotalSuccOpStatsSum;
    SOptStats TotalFailedOpStatsPerType[NUM_SHADER_TYPES][NUM_STATS_SIMD_TYPES];
    SOptStats TotalFailedOpStatsSum;

    DWORD TotalSuccCompilationsPerType[NUM_SHADER_TYPES][NUM_STATS_SIMD_TYPES];
    DWORD TotalFailedCompilationsPerType[NUM_SHADER_TYPES][NUM_STATS_SIMD_TYPES];
#endif

#if USC_COMPILE_TIME_METRICS_ENABLE 
    char    CompTimeFileName[ 1024 ];
    char    CompTimeFileNameDetailed[ 1024 ];
    QWORD   CompileTimeMetrics[MAX_USC_COMPILE_TIME_INTERVALS]; 
    QWORD   StartTimestamp;
#endif

    bool Initialized;

#if USC_COMPILE_TIME_METRICS_ENABLE
    void CleanupCompileTimeMetrics();
#endif

#if USC_QUALITY_METRICS_ENABLE 
    void CleanupQualityMetrics();
#endif

#if GHAL3D_OPTIMIZATION_STATS_ENABLE
    void CleanupShaderCounters();
    void DumpShaderCountSummary(
        void* outputFile,
        DWORD totalCompilationsPerType[ NUM_SHADER_TYPES ][ NUM_STATS_SIMD_TYPES ] );
#endif

    void CleanupDebugState();
};

#endif // defined( _DEBUG ) || GHAL3D_DEBUG_VARIABLES
}
