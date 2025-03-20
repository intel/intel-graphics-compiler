/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*
 * ISA Instruction Description
 *
 * This file contains tables used to define ISA instructions.
 * These tables are exposed in the corresponding header with
 * external linkage alone with their associated enums.
 */

#include "IsaDescription.h"
#include "Assertions.h"
#include "IGC/common/StringMacros.hpp"

#include <cstring>

struct ISA_Inst_Info ISA_Inst_Table[ISA_OPCODE_ENUM_SIZE] = {
    {ISA_RESERVED_0, ISA_Inst_Reserved, "reserved0", 0, 0},
    {ISA_ADD, ISA_Inst_Arith, "add", 2, 1},
    {ISA_AVG, ISA_Inst_Arith, "avg", 2, 1},
    {ISA_DIV, ISA_Inst_Arith, "div", 2, 1},
    {ISA_DP2, ISA_Inst_Arith, "dp2", 2, 1},
    {ISA_DP3, ISA_Inst_Arith, "dp3", 2, 1},
    {ISA_DP4, ISA_Inst_Arith, "dp4", 2, 1},
    {ISA_DPH, ISA_Inst_Arith, "dph", 2, 1},
    {ISA_EXP, ISA_Inst_Arith, "exp", 1, 1},
    {ISA_FRC, ISA_Inst_Arith, "frc", 1, 1},
    {ISA_LINE, ISA_Inst_Arith, "line", 2, 1},
    {ISA_LOG, ISA_Inst_Arith, "log", 1, 1},
    {ISA_MAD, ISA_Inst_Arith, "mad", 3, 1},
    {ISA_MULH, ISA_Inst_Arith, "mulh", 2, 1},
    {ISA_LRP, ISA_Inst_Arith, "lrp", 3, 1},
    {ISA_MOD, ISA_Inst_Arith, "mod", 2, 1},
    {ISA_MUL, ISA_Inst_Arith, "mul", 2, 1},
    {ISA_POW, ISA_Inst_Arith, "pow", 2, 1},
    {ISA_RNDD, ISA_Inst_Arith, "rndd", 1, 1},
    {ISA_RNDU, ISA_Inst_Arith, "rndu", 1, 1},
    {ISA_RNDE, ISA_Inst_Arith, "rnde", 1, 1},
    {ISA_RNDZ, ISA_Inst_Arith, "rndz", 1, 1},
    {ISA_SAD2, ISA_Inst_Arith, "sad2", 2, 1},
    {ISA_SIN, ISA_Inst_Arith, "sin", 1, 1},
    {ISA_COS, ISA_Inst_Arith, "cos", 1, 1},
    {ISA_SQRT, ISA_Inst_Arith, "sqrt", 1, 1},
    {ISA_RSQRT, ISA_Inst_Arith, "rsqrt", 1, 1},
    {ISA_INV, ISA_Inst_Arith, "inv", 1, 1},
    {ISA_DPASW, ISA_Inst_Misc, "dpasw", 3, 1},
    {ISA_FCVT, ISA_Inst_Mov, "fcvt", 1, 1},
    {ISA_SRND, ISA_Inst_Arith, "srnd", 2, 1},
    {ISA_LZD, ISA_Inst_Arith, "lzd", 1, 1},
    {ISA_AND, ISA_Inst_Logic, "and", 2, 1},
    {ISA_OR, ISA_Inst_Logic, "or", 2, 1},
    {ISA_XOR, ISA_Inst_Logic, "xor", 2, 1},
    {ISA_NOT, ISA_Inst_Logic, "not", 1, 1},
    {ISA_SHL, ISA_Inst_Logic, "shl", 2, 1},
    {ISA_SHR, ISA_Inst_Logic, "shr", 2, 1},
    {ISA_ASR, ISA_Inst_Logic, "asr", 2, 1},
    {ISA_CBIT, ISA_Inst_Logic, "cbit", 1, 1},
    {ISA_ADDR_ADD, ISA_Inst_Address, "addr_add", 2, 1},
    {ISA_MOV, ISA_Inst_Mov, "mov", 1, 1},
    {ISA_SEL, ISA_Inst_Mov, "sel", 2, 1},
    {ISA_SETP, ISA_Inst_Mov, "setp", 1, 1},
    {ISA_CMP, ISA_Inst_Compare, "cmp", 2, 1},
    {ISA_MOVS, ISA_Inst_Mov, "movs", 1, 1},
    {ISA_FBL, ISA_Inst_Logic, "fbl", 1, 1},
    {ISA_FBH, ISA_Inst_Logic, "fbh", 1, 1},
    {ISA_SUBROUTINE, ISA_Inst_Flow, "func", 1, 0},
    {ISA_LABEL, ISA_Inst_Flow, "label", 1, 0},
    {ISA_JMP, ISA_Inst_Flow, "jmp", 1, 0},
    {ISA_CALL, ISA_Inst_Flow, "call", 1, 0},
    {ISA_RET, ISA_Inst_Flow, "ret", 0, 0},
    {ISA_OWORD_LD, ISA_Inst_Data_Port, "oword_ld", 2, 1},
    {ISA_OWORD_ST, ISA_Inst_Data_Port, "oword_st", 3, 0},
    {ISA_MEDIA_LD, ISA_Inst_Data_Port, "media_ld", 5, 1},
    {ISA_MEDIA_ST, ISA_Inst_Data_Port, "media_st", 6, 0},
    {ISA_GATHER, ISA_Inst_Data_Port, "gather", 4, 1},
    {ISA_SCATTER, ISA_Inst_Data_Port, "scatter", 5, 0},
    {ISA_RESERVED_3B, ISA_Inst_Reserved, "reserved3b", 0, 0},
    {ISA_OWORD_LD_UNALIGNED, ISA_Inst_Data_Port, "oword_ld_unaligned", 2, 1},
    {ISA_RESERVED_3D, ISA_Inst_Reserved, "reserved3d", 0, 0},
    {ISA_RESERVED_3E, ISA_Inst_Reserved, "reserved3e", 0, 0},
    {ISA_RESERVED_3F, ISA_Inst_Reserved, "reserved3f", 0, 0},
    {ISA_SAMPLE, ISA_Inst_Sampler, "sample", 5, 1},
    {ISA_SAMPLE_UNORM, ISA_Inst_Sampler, "sample_unorm", 6, 1},
    {ISA_LOAD, ISA_Inst_Sampler, "load", 4, 1},
    {ISA_AVS, ISA_Inst_Sampler, "avs", 13, 1},
    {ISA_VA, ISA_Inst_Sampler, "va", 0, 0},
    {ISA_FMINMAX, ISA_Inst_Mov, "fminmax", 2, 1},
    {ISA_BFE, ISA_Inst_Logic, "bfe", 3, 1},
    {ISA_BFI, ISA_Inst_Logic, "bfi", 4, 1},
    {ISA_BFREV, ISA_Inst_Logic, "bfrev", 1, 1},
    {ISA_ADDC, ISA_Inst_Arith, "addc", 2, 2},
    {ISA_SUBB, ISA_Inst_Arith, "subb", 2, 2},
    {ISA_GATHER4_TYPED, ISA_Inst_Data_Port, "gather4_typed", 7, 1},
    {ISA_SCATTER4_TYPED, ISA_Inst_Data_Port, "scatter4_typed", 8, 0},
    {ISA_VA_SKL_PLUS, ISA_Inst_Sampler, "va_skl_plus", 0, 0},
    {ISA_SVM, ISA_Inst_SVM, "svm", 0, 0},
    {ISA_IFCALL, ISA_Inst_Flow, "ifcall", 4, 0},
    {ISA_FADDR, ISA_Inst_Flow, "faddr", 1, 1},
    {ISA_FILE, ISA_Inst_Misc, "file", 1, 0},
    {ISA_LOC, ISA_Inst_Misc, "loc", 1, 0},
    {ISA_RESERVED_53, ISA_Inst_Reserved, "reserved53", 0, 0},
    {ISA_VME_IME, ISA_Inst_Misc, "vme_ime", 5, 1},
    {ISA_VME_SIC, ISA_Inst_Misc, "vme_sic", 2, 1},
    {ISA_VME_FBR, ISA_Inst_Misc, "vme_fbr", 2, 1},
    {ISA_VME_IDM, ISA_Inst_Misc, "vme_idm", 2, 1},
    {ISA_RESERVED_58, ISA_Inst_Reserved, "reserved58", 0, 0},
    {ISA_BARRIER, ISA_Inst_Sync, "barrier", 0, 0},
    {ISA_SAMPLR_CACHE_FLUSH, ISA_Inst_Sync, "sampler_cache_flush", 0, 0},
    {ISA_WAIT, ISA_Inst_Sync, "wait", 0, 0},
    {ISA_FENCE, ISA_Inst_Sync, "fence", 0, 0},
    {ISA_RAW_SEND, ISA_Inst_Misc, "raw_send", 0, 0},
    {ISA_RESERVED_5E, ISA_Inst_Reserved, "reserved5E", 0, 0},
    {ISA_YIELD, ISA_Inst_Sync, "yield", 0, 0},
    {ISA_NBARRIER, ISA_Inst_Sync, "nbarrier", 4, 0},
    {ISA_RESERVED_61, ISA_Inst_Reserved, "reserved61", 0, 0},
    {ISA_RESERVED_62, ISA_Inst_Reserved, "reserved62", 0, 0},
    {ISA_RESERVED_63, ISA_Inst_Reserved, "reserved63", 0, 0},
    {ISA_RESERVED_64, ISA_Inst_Reserved, "reserved64", 0, 0},
    {ISA_RESERVED_65, ISA_Inst_Reserved, "reserved65", 0, 0},
    {ISA_RESERVED_66, ISA_Inst_Reserved, "reserved66", 0, 0},
    {ISA_FCALL, ISA_Inst_Flow, "fcall", 3, 0},
    {ISA_FRET, ISA_Inst_Flow, "fret", 0, 0},
    {ISA_SWITCHJMP, ISA_Inst_Flow, "switchjmp", 0, 0},
    {ISA_SAD2ADD, ISA_Inst_Arith, "sad2add", 3, 1},
    {ISA_PLANE, ISA_Inst_Arith, "plane", 2, 1},
    {ISA_GOTO, ISA_Inst_SIMD_Flow, "goto", 1, 0},
    {ISA_3D_SAMPLE, ISA_Inst_Sampler, "sample_3d", 6, 1},
    {ISA_3D_LOAD, ISA_Inst_Sampler, "load_3d", 6, 1},
    {ISA_3D_GATHER4, ISA_Inst_Sampler, "gather4_3d", 6, 1},
    {ISA_3D_INFO, ISA_Inst_Sampler, "info_3d", 2, 1},
    {ISA_3D_RT_WRITE, ISA_Inst_Data_Port, "rt_write_3d", 3, 1},
    {ISA_3D_URB_WRITE, ISA_Inst_Misc, "urb_write_3d", 6, 0},
    {ISA_3D_TYPED_ATOMIC, ISA_Inst_Data_Port, "typed_atomic", 9, 1},
    {ISA_GATHER4_SCALED, ISA_Inst_Data_Port, "gather4_scaled", 4, 1},
    {ISA_SCATTER4_SCALED, ISA_Inst_Data_Port, "scatter4_scaled", 5, 0},
    {ISA_RESERVED_76, ISA_Inst_Reserved, "reserved76", 0, 0},
    {ISA_RESERVED_77, ISA_Inst_Reserved, "reserved77", 0, 0},
    {ISA_GATHER_SCALED, ISA_Inst_Data_Port, "gather_scaled", 4, 1},
    {ISA_SCATTER_SCALED, ISA_Inst_Data_Port, "scatter_scaled", 5, 0},
    {ISA_RAW_SENDS, ISA_Inst_Misc, "raw_sends", 0, 0},
    {ISA_LIFETIME, ISA_Inst_Misc, "lifetime", 2, 0},
    {ISA_SBARRIER, ISA_Inst_Sync, "sbarrier", 1, 0},
    {ISA_DWORD_ATOMIC, ISA_Inst_Data_Port, "dword_atomic", 5, 1},
    {ISA_SQRTM, ISA_Inst_Arith, "sqrtm", 1, 1},
    {ISA_DIVM, ISA_Inst_Arith, "divm", 2, 1},
    {ISA_ROL, ISA_Inst_Logic, "rol", 2, 1},
    {ISA_ROR, ISA_Inst_Logic, "ror", 2, 1},
    {ISA_DP4A, ISA_Inst_Arith, "dp4a", 3, 1},
    {ISA_DPAS, ISA_Inst_Misc, "dpas", 3, 1},
    {ISA_ADD3, ISA_Inst_Arith, "add3", 3, 1},
    {ISA_BFN, ISA_Inst_Logic, "bfn", 3, 1},
    {ISA_QW_GATHER, ISA_Inst_Data_Port, "qw_gather", 3, 1},
    {ISA_QW_SCATTER, ISA_Inst_Data_Port, "qw_scatter", 4, 0},
    {ISA_RESERVED_88, ISA_Inst_Mov, "reserved88", 1, 1},
    {ISA_LSC_UNTYPED, ISA_Inst_LSC, "lsc_untyped", 0, 0},
    {ISA_LSC_TYPED, ISA_Inst_LSC, "lsc_typed", 0, 0},
    {ISA_LSC_FENCE, ISA_Inst_LSC, "lsc_fence", 0, 0},
    {ISA_RESERVED_8C, ISA_Inst_Reserved, "reserved8c", 0, 0},
    {ISA_RESERVED_8D, ISA_Inst_Reserved, "reserved8d", 0, 0},
    {ISA_RESERVED_8E, ISA_Inst_Reserved, "reserved8e", 0, 0},
    {ISA_RESERVED_8F, ISA_Inst_Reserved, "reserved8f", 0, 0},
    {ISA_RESERVED_90, ISA_Inst_Reserved, "reserved90", 0, 0},
    {ISA_MADW, ISA_Inst_Arith, "madw", 3, 1},
    {ISA_ADD3O, ISA_Inst_Arith, "add3.o", 3, 1},
    {ISA_RESERVED_93, ISA_Inst_Reserved, "reserved93", 0, 0},
    {ISA_BREAKPOINT, ISA_Inst_Misc, "breakpoint", 0, 0},
    {ISA_RESERVED_95, ISA_Inst_Reserved, "reserved95", 0, 0},
    {ISA_RESERVED_96, ISA_Inst_Reserved, "reserved96", 0, 0},
    {ISA_RESERVED_97, ISA_Inst_Reserved, "reserved97", 0, 0},
    {ISA_RESERVED_98, ISA_Inst_Reserved, "reserved98", 0, 0},
    {ISA_RESERVED_99, ISA_Inst_Reserved, "reserved99", 0, 0},
    {ISA_RESERVED_9A, ISA_Inst_Reserved, "reserved9a", 0, 0},
    {ISA_INVM,  ISA_Inst_Arith, "invm",  2, 2},
    {ISA_RSQTM, ISA_Inst_Arith, "rsqtm", 1, 2},
};

VISA_INST_Desc CISA_INST_table[ISA_NUM_OPCODE] = {
    /// 0
    {
        ALL,
        ISA_RESERVED_0,
        ISA_Inst_Reserved,
        "RESERVED_0",
        0,
        0,
        {},

    },

    /// 1
    {
        ALL,
        ISA_ADD,
        ISA_Inst_Arith,
        "add",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_INTEGER | TYPE_FLOAT_ALL, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER | TYPE_FLOAT_ALL, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER | TYPE_FLOAT_ALL, 0},
        },

    },

    /// 2
    {
        ALL,
        ISA_AVG,
        ISA_Inst_Arith,
        "avg",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
        },

    },

    ///  3
    {
        ALL,
        ISA_DIV,
        ISA_Inst_Arith,
        "div",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_INTEGER | TYPE_FLOAT, SAT_FLOAT_ONLY},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER | TYPE_FLOAT_ALL, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER | TYPE_FLOAT_ALL, 0},
        },

    },

    /// 0x4
    {
        ALL,
        ISA_DP2,
        ISA_Inst_Arith,
        "dp2",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, GE_4},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_C | HORIZON_STRIDE_1},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, HORIZON_STRIDE_1},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, HORIZON_STRIDE_1},
        },

    },

    /// 5
    {
        ALL,
        ISA_DP3,
        ISA_Inst_Arith,
        "dp3",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, GE_4},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_C | HORIZON_STRIDE_1},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, HORIZON_STRIDE_1},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, HORIZON_STRIDE_1},
        },

    },

    /// 6
    {
        ALL,
        ISA_DP4,
        ISA_Inst_Arith,
        "dp4",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, GE_4},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_C | HORIZON_STRIDE_1},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, HORIZON_STRIDE_1},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, HORIZON_STRIDE_1},
        },

    },

    /// 7
    {
        ALL,
        ISA_DPH,
        ISA_Inst_Arith,
        "dph",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, GE_4},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_C | HORIZON_STRIDE_1},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, HORIZON_STRIDE_1},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, HORIZON_STRIDE_1},
        },

    },

    /// 8
    {
        ALL,
        ISA_EXP,
        ISA_Inst_Arith,
        "exp",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL, SAT_C | HORIZON_STRIDE_1},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, HORIZON_STRIDE_1},
        },

    },

    /// 9
    {
        ALL,
        ISA_FRC,
        ISA_Inst_Arith,
        "frc",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
        },

    },

    /// 10
    {
        ALL,
        ISA_LINE,
        ISA_Inst_Arith,
        "line",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, GE_4},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_INTEGER | TYPE_FLOAT, SAT_C},
            {OPND_SRC_GEN, TYPE_INTEGER | TYPE_FLOAT,
             HORIZON_VERTICAL_STRIDE_0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER | TYPE_FLOAT, 0},
        },

    },

    /// 11
    {
        ALL,
        ISA_LOG,
        ISA_Inst_Arith,
        "log",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
        },

    },


    /// 12
    {
        ALL,
        ISA_MAD,
        ISA_Inst_Arith,
        "mad",
        6,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL, SAT_FLOAT_ONLY},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
        },

    },

    /// 13
    {
        ALL,
        ISA_MULH,
        ISA_Inst_Arith,
        "mulh",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD | ISA_TYPE_D, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD | ISA_TYPE_D, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD | ISA_TYPE_D, 0},
        },

    },

    /// 14
    {
        ALL,
        ISA_LRP,
        ISA_Inst_Reserved,
        "lrp",
        6,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_FLOAT_ONLY},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
        },

    },

    /// 15
    {
        ALL,
        ISA_MOD,
        ISA_Inst_Arith,
        "mod",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
        },

    },

    /// 16
    {
        ALL,
        ISA_MUL,
        ISA_Inst_Arith,
        "mul",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL | TYPE_INTEGER, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL | TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL | TYPE_INTEGER, 0},
        },

    },

    /// 17
    {
        ALL,
        ISA_POW,
        ISA_Inst_Arith,
        "pow",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
        },

    },

    /// 18
    {
        ALL,
        ISA_RNDD,
        ISA_Inst_Arith,
        "rndd",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
        },

    },

    /// 19
    {
        ALL,
        ISA_RNDU,
        ISA_Inst_Arith,
        "rndu",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
        },

    },

    /// 20
    {
        ALL,
        ISA_RNDE,
        ISA_Inst_Arith,
        "rnde",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
        },

    },

    /// 21
    {
        ALL,
        ISA_RNDZ,
        ISA_Inst_Arith,
        "rndz",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
        },

    },

    /// 22
    {
        ALL,
        ISA_SAD2,
        ISA_Inst_Arith,
        "sad2",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_W | ISA_TYPE_UW,
             SAT_C | HORIZON_STRIDE_2},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_B | ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_B | ISA_TYPE_UB, 0},
        },

    },

    /// 23
    {
        ALL,
        ISA_SIN,
        ISA_Inst_Arith,
        "sin",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
        },

    },

    /// 24
    {
        ALL,
        ISA_COS,
        ISA_Inst_Arith,
        "cos",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
        },

    },

    /// 25
    {
        ALL,
        ISA_SQRT,
        ISA_Inst_Arith,
        "sqrt",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
        },

    },

    /// 26
    {
        ALL,
        ISA_RSQRT,
        ISA_Inst_Arith,
        "rsqrt",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
        },

    },

    /// 27
    {
        ALL,
        ISA_INV,
        ISA_Inst_Arith,
        "inv",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT_ALL, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT_ALL, 0},
        },

    },

    /// 28 (0x1C)
    {
        Xe_XeHPSDV,
        ISA_DPASW,
        ISA_Inst_Misc,
        "dpasw",
        6,
        SAME_DATA_TYPE,
        {{OPND_EXECSIZE, ISA_TYPE_UB, 0},
         {OPND_DST_GEN, ISA_TYPE_D | ISA_TYPE_UD | ISA_TYPE_F, GRF_ALIGNED},
         {OPND_SRC_GEN, ISA_TYPE_D | ISA_TYPE_UD | ISA_TYPE_F, GRF_ALIGNED},
         {OPND_SRC_GEN, ISA_TYPE_D | ISA_TYPE_UD, GRF_ALIGNED},
         {OPND_SRC_GEN, ISA_TYPE_D | ISA_TYPE_UD, GRF_ALIGNED},
         {OPND_OTHER, ISA_TYPE_UD, 0}},
    },

    /// 29 (0x1D)
    {
        Xe_PVC,
        ISA_FCVT,
        ISA_Inst_Mov,
        "fcvt",
        3,
        SAME_SPECIAL_KIND,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_DST_GEN,
             ISA_TYPE_UB | ISA_TYPE_B | ISA_TYPE_UD | ISA_TYPE_HF | ISA_TYPE_F,
             0},
            {OPND_SRC_GEN,
             ISA_TYPE_UB | ISA_TYPE_B | ISA_TYPE_UD | ISA_TYPE_HF | ISA_TYPE_F,
             0},
        },
    },

    /// 30 (0x1E)
    // FIXME, ISA_TYPE_F | ISA_TYPE_HF of src1 is to be WA the LIT test of VC
    {
        Xe_PVCXT,
        ISA_SRND,
        ISA_Inst_Arith,
        "srnd",
        4,
        SAME_SPECIAL_KIND,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_DST_GEN, ISA_TYPE_UB | ISA_TYPE_B | ISA_TYPE_HF, 0},
            {OPND_SRC_GEN, ISA_TYPE_HF | ISA_TYPE_BF | ISA_TYPE_F, 0},
            {OPND_SRC_GEN | OPND_IMM,
             ISA_TYPE_UB | ISA_TYPE_UW | ISA_TYPE_F | ISA_TYPE_HF, 0},
        },
    },
    /// 31
    {
        ALL,
        ISA_LZD,
        ISA_Inst_Arith,
        "lzd",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, 0},
        },

    },

    /// 32
    {
        ALL,
        ISA_AND,
        ISA_Inst_Logic,
        "and",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, PREDICATE_NONEPRED_OPND},
            {OPND_VECTOR_DST_G_I_P, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_P_AO, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_P_AO, TYPE_INTEGER, 0},
        },

    },

    /// 33
    {
        ALL,
        ISA_OR,
        ISA_Inst_Logic,
        "or",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, PREDICATE_NONEPRED_OPND},
            {OPND_VECTOR_DST_G_I_P, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_P_AO, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_P_AO, TYPE_INTEGER, 0},
        },

    },

    /// 34
    {
        ALL,
        ISA_XOR,
        ISA_Inst_Logic,
        "xor",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, PREDICATE_NONEPRED_OPND},
            {OPND_VECTOR_DST_G_I_P, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_P_AO, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_P_AO, TYPE_INTEGER, 0},
        },

    },

    /// 35
    {
        ALL,
        ISA_NOT,
        ISA_Inst_Logic,
        "not",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, PREDICATE_NONEPRED_OPND},
            {OPND_VECTOR_DST_G_I_P, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_P_AO, TYPE_INTEGER, 0},
        },

    },

    /// 36
    {
        ALL,
        ISA_SHL,
        ISA_Inst_Logic,
        "shl",
        5,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, PREDICATE_NONEPRED_OPND},
            {OPND_VECTOR_DST_G_I, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
        },

    },

    /// 37
    {
        ALL,
        ISA_SHR,
        ISA_Inst_Logic,
        "shr",
        5,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, PREDICATE_NONEPRED_OPND},
            {OPND_VECTOR_DST_G_I, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
        },

    },

    /// 38
    {
        ALL,
        ISA_ASR,
        ISA_Inst_Logic,
        "asr",
        5,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, PREDICATE_NONEPRED_OPND},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_B | ISA_TYPE_W | ISA_TYPE_D, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_B | ISA_TYPE_W | ISA_TYPE_D,
             0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
        },

    },

    /// 39
    {
        ALL,
        ISA_CBIT,
        ISA_Inst_Logic,
        "cbit",
        4,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, PREDICATE_NONEPRED_OPND},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
        },

    },

    /// 40
    {
        ALL,
        ISA_ADDR_ADD,
        ISA_Inst_Address,
        "addr_add",
        4,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_DST_ADDR, 0, 0},
            {OPND_VECTOR_SRC_G_A_AO, 0, SCALAR_REGION},
            {OPND_VECTOR_SRC_G_IMM_AO, ISA_TYPE_UW, 0},
        },

    },

    /// 41
    {
        ALL,
        ISA_MOV,
        ISA_Inst_Mov,
        "mov",
        4,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, 0, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, 0, 0},
        },

    },

    /// 42
    {
        ALL,
        ISA_SEL,
        ISA_Inst_Mov,
        "sel",
        5,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, 0, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, 0, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, 0, 0},
        },

    },

    /// 43
    {
        ALL,
        ISA_SETP,
        ISA_Inst_Mov,
        "setp",
        3,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_DST_PRED, 0, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_INTEGER, 0},
        },

    },

    /// 44
    {
        ALL,
        ISA_CMP,
        ISA_Inst_Compare,
        "cmp",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_CMP_SUBOP, ISA_TYPE_UB, 0},
            {OPND_DST_PRED | OPND_DST_GEN, 0, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, 0, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, 0, 0},
        },

    },

    /// 45
    {
        ALL,
        ISA_MOVS,
        ISA_Inst_Mov,
        "movs",
        3,
        SAME_SPECIAL_KIND,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_SPECIAL, ISA_TYPE_UW, 0},
            {OPND_SPECIAL, ISA_TYPE_UW, 0},
        },

    },

    /// 46
    {
        ALL,
        ISA_FBL,
        ISA_Inst_Logic,
        "fbl",
        4,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, 0},
        },

    },

    /// 47
    {
        ALL,
        ISA_FBH,
        ISA_Inst_Logic,
        "fbh",
        4,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD | ISA_TYPE_D, 0},
        },

    },

    /// 48
    {
        ALL,
        ISA_SUBROUTINE,
        ISA_Inst_Flow,
        "func",
        1,
        0,
        {
            {OPND_LABEL, ISA_TYPE_UW, 0},
        },

    },

    /// 49
    {
        ALL,
        ISA_LABEL,
        ISA_Inst_Flow,
        "label",
        1,
        0,
        {
            {OPND_LABEL, ISA_TYPE_UW, 0},
        },

    },

    /// 50
    {
        ALL,
        ISA_JMP,
        ISA_Inst_Flow,
        "jmp",
        3,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_LABEL, ISA_TYPE_UW, LABEL_BLOCK_C},
        },

    },

    /// 51
    {
        ALL,
        ISA_CALL,
        ISA_Inst_Flow,
        "call",
        3,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_LABEL, ISA_TYPE_UW, LABEL_FUNC_C},
        },

    },

    /// 52
    {
        ALL,
        ISA_RET,
        ISA_Inst_Flow,
        "ret",
        2,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},
        },

    },

    /// 53
    {
        ALL,
        ISA_OWORD_LD,
        ISA_Inst_Data_Port,
        "oword_ld",
        5,
        0,
        {
            {OPND_OWORD_SIZE, ISA_TYPE_UB, 0},
            {OPND_IS_MODIFIED, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION},
            {OPND_RAW, 0, GE_READSIZE},
        },

    },

    /// 54
    {
        ALL,
        ISA_OWORD_ST,
        ISA_Inst_Data_Port,
        "oword_st",
        4,
        0,
        {
            {OPND_OWORD_SIZE, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION},
            {OPND_RAW, 0, GE_WRITESIZE},
        },

    },

    /// 55
    {
        ALL,
        ISA_MEDIA_LD,
        ISA_Inst_Data_Port,
        "media_ld",
        8,
        0,
        {
            {OPND_MEDIA_LD_MODIFIER, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_PLANE, ISA_TYPE_UB, VALUE_0_3},
            {OPND_BLOCK_WIDTH, ISA_TYPE_UB, VALUE_1_32},
            {OPND_BLOCK_HEIGHT, ISA_TYPE_UB, VALUE_1_64},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION},
            {OPND_RAW, 0, GRF_ALIGNED | SIZE_GE_WIDTH_M_HIEGH},
        },

    },

    /// 56
    {
        ALL,
        ISA_MEDIA_ST,
        ISA_Inst_Data_Port,
        "media_st",
        8,
        0,
        {
            {OPND_MEDIA_ST_MODIFIER, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_PLANE, ISA_TYPE_UB, VALUE_0_3},
            {OPND_BLOCK_WIDTH, ISA_TYPE_UB, VALUE_1_32},
            {OPND_BLOCK_HEIGHT, ISA_TYPE_UB, VALUE_1_64},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION},
            {OPND_RAW, 0, GRF_ALIGNED | SIZE_GE_WIDTH_M_HIEGH},
        },

    },

    /// 57
    {
        ALL,
        ISA_GATHER,
        ISA_Inst_Data_Port,
        "gather",
        7,
        0,
        {
            {OPND_ELEM_SIZE, ISA_TYPE_UB, 0},
            {OPND_IS_MODIFIED, ISA_TYPE_UB, 0},
            {OPND_ELEM_NUM, ISA_TYPE_UB, ELEM_NUM_8_16},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION},
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_UD | ISA_TYPE_D | ISA_TYPE_F, GRF_ALIGNED},
        },

    },

    /// 58
    {
        ALL,
        ISA_SCATTER,
        ISA_Inst_Data_Port,
        "scatter",
        6,
        0,
        {
            {OPND_ELEM_SIZE, ISA_TYPE_UB, 0},
            {OPND_ELEM_NUM, ISA_TYPE_UB, ELEM_NUM_8_16},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION},
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_UD | ISA_TYPE_D | ISA_TYPE_F, GRF_ALIGNED},
        },

    },

    /// 59
    {
        ALL,
        ISA_RESERVED_3B,
        ISA_Inst_Reserved,
        "reserved_3B",
        8,
        0,
        {},

    },

    /// 60
    {
        ALL,
        ISA_OWORD_LD_UNALIGNED,
        ISA_Inst_Data_Port,
        "oword_ld_unaligned",
        5,
        0,
        {
            {OPND_OWORD_SIZE, ISA_TYPE_UB, 0},
            {OPND_IS_MODIFIED, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION},
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED | GE_READSIZE},
        },

    },

    /// 61
    {
        ALL,
        ISA_RESERVED_3D,
        ISA_Inst_Reserved,
        "reserved_3D",
        0,
        0,
        {},
    },

    /// 62
    {
        ALL,
        ISA_RESERVED_3E,
        ISA_Inst_Reserved,
        "reserved_3E",
        0,
        0,
        {},
    },

    /// 63
    {
        ALL,
        ISA_RESERVED_3F,
        ISA_Inst_Reserved,
        "reserved_3F",
        0,
        0,
        {},
    },

    /// 64
    {
        ALL,
        ISA_SAMPLE,
        ISA_Inst_Sampler,
        "sample",
        7,
        0,
        {
            {OPND_CHANNEL_SIMD_MODE, ISA_TYPE_UB, 0},
            {OPND_SAMPLE, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},
        },

    },

    /// 65
    {
        ALL,
        ISA_SAMPLE_UNORM,
        ISA_Inst_Sampler,
        "sample_unorm",
        8,
        0,
        {
            {OPND_CHAN_PATT, ISA_TYPE_UB, 0},
            {OPND_SAMPLE, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_F, SCALAR_REGION},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_F, SCALAR_REGION},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_F, SCALAR_REGION},
            {OPND_RAW, ISA_TYPE_UW, GRF_ALIGNED},
        },

    },

    /// 66
    {
        ALL,
        ISA_LOAD,
        ISA_Inst_Sampler,
        "load",
        6,
        0,
        {
            {OPND_CHANNEL_SIMD_MODE, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},
        },

    },

    /// 67
    {
        ALL,
        ISA_AVS,
        ISA_Inst_Sampler,
        "avs",
        15,
        1,
        {
            {OPND_CHANNEL_SIMD_MODE, ISA_TYPE_UB, 0},
            {OPND_SAMPLE, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // deltaU
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // deltaV
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // u2d
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // groupID
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F,
             SCALAR_REGION},              // verticalBlockNumber
            {OPND_OTHER, ISA_TYPE_UB, 0}, // cntrl
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // v2d
            {OPND_OTHER, ISA_TYPE_UB, 0},                         // execMode
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW | ISA_TYPE_UD,
             SCALAR_REGION},                     // IEFBypass
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED}, // dst
        },

    },

    /// 68
    {
        ALL,
        ISA_VA,
        ISA_Inst_Sampler,
        "va",
        1,
        1,
        {{OPND_SUBOPCODE, ISA_TYPE_UB, 0}},

    },

    /// 69
    {
        ALL,
        ISA_FMINMAX,
        ISA_Inst_Mov,
        "fminmax",
        5,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OP_EXT, ISA_TYPE_UB, 0},
            {OPND_VECTOR_DST_G_I, 0, 0},
            {OPND_VECTOR_SRC_G_I_IMM, 0, 0},
            {OPND_VECTOR_SRC_G_I_IMM, 0, 0},

        },

    },

    /// 70
    {
        ALL,
        ISA_BFE,
        ISA_Inst_Logic,
        "bfe",
        6,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, ///  exec_size
            {OPND_PRED, ISA_TYPE_UW, 0},     ///  predicate
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},

        },

    },

    /// 71
    {
        ALL,
        ISA_BFI,
        ISA_Inst_Logic,
        "bfi",
        7,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, ///  exec_size
            {OPND_PRED, ISA_TYPE_UW, 0},     ///  predicate
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},

        },

    },

    /// 72
    {
        ALL,
        ISA_BFREV,
        ISA_Inst_Logic,
        "bfrev",
        4,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, ///  exec_size
            {OPND_PRED, ISA_TYPE_UW, 0},     ///  predicate
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD | ISA_TYPE_D, GRF_ALIGNED},
        },

    },

    /// 73
    {
        ALL,
        ISA_ADDC,
        ISA_Inst_Arith,
        "addc",
        6,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, ///  exec_size
            {OPND_PRED, ISA_TYPE_UW, 0},     ///  predicate
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD, GRF_ALIGNED},

        },

    },

    /// 74
    {
        ALL,
        ISA_SUBB,
        ISA_Inst_Arith,
        "subb",
        6,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, ///  exec_size
            {OPND_PRED, ISA_TYPE_UW, 0},     ///  predicate
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD, GRF_ALIGNED},

        },

    },

    /// 75
    {
        ALL,
        ISA_GATHER4_TYPED,
        ISA_Inst_Data_Port,
        "gather4_typed",
        9,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// exec_size
            {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
            {OPND_OTHER, ISA_TYPE_UB, 0},    /// channel mask
            {OPND_SURFACE, ISA_TYPE_UB, 0},  /// Surface
            {OPND_RAW, ISA_TYPE_UD, 0},      /// uOffset
            {OPND_RAW, ISA_TYPE_UD, 0},      /// vOffset
            {OPND_RAW, ISA_TYPE_UD, 0},      /// rOffset
            {OPND_RAW, ISA_TYPE_UD, 0},      /// lod
            {OPND_RAW, ISA_TYPE_UD | ISA_TYPE_D | ISA_TYPE_F, 0}, /// dst
        },

    },

    /// 76
    {
        ALL,
        ISA_SCATTER4_TYPED,
        ISA_Inst_Data_Port,
        "scatter4_typed",
        9,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// exec_size
            {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
            {OPND_OTHER, ISA_TYPE_UB, 0},    /// channel mask
            {OPND_SURFACE, ISA_TYPE_UB, 0},  /// Surface
            {OPND_RAW, ISA_TYPE_UD, 0},      /// uOffset
            {OPND_RAW, ISA_TYPE_UD, 0},      /// vOffset
            {OPND_RAW, ISA_TYPE_UD, 0},      /// rOffset
            {OPND_RAW, ISA_TYPE_UD, 0},      /// lod
            {OPND_RAW, ISA_TYPE_UD | ISA_TYPE_D | ISA_TYPE_F, 0}, /// dst
        },

    },

    /// 77
    {
        ALL,
        ISA_VA_SKL_PLUS,
        ISA_Inst_Sampler,
        "va_skl_plus",
        1,
        0,
        {{OPND_SUBOPCODE, ISA_TYPE_UB, 0}},

    },

    /// 78
    {
        ALL,
        ISA_SVM,
        ISA_Inst_SVM,
        "svm",
        1,
        0,
        {{OPND_SUBOPCODE, ISA_TYPE_UB, 0}},

    },

    /// 79
    {
        ALL,
        ISA_IFCALL,
        ISA_Inst_Flow,
        "ifcall",
        6,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_KIND, ISA_TYPE_UB, 0}, /// isUniform
            {OPND_KIND, ISA_TYPE_UD, 0}, /// function_addr
            {OPND_KIND, ISA_TYPE_UB, 0}, /// arg_size
            {OPND_KIND, ISA_TYPE_UB, 0}, /// return_size
        },

    },

    /// 80
    {
        ALL,
        ISA_FADDR,
        ISA_Inst_Flow,
        "faddr",
        2,
        0,
        {
            {OPND_KIND, ISA_TYPE_UW, 0}, /// symbol name: index to string table
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD, 0},
        },

    },

    /// 81
    {
        ALL,
        ISA_FILE,
        ISA_Inst_Misc,
        "file",
        1,
        0,
        {
            {OPND_STRING, ISA_TYPE_UD, LENGHT_LESS_256},
        },

    },

    /// 82
    {
        ALL,
        ISA_LOC,
        ISA_Inst_Misc,
        "loc",
        1,
        0,
        {
            {OPND_IMM, ISA_TYPE_UD, 0}, /// NOT finished
        },

    },

    /// 83
    {
        ALL,
        ISA_RESERVED_53,
        ISA_Inst_Reserved,
        "reserved_53",
        0,
        0,
        {

        },
    },

    /// 84
    {
        ALL,
        ISA_VME_IME,
        ISA_Inst_Misc,
        "vme_ime",
        9,
        0, /// VME_IME
        {
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_96},
            {OPND_RAW, ISA_TYPE_UB, SIZE_STREAM_MODE_DEPENDENT_1},
            {OPND_STREAM_MODE, ISA_TYPE_UB, 0},
            {OPND_SEARCH_CRTL, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_RAW, ISA_TYPE_UW, ELEM_NUM_GE_2},
            {OPND_RAW, ISA_TYPE_UW, ELEM_NUM_GE_2},
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_4},
            {OPND_RAW, ISA_TYPE_UB, SIZE_STREAM_MODE_DEPENDENT_2},
        },

    },

    /// 85
    {
        ALL,
        ISA_VME_SIC,
        ISA_Inst_Misc,
        "vme_sic",
        4,
        0, /// VME_SIC
        {
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_96},
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_128},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_224},
        },

    },

    /// 86
    {
        ALL,
        ISA_VME_FBR,
        ISA_Inst_Misc,
        "vme_fbr",
        7,
        0, /// VME_FBR
        {
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_96},
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_128},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_MATRIX_MODE, ISA_TYPE_UB, 0},
            {OPND_SUBMATRIX_SHAPE, ISA_TYPE_UB, 0},
            {OPND_SUBPRE_SHAPE, ISA_TYPE_UB, 0},
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_224},
        },

    },

    /// 87
    ///  FIXME: fix ELEM_NUM type
    {
        GENX_SKL,
        ISA_VME_IDM,
        ISA_Inst_Misc,
        "vme_idm",
        4,
        0, ///  VME_IDM
        {
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_128},
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_GE_32},
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_RAW, ISA_TYPE_UB, ELEM_NUM_GE_32},
        },

    },

    /// 88
    {
        ALL,
        ISA_RESERVED_58,
        ISA_Inst_Reserved,
        "reserved_58",
        0,
        0,
        {},

    },

    /// 89
    {
        ALL,
        ISA_BARRIER,
        ISA_Inst_Sync,
        "barrier",
        0,
        0,
        {},

    },

    /// 90
    {
        ALL,
        ISA_SAMPLR_CACHE_FLUSH,
        ISA_Inst_Sync,
        "samplr_cache_flush",
        0,
        0,
        {},

    },

    /// 91
    {
        ALL,
        ISA_WAIT,
        ISA_Inst_Reserved,
        "wait",
        1,
        0,
        {
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, 0} // mask
        },

    },

    /// 92
    {
        ALL,
        ISA_FENCE,
        ISA_Inst_Reserved,
        "fence",
        1,
        0,
        {
            {OPND_IMM, ISA_TYPE_UB, 0}, /// mode
        },

    },

    /// 93
    {
        ALL,
        ISA_RAW_SEND,
        ISA_Inst_Misc,
        "raw_send",
        9,
        0,
        {
            {OPND_IMM, ISA_TYPE_UB, 0},                /// modifier
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},           ///  exec_size
            {OPND_PRED, ISA_TYPE_UW, 0},               ///  predicate
            {OPND_IMM, ISA_TYPE_UD, 0},                /// exMsgDesc
            {OPND_IMM, ISA_TYPE_UB, 0},                /// numSrcs
            {OPND_IMM, ISA_TYPE_UB, 0},                /// numDst
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD, 0}, /// Desc
            {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED},      /// src
            {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}       /// dst
        },

    },

    /// 94
    {
        ALL,
        ISA_RESERVED_5E,
        ISA_Inst_Reserved,
        "reserved_5e",
        0,
        0,
        {},

    },

    /// 95
    {
        ALL,
        ISA_YIELD,
        ISA_Inst_Sync,
        "yield",
        0,
        0,
        {},

    },

    /// 96 (0x60)
    {
        Xe_PVC,
        ISA_NBARRIER,
        ISA_Inst_Sync,
        "nbarrier",
        5,
        0,
        {
            {OPND_IMM, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, 0},
        },
    },

    /// 97
    {
        ALL,
        ISA_RESERVED_61,
        ISA_Inst_Reserved,
        "reserved_61",
        0,
        0,
        {},

    },

    /// 98
    {
        ALL,
        ISA_RESERVED_62,
        ISA_Inst_Reserved,
        "reserved_62",
        0,
        0,
        {},

    },

    /// 99
    {
        ALL,
        ISA_RESERVED_63,
        ISA_Inst_Reserved,
        "reserved_63",
        0,
        0,
        {},

    },

    /// 100
    {
        ALL,
        ISA_RESERVED_64,
        ISA_Inst_Reserved,
        "reserved_64",
        0,
        0,
        {},

    },

    /// 101
    {
        ALL,
        ISA_RESERVED_65,
        ISA_Inst_Reserved,
        "reserved_65",
        0,
        0,
        {},

    },

    /// 102
    {
        ALL,
        ISA_RESERVED_66,
        ISA_Inst_Reserved,
        "reserved_66",
        0,
        0,
        {},
    },

    /// 103
    /// minimum number of operands is 5. Argumetns are extra
    {
        ALL,
        ISA_FCALL,
        ISA_Inst_Flow,
        "fcall",
        5,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_KIND, ISA_TYPE_UW, 0}, /// function_id
            {OPND_KIND, ISA_TYPE_UB, 0}, /// arg_size
            {OPND_KIND, ISA_TYPE_UB, 0}, /// return_size
        },

    },

    /// 104
    {
        ALL,
        ISA_FRET,
        ISA_Inst_Flow,
        "fret",
        2,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
        },

    },

    /// 105
    {
        ALL,
        ISA_SWITCHJMP,
        ISA_Inst_Flow,
        "switchjmp",
        4,
        0, /// max size
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_IMM, ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_F, SCALAR_REGION},
            {OPND_LABEL, ISA_TYPE_UW,
             LABEL_BLOCK_C}, /// actual number of labels can be [1, 32]
        },

    },

    /// 106
    {
        ALL,
        ISA_SAD2ADD,
        ISA_Inst_Arith,
        "sad2add",
        6,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_W | ISA_TYPE_UW,
             SAT_C | HORIZON_STRIDE_2},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_B | ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_B | ISA_TYPE_UB, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_B | ISA_TYPE_UB, 0},
        },

    },

    /// 107
    {
        ALL,
        ISA_PLANE,
        ISA_Inst_Arith,
        "plane",
        5,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_F, 0},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, 0},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, 0},
        },

    },

    /// 108
    {
        ALL,
        ISA_GOTO,
        ISA_Inst_SIMD_Flow,
        "goto",
        3,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_LABEL, ISA_TYPE_UW, LABEL_BLOCK_C},
        },

    },

    /// 109
    {
        ALL,
        ISA_3D_SAMPLE,
        ISA_Inst_Sampler,
        "3d_sample",
        12,
        0,
        {
            {OPND_OTHER, ISA_TYPE_UW, 0},
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_OTHER, ISA_TYPE_UB, 0}, /// Channel
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION}, /// aoffimmi
            {OPND_SAMPLE | OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB | ISA_TYPE_UQ, 0}, /// sampler
            {OPND_OTHER, ISA_TYPE_UD, 0}, /// reserved
            {OPND_SURFACE | OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB | ISA_TYPE_UQ, 0}, /// surface
            {OPND_OTHER, ISA_TYPE_UD, 0}, /// reserved
            {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED}, /// Destination
            {OPND_RAW, ISA_TYPE_UD, 0},   /// Paired Surface
            {OPND_OTHER, ISA_TYPE_UB, 0}, ///  numberMsgSpecific Operands
        },

    },

    /// 110
    {
        ALL,
        ISA_3D_LOAD,
        ISA_Inst_Sampler,
        "3d_load",
        10,
        0,
        {
            {OPND_OTHER, ISA_TYPE_UW, 0},
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_OTHER, ISA_TYPE_UB, 0}, /// Channel
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION}, /// aoffimmi
            {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR | OPND_SURFACE, ISA_TYPE_UB | ISA_TYPE_UQ, 0}, /// surface
            {OPND_OTHER, ISA_TYPE_UD, 0}, /// reserved
            {OPND_RAW, ISA_TYPE_F | ISA_TYPE_UD | ISA_TYPE_D, 0}, /// Destination
            {OPND_RAW, ISA_TYPE_UD, 0},   /// Paired Surface
            {OPND_OTHER, ISA_TYPE_UB, 0}, ///  numberMsgSpecific Operands
        },

    },

    /// 111
    {
        ALL,
        ISA_3D_GATHER4,
        ISA_Inst_Sampler,
        "3d_gather4",
        12,
        0,
        {
            {OPND_OTHER, ISA_TYPE_UW, 0},
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_OTHER, ISA_TYPE_UB, 0}, /// Channel
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD, SCALAR_REGION}, /// aoffimmi
            {OPND_SAMPLE | OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB | ISA_TYPE_UQ, 0}, /// sampler
            {OPND_OTHER, ISA_TYPE_UD, 0}, /// reserved
            {OPND_SURFACE | OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB | ISA_TYPE_UQ, 0}, /// surface
            {OPND_OTHER, ISA_TYPE_UD, 0}, /// reserved
            {OPND_RAW, ISA_TYPE_F | ISA_TYPE_UD | ISA_TYPE_D, 0}, /// Destination
            {OPND_RAW, ISA_TYPE_UD, 0},   /// Paired Surface
            {OPND_OTHER, ISA_TYPE_UB, 0}, ///  numberMsgSpecific Operands
        },
    },

    /// 112
    {
        ALL,
        ISA_3D_INFO,
        ISA_Inst_Sampler,
        "3d_info",
        6,
        0,
        {
            {OPND_OTHER, ISA_TYPE_UW, 0}, /// subOpcode
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_OTHER, ISA_TYPE_UB, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0}, /// Surface
            {OPND_OTHER, ISA_TYPE_UD, 0}, /// reserved
            {OPND_RAW, ISA_TYPE_UD, 0}, /// LOD
                 // LOD is only for one of the opcodes, it will be handled in
                 // the builder interface, and operand number incremented.
            {OPND_RAW, ISA_TYPE_UD, 0} /// dst
        },

    },

    /// 113
    {
        ALL,
        ISA_3D_RT_WRITE,
        ISA_Inst_Sampler,
        "3d_rt_write",
        4,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},    /// predicate
            {OPND_OTHER, ISA_TYPE_UW, 0},   /// mode
            {OPND_SURFACE | OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB | ISA_TYPE_Q | ISA_TYPE_UQ, 0}, /// Surface
        },

    },

    /// 114
    {
        ALL,
        ISA_3D_URB_WRITE,
        ISA_Inst_Sampler,
        "3d_urb_write",
        8,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_OTHER, ISA_TYPE_UB, 0}, /// number of output parameters
            {OPND_RAW, ISA_TYPE_UD, 0},   /// channel mask
            {OPND_OTHER, ISA_TYPE_UW, 0}, /// global offset
            {OPND_RAW, ISA_TYPE_UD, 0},   /// URB Handle
            {OPND_RAW, ISA_TYPE_UD, 0},   /// per slot offset
            {OPND_RAW, ISA_TYPE_UD | ISA_TYPE_D | ISA_TYPE_F,
             0} ///  vertex data
        },

    },

    /// 115
    {
        ALL,
        ISA_3D_TYPED_ATOMIC,
        ISA_Inst_Data_Port,
        "typed_atomic",
        11,
        0,
        {
            {OPND_OTHER, ISA_TYPE_UB, 0}, /// sub opcode
            {OPND_EXECSIZE, ISA_TYPE_UB, SIZE_1},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_SURFACE, ISA_TYPE_UB, 0},         /// Surface
            {OPND_RAW, ISA_TYPE_UD, 0},             /// U
            {OPND_RAW, ISA_TYPE_UD, 0},             /// V
            {OPND_RAW, ISA_TYPE_UD, 0},             /// R
            {OPND_RAW, ISA_TYPE_UD, 0},             /// LOD
            {OPND_RAW, ISA_TYPE_UD, 0},             /// Src0
            {OPND_RAW, ISA_TYPE_UD, 0},             /// Src1
            {OPND_RAW, ISA_TYPE_UD | ISA_TYPE_D, 0} /// dst
        },

    },

    /// 116
    {
        ALL,
        ISA_GATHER4_SCALED,
        ISA_Inst_Data_Port,
        "gather4_scaled",
        8,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// execution size
            {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
            {OPND_OTHER, ISA_TYPE_UB, 0},    /// channel mask
            {OPND_OTHER, ISA_TYPE_UW, 0},    /// scale
            {OPND_SURFACE, ISA_TYPE_UB, 0},  /// surface
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD,
             SCALAR_REGION},                      /// global offset
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED}, /// offsets
            {OPND_RAW, ISA_TYPE_F | ISA_TYPE_D | ISA_TYPE_UD,
             GRF_ALIGNED} /// dst
        },

    },

    /// 117
    {
        ALL,
        ISA_SCATTER4_SCALED,
        ISA_Inst_Data_Port,
        "scatter4_scaled",
        8,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// execution size
            {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
            {OPND_OTHER, ISA_TYPE_UB, 0},    /// channel mask
            {OPND_OTHER, ISA_TYPE_UW, 0},    /// scale
            {OPND_SURFACE, ISA_TYPE_UB, 0},  /// surface
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD,
             SCALAR_REGION},                      /// global offset
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED}, /// offsets
            {OPND_RAW, ISA_TYPE_F | ISA_TYPE_D | ISA_TYPE_UD,
             GRF_ALIGNED} /// dst
        },

    },

    /// 118
    {
        ALL,
        ISA_RESERVED_76,
        ISA_Inst_Reserved,
        "reserved_76",
        0,
        0,
        {},

    },

    /// 119
    {
        ALL,
        ISA_RESERVED_77,
        ISA_Inst_Reserved,
        "reserved_77",
        0,
        0,
        {},

    },

    /// 120 (0x78)
    {
        ALL,
        ISA_GATHER_SCALED,
        ISA_Inst_Data_Port,
        "gather_scaled",
        9,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// execution size
            {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
            {OPND_OTHER, ISA_TYPE_UB, 0},    /// block size
            {OPND_OTHER, ISA_TYPE_UB, 0},    /// num of blocks
            {OPND_OTHER, ISA_TYPE_UW, 0},    /// scale
            {OPND_SURFACE, ISA_TYPE_UB, 0},  /// surface
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD,
             SCALAR_REGION},                      /// global offset
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED}, /// offsets
            {OPND_RAW, ISA_TYPE_F | ISA_TYPE_D | ISA_TYPE_UD,
             GRF_ALIGNED} /// dst
        },

    },

    /// 121 (0x79)
    {
        ALL,
        ISA_SCATTER_SCALED,
        ISA_Inst_Data_Port,
        "scatter_scaled",
        9,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// execution size
            {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
            {OPND_OTHER, ISA_TYPE_UB, 0},    /// block size
            {OPND_OTHER, ISA_TYPE_UB, 0},    /// num of blocks
            {OPND_OTHER, ISA_TYPE_UW, 0},    /// scale
            {OPND_SURFACE, ISA_TYPE_UB, 0},  /// surface
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD,
             SCALAR_REGION},                      /// global offset
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED}, /// offsets
            {OPND_RAW, ISA_TYPE_F | ISA_TYPE_D | ISA_TYPE_UD,
             GRF_ALIGNED} /// dst
        },

    },

    /// 122
    {
        ALL,
        ISA_RAW_SENDS,
        ISA_Inst_Misc,
        "raw_sends",
        12,
        0,
        {
            {OPND_IMM, ISA_TYPE_UB, 0},                /// modifier
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},           ///  exec_size
            {OPND_PRED, ISA_TYPE_UW, 0},               ///  predicate
            {OPND_IMM, ISA_TYPE_UB, 0},                /// numSrc0
            {OPND_IMM, ISA_TYPE_UB, 0},                /// numSrc1
            {OPND_IMM, ISA_TYPE_UB, 0},                /// numDst
            {OPND_IMM, ISA_TYPE_UB, 0},                /// FFID
            {OPND_IMM, ISA_TYPE_UD, 0},                /// exMsgDesc
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UD, 0}, /// Desc
            {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED},      /// src0
            {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED},      /// src1
            {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}       /// dst
        },

    },

    /// 123 (0x7B)
    {
        ALL,
        ISA_LIFETIME,
        ISA_Inst_Misc,
        "lifetime",
        2,
        0,
        {
            {OPND_IMM, ISA_TYPE_UB, 0}, // properties
            {OPND_IMM, ISA_TYPE_UD, 0}  // variable id
        },
    },

    /// 124 (0x7C)
    {
        ALL,
        ISA_SBARRIER,
        ISA_Inst_Sync,
        "sbarrier",
        1,
        0,
        {
            {OPND_IMM, ISA_TYPE_UB, 0}, // signal/wait
        },

    },

    /// 125 (0x7D)
    {
        ALL,
        ISA_DWORD_ATOMIC,
        ISA_Inst_Data_Port,
        "dword_atomic",
        8,
        0,
        {
            {OPND_ATOMIC_SUBOP, ISA_TYPE_UB, 0},
            {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// execution size
            {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
            {OPND_SURFACE, ISA_TYPE_UB, 0},
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_D | ISA_TYPE_F | ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_D | ISA_TYPE_F | ISA_TYPE_UD, GRF_ALIGNED},
            {OPND_RAW, ISA_TYPE_D | ISA_TYPE_F | ISA_TYPE_UD, GRF_ALIGNED},
        },

    },

    /// 126 (0x7E)
    {
        ALL,
        ISA_SQRTM,
        ISA_Inst_Arith,
        "sqrtm",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_DF, SAT_C},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_DF, 0},
        },

    },

    /// 127 (0x7F)
    {
        ALL,
        ISA_DIVM,
        ISA_Inst_Arith,
        "divm",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, TYPE_FLOAT, SAT_FLOAT_ONLY},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, TYPE_FLOAT, 0},
        },
    },

    /// 128 (0x80)
    {
        GENX_ICLLP,
        ISA_ROL,
        ISA_Inst_Logic,
        "rol",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
        },
    },

    /// 129 (0x81)
    {
        GENX_ICLLP,
        ISA_ROR,
        ISA_Inst_Logic,
        "ror",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
        },
    },

    /// 130 (0x82)
    {
        GENX_TGLLP,
        ISA_DP4A,
        ISA_Inst_Arith,
        "dp4a",
        6,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I, ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I, ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_D | ISA_TYPE_UD, 0},
        },
    },

    /// 131 (0x83)
    {
        Xe_XeHPSDV,
        ISA_DPAS,
        ISA_Inst_Misc,
        "dpas",
        6,
        SAME_DATA_TYPE,
        {{OPND_EXECSIZE, ISA_TYPE_UB, 0},
         {OPND_DST_GEN, ISA_TYPE_D | ISA_TYPE_UD | TYPE_FLOAT_ALL, GRF_ALIGNED},
         {OPND_SRC_GEN, ISA_TYPE_D | ISA_TYPE_UD | TYPE_FLOAT_ALL, GRF_ALIGNED},
         {OPND_SRC_GEN, ISA_TYPE_D | ISA_TYPE_UD | ISA_TYPE_DF, GRF_ALIGNED},
         {OPND_SRC_GEN, ISA_TYPE_D | ISA_TYPE_UD | ISA_TYPE_DF, GRF_ALIGNED},
         {OPND_OTHER, ISA_TYPE_UD, 0}},
    },

    /// 132 (0x84)
    {
        ALL,
        ISA_ADD3,
        ISA_Inst_Arith,
        "add3",
        6,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
        },
    },

    // 133 (0x85)
    {
        ALL,
        ISA_BFN,
        ISA_Inst_Logic,
        "bfn",
        7,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0}, // dst
            {OPND_VECTOR_SRC_G_I_IMM,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD,
             0}, // src0: reg | imm16
            {OPND_VECTOR_SRC_G_I_IMM,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD,
             0}, // src1: reg
            {OPND_VECTOR_SRC_G_I_IMM,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD,
             0}, // src2: reg | imm16
            {OPND_OTHER, ISA_TYPE_UB,
             0} // BooleanFuncCtrl, must be the last opend
        },
    },

    /// 134 (0x86)
    {
        Xe_XeHPSDV,
        ISA_QW_GATHER,
        ISA_Inst_Data_Port,
        "qw_gather",
        6,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},      /// execution size
            {OPND_PRED, ISA_TYPE_UW, 0},          /// predicate
            {OPND_OTHER, ISA_TYPE_UB, 0},         /// numBlocks
            {OPND_SURFACE, ISA_TYPE_UB, 0},       /// surface
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED}, /// offsets
            {OPND_RAW, ISA_TYPE_DF | ISA_TYPE_UQ, GRF_ALIGNED} /// dst
        },
    },

    /// 135 (0x87)
    {
        Xe_XeHPSDV,
        ISA_QW_SCATTER,
        ISA_Inst_Data_Port,
        "qw_scatter",
        6,
        0,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},      /// execution size
            {OPND_PRED, ISA_TYPE_UW, 0},          /// predicate
            {OPND_OTHER, ISA_TYPE_UB, 0},         /// numBlocks
            {OPND_SURFACE, ISA_TYPE_UB, 0},       /// surface
            {OPND_RAW, ISA_TYPE_UD, GRF_ALIGNED}, /// offsets
            {OPND_RAW, ISA_TYPE_DF | ISA_TYPE_UQ, GRF_ALIGNED} /// src
        },
    },

    /// 136 (0x88)
    {
        ALL,
        ISA_RESERVED_88,
        ISA_Inst_Reserved,
        "reserved_88",
        0,
        0,
        {},
    },

    /// 137 (0x89)
    {
        Xe_DG2,
        ISA_LSC_UNTYPED,
        ISA_Inst_LSC,
        "lsc_untyped",
        1,
        SAME_SPECIAL_KIND,
        {{OPND_SUBOPCODE, ISA_TYPE_UB, 0}},
    },
    /// 138 (0x8A)
    {
        Xe_DG2,
        ISA_LSC_TYPED,
        ISA_Inst_LSC,
        "lsc_typed",
        1,
        SAME_SPECIAL_KIND,
        {{OPND_SUBOPCODE, ISA_TYPE_UB, 0}},
    },
    /// 139 (0x8B)
    {Xe_DG2,
     ISA_LSC_FENCE,
     ISA_Inst_LSC,
     "lsc_fence",
     5,
     0,
     {
         /* execution control */
         {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /* execution size */
         {OPND_PRED, ISA_TYPE_UW, 0},     /* predicate */
         /* caching opts */
         {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_SFID */
         {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_FENCE_OP */
         {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_SCOPE */
     }},
    /// 140 (0x8C)
    {
        ALL,
        ISA_RESERVED_8C,
        ISA_Inst_Reserved,
        "reserved_8c",
        0,
        0,
        {},
    },

    /// 141 (0x8D)
    {
        ALL,
        ISA_RESERVED_8D,
        ISA_Inst_Reserved,
        "reserved_8d",
        0,
        0,
        {},
    },

    /// 142 (0x8E)
    {
        ALL,
        ISA_RESERVED_8E,
        ISA_Inst_Reserved,
        "reserved_8e",
        0,
        0,
        {},
    },
    /// 143 (0x8F)
    {
        ALL,
        ISA_RESERVED_8F,
        ISA_Inst_Reserved,
        "reserved_8f",
        0,
        0,
        {},
    },

    /// 144 (0x90)
    {
        ALL,
        ISA_RESERVED_90,
        ISA_Inst_Reserved,
        "reserved_90",
        0,
        0,
        {},
    },
    /// 145 (0x91)
    {
        ALL,
        ISA_MADW,
        ISA_Inst_Arith,
        "madw",
        6,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_UD | ISA_TYPE_D, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD | ISA_TYPE_D, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD | ISA_TYPE_D, 0},
            {OPND_VECTOR_SRC_G_I_IMM_AO, ISA_TYPE_UD | ISA_TYPE_D, 0},
        },
    },

    /// 146 (0x92)
    {
        ALL,
        ISA_ADD3O,
        ISA_Inst_Arith,
        "add3.o",
        6,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_PRED, ISA_TYPE_UW, 0},
            {OPND_VECTOR_DST_G_I,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
            {OPND_VECTOR_SRC_G_I_IMM,
             ISA_TYPE_W | ISA_TYPE_UW | ISA_TYPE_D | ISA_TYPE_UD, 0},
        },
    },
    /// 147 (0x93)
    {
        ALL,
        ISA_RESERVED_93,
        ISA_Inst_Reserved,
        "reserved_93",
        0,
        0,
        {},
    },
    {
        ALL,
        ISA_BREAKPOINT,
        ISA_Inst_Misc,
        "breakpoint",
        0,
        0,
        { }
    },
    /// 149 (0x95)
    {
        ALL,
        ISA_RESERVED_95,
        ISA_Inst_Reserved,
        "reserved_95",
        0,
        0,
        {},
    },

    // 150 (0x96)
    {
        ALL,
        ISA_RESERVED_96,
        ISA_Inst_Reserved,
        "reserved_96",
        0,
        0,
        {},
    },

    // 151 (0x97)
    {
        ALL,
        ISA_RESERVED_97,
        ISA_Inst_Reserved,
        "reserved_97",
        0,
        0,
        {},
    },

    // 152 (0x98)
    {
        ALL,
        ISA_RESERVED_98,
        ISA_Inst_Reserved,
        "reserved_98",
        0,
        0,
        {},
    },

    // 153 (0x99)
    {
        ALL,
        ISA_RESERVED_99,
        ISA_Inst_Reserved,
        "reserved_99",
        0,
        0,
        {},
    },

    // 154 (0x9A)
    {
        ALL,
        ISA_RESERVED_9A,
        ISA_Inst_Reserved,
        "reserved_9a",
        0,
        0,
        {},
    },

    /// 155 (0x9B)
    {
        ALL,
        ISA_INVM,
        ISA_Inst_Arith,
        "invm",
        5,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_DF | ISA_TYPE_F, 0},      // dst = invm(src0/src1)
            {OPND_DST_PRED, ISA_TYPE_UD | ISA_TYPE_UW, 0},           // flag = (eo) of invm
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_DF | ISA_TYPE_F, 0},  // src0
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_DF | ISA_TYPE_F, 0},  // src1
        },

    },

    /// 156 (0x9C)
    {
        ALL,
        ISA_RSQTM,
        ISA_Inst_Arith,
        "rsqtm",
        4,
        SAME_DATA_TYPE,
        {
            {OPND_EXECSIZE, ISA_TYPE_UB, 0},
            {OPND_VECTOR_DST_G_I, ISA_TYPE_DF | ISA_TYPE_F, 0},      // dst = rsqtm(src0)
            {OPND_DST_PRED, ISA_TYPE_UD | ISA_TYPE_UW, 0},           // flag = (eo) of rsqtm
            {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_DF | ISA_TYPE_F, 0},  // src0
        },

    },

};

static const ISA_SubInst_Desc VASubOpcodeDesc[] = {
    {}, /// AVS subOpcode
    {Convolve_FOPCODE,
     ISA_Inst_Sampler,
     "va_convolve_2d",
     6,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                        /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                         // properties
         {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},                  // dst
     }},
    {MINMAX_FOPCODE,
     ISA_Inst_Sampler,
     "va_minmax",
     5,
     {
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, 0},            // mmfMode
         {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},                  // dst
     }},
    {MINMAXFILTER_FOPCODE,
     ISA_Inst_Sampler,
     "va_minmaxfilter",
     8,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                        /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                         // cntrl
         {OPND_OTHER, ISA_TYPE_UB, 0},                         // execMode
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, 0},            // mmfMode
         {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},                  // dst
     }},
    {ERODE_FOPCODE,
     ISA_Inst_Sampler,
     "va_erode",
     6,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                        /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                         // execMode
         {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},                  // dst
     }},
    {Dilate_FOPCODE,
     ISA_Inst_Sampler,
     "va_dilate",
     6,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                        /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                         // execMode
         {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},                  // dst
     }},
    {BoolCentroid_FOPCODE,
     ISA_Inst_Sampler,
     "va_boolcentroid",
     6,
     {
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // vOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, SCALAR_REGION}, // vSize
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, SCALAR_REGION}, // hSize
         {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},                   // dst
     }},
    {Centroid_FOPCODE,
     ISA_Inst_Sampler,
     "va_centroid",
     5,
     {
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // vOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB, SCALAR_REGION}, // vSize
         {OPND_RAW, ISA_TYPE_F, GRF_ALIGNED},                   // dst
     }},
};

static const ISA_SubInst_Desc VAPlusSubOpcodeDesc[] = {
    {}, /// AVS subOpcode
    {}, // convolve
    {}, // minmax
    {}, // minmaxfilter
    {}, // erode
    {}, // dilate
    {}, // boolCentroid
    {}, // centroid
    {VA_OP_CODE_1D_CONVOLVE_VERTICAL,
     ISA_Inst_Sampler,
     "va_convolve_1d_v",
     6,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                        /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                         // mode
         {OPND_RAW_DST, ISA_TYPE_W, GRF_ALIGNED},              // dst
     }},
    {VA_OP_CODE_1D_CONVOLVE_HORIZONTAL,
     ISA_Inst_Sampler,
     "va_convolve_1d_h",
     6,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                        /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                         // mode
         {OPND_RAW_DST, ISA_TYPE_W, GRF_ALIGNED},              // dst
     }},
    {VA_OP_CODE_1PIXEL_CONVOLVE,
     ISA_Inst_Sampler,
     "va_convolve_1pixel",
     7,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                        /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                         // mode
         {OPND_RAW_SRC, ISA_TYPE_W, GRF_ALIGNED},              // offset
         {OPND_RAW_DST, ISA_TYPE_W, GRF_ALIGNED},              // dst
     }},
    {VA_OP_CODE_FLOOD_FILL,
     ISA_Inst_Sampler,
     "va_floodfill",
     6,
     {
         {OPND_OTHER, ISA_TYPE_UB, 0},            // is8Connect
         {OPND_RAW_SRC, ISA_TYPE_W, GRF_ALIGNED}, // PixelMaskHDirection
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F,
          SCALAR_REGION}, // PixelMaskVDirectionLeft
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F,
          SCALAR_REGION}, // PixelMaskVDirectionRight
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // LoopCount
         {OPND_RAW_DST, ISA_TYPE_W, GRF_ALIGNED},              // dst
     }},
    {VA_OP_CODE_LBP_CREATION,
     ISA_Inst_Sampler,
     "va_lbpcreation",
     5,
     {
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                         // mode
         {OPND_RAW_DST, ISA_TYPE_UB, GRF_ALIGNED},             // dst
     }},
    {VA_OP_CODE_LBP_CORRELATION,
     ISA_Inst_Sampler,
     "va_lbpcorrelation",
     5,
     {
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_W, SCALAR_REGION}, // disparity
         {OPND_RAW_DST, ISA_TYPE_W, GRF_ALIGNED},              // dst
     }},
    {}, // none
    {VA_OP_CODE_CORRELATION_SEARCH,
     ISA_Inst_Sampler,
     "va_correlationsearch",
     10,
     {
         {OPND_SURFACE, ISA_TYPE_UB, 0},                       /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // vOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION}, // verticalOrigin
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F,
          SCALAR_REGION}, // horizontalOrigin
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB,
          SCALAR_REGION}, // xDirectionSize
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB,
          SCALAR_REGION}, // yDirectionSize
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB,
          SCALAR_REGION}, // xDirectionSearchSize
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UB,
          SCALAR_REGION},                          // yDirectionSearchSize
         {OPND_RAW_DST, ISA_TYPE_UD, GRF_ALIGNED}, // dst
     }},
    {ISA_HDC_CONV,
     ISA_Inst_Sampler,
     "va_hdc_convolve_2d",
     8,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                         /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                          /// Properties
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// dstSurface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstXOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstYOffset
     }},
    {ISA_HDC_MMF,
     ISA_Inst_Sampler,
     "va_hdc_minmaxfilter",
     9,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                         /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                          /// pixelSize
         {OPND_OTHER, ISA_TYPE_UB, 0},                          /// mmfMode
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// dstSurface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstXOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstYOffset
     }},
    {ISA_HDC_ERODE,
     ISA_Inst_Sampler,
     "va_hdc_erode",
     7,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                         /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // vOffset
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// dstSurface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstXOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstYOffset
     }},
    {ISA_HDC_DILATE,
     ISA_Inst_Sampler,
     "va_hdc_dilate",
     7,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                         /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // vOffset
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// dstSurface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstXOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstYOffset
     }},
    {ISA_HDC_LBPCORRELATION,
     ISA_Inst_Sampler,
     "va_hdc_lbpcorrelation",
     7,
     {
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // vOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_W, SCALAR_REGION},  // disparity
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// dstSurface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstXOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstYOffset
     }},
    {ISA_HDC_LBPCREATION,
     ISA_Inst_Sampler,
     "va_hdc_lbpcreation",
     7,
     {
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  // vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                          /// mode
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// dstSurface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstXOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, // dstYOffset
     }},
    {ISA_HDC_1DCONV_H,
     ISA_Inst_Sampler,
     "va_hdc_convolve_1d_h",
     8,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                         /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  //  uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  //  vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                          /// pixelSize
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// dstSurface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, //  dstXOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, //  dstYOffset
     }},
    {ISA_HDC_1DCONV_V,
     ISA_Inst_Sampler,
     "va_hdc_convolve_1d_v",
     8,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                         /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  //  uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  //  vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                          /// pixelSize
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// dstSurface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, //  dstXOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, //  dstYOffset
     }},
    {ISA_HDC_1PIXELCONV,
     ISA_Inst_Sampler,
     "va_hdc_convolve_1pixel",
     9,
     {
         {OPND_SAMPLE, ISA_TYPE_UB, 0},                         /// Sampler
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// Surface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  //  uOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_F, SCALAR_REGION},  //  vOffset
         {OPND_OTHER, ISA_TYPE_UB, 0},                          /// pixelSize
         {OPND_RAW_SRC, ISA_TYPE_W, GRF_ALIGNED},               //  offsets
         {OPND_SURFACE, ISA_TYPE_UB, 0},                        /// dstSurface
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, //  dstXOffset
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UW, SCALAR_REGION}, //  dstYOffset
     }},
};

static const ISA_SubInst_Desc SVMSubOpcodeDesc[] = {
    {}, /// 0th entry
    {SVM_BLOCK_LD,
     ISA_Inst_SVM,
     "svm_block_ld",
     3,
     {
         {OPND_OTHER, ISA_TYPE_UB, 0},                /// Properties
         {OPND_VECTOR_SRC, ISA_TYPE_UQ, GRF_ALIGNED}, /// Address
         {OPND_RAW, ISA_TYPE_UQ, 0}                   /// src
     }},

    {SVM_BLOCK_ST,
     ISA_Inst_SVM,
     "svm_block_st",
     3,
     {
         {OPND_OTHER, ISA_TYPE_UB, 0},                /// Properties
         {OPND_VECTOR_SRC, ISA_TYPE_UQ, GRF_ALIGNED}, /// Address
         {OPND_RAW, ISA_TYPE_UQ, 0}                   /// src
     }},

    {SVM_GATHER,
     ISA_Inst_SVM,
     "svm_gather",
     6,
     {
         {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// exec_size
         {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
         {OPND_OTHER, ISA_TYPE_UB, 0},    /// Block size
         {OPND_OTHER, ISA_TYPE_UB, 0},    /// Num blocks
         {OPND_RAW, ISA_TYPE_UQ, 0},      /// Addresses
         {OPND_RAW, ISA_TYPE_UQ, 0}       /// dst
     }},

    {SVM_SCATTER,
     ISA_Inst_SVM,
     "svm_scatter",
     6,
     {
         {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// exec_size
         {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
         {OPND_OTHER, ISA_TYPE_UB, 0},    /// Block size
         {OPND_OTHER, ISA_TYPE_UB, 0},    /// Num blocks
         {OPND_RAW, ISA_TYPE_UQ, 0},      /// Addresses
         {OPND_RAW, ISA_TYPE_UQ, 0}       /// dst
     }},

    {SVM_ATOMIC,
     ISA_Inst_SVM,
     "svm_atomic",
     7,
     {
         {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// exec_size
         {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
         {OPND_OTHER, ISA_TYPE_UB, 0},    /// Op
         {OPND_RAW, ISA_TYPE_UQ, 0},      /// Addresses
         {OPND_RAW, ISA_TYPE_UQ, 0},      /// src0
         {OPND_RAW, ISA_TYPE_UQ, 0},      /// src1
         {OPND_RAW, ISA_TYPE_UQ, 0}       /// dst
     }},

    {SVM_GATHER4SCALED,
     ISA_Inst_SVM,
     "svm_gather4scaled",
     7,
     {
         {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// exec_size
         {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
         {OPND_OTHER, ISA_TYPE_UB, 0},    /// channel mask
         {OPND_OTHER, ISA_TYPE_UW, 0},    /// scale
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UQ,
          SCALAR_REGION},                      /// global offset
         {OPND_RAW, ISA_TYPE_UQ, GRF_ALIGNED}, /// offsets
         {OPND_RAW, ISA_TYPE_F | ISA_TYPE_D | ISA_TYPE_UD, GRF_ALIGNED} /// dst
     }},

    {SVM_SCATTER4SCALED,
     ISA_Inst_SVM,
     "svm_scatter4scaled",
     7,
     {
         {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /// exec_size
         {OPND_PRED, ISA_TYPE_UW, 0},     /// predicate
         {OPND_OTHER, ISA_TYPE_UB, 0},    /// channel mask
         {OPND_OTHER, ISA_TYPE_UW, 0},    /// scale
         {OPND_VECTOR_SRC_G_I_IMM, ISA_TYPE_UQ,
          SCALAR_REGION},                      /// global offset
         {OPND_RAW, ISA_TYPE_UQ, GRF_ALIGNED}, /// offsets
         {OPND_RAW, ISA_TYPE_F | ISA_TYPE_D | ISA_TYPE_UD, GRF_ALIGNED} /// src
     }}};

///////////////////////////////////////////////////////////////////////////
// LSC ops
// clang-format off
#define LSC_UNTYPED_OP(ISA_OP, MNEMONIC)                                       \
  {                                                                            \
    (ISA_OP), ISA_Inst_LSC, (MNEMONIC), 20, {                                  \
      /* execution control */                                                  \
      {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /* execution size */                    \
          {OPND_PRED, ISA_TYPE_UW, 0}, /* predicate */                         \
          /* getPrimitiveOperand(0) */                                         \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_SFID */ /* caching opts */      \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l1 */               \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l3 */               \
          /* addr operands */                                                  \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_ADDR::type */                   \
          {OPND_OTHER, ISA_TYPE_UW, 0}, /* LSC_ADDR::immScale */               \
          {OPND_OTHER, ISA_TYPE_D, 0},  /* LSC_ADDR::immOffset */              \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_ADDR::size */                   \
          /* data shape */                                                     \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::size */             \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::order */            \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::elems */            \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::cmask */            \
          /* operands */                                                       \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* surface base */ \
          {OPND_OTHER, ISA_TYPE_UD, 0},         /* surface index */            \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* DstData */                  \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* Src0Addr */                 \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* Src1Data */                 \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* Src2Data */                 \
    }                                                                          \
  }

#define LSC_UNTYPED_STRIDED_OP(ISA_OP, MNEMONIC)                               \
  {                                                                            \
    (ISA_OP), ISA_Inst_LSC, (MNEMONIC), 19, {                                  \
      /* execution control */                                                  \
      {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /* execution size */                    \
          {OPND_PRED, ISA_TYPE_UW, 0}, /* predicate */                         \
          /* getPrimitiveOperand(0) */                                         \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_SFID */                         \
          /* caching opts */                                                   \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l1 */               \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l3 */               \
          /* addr stuff */                                                     \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_ADDR::type */                   \
          {OPND_OTHER, ISA_TYPE_UW, 0}, /* LSC_ADDR::immScale */               \
          {OPND_OTHER, ISA_TYPE_D, 0},  /* LSC_ADDR::immOffset */              \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_ADDR::size */                   \
          /* data shape stuff */                                               \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::size */             \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::order */            \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::elems */            \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::cmask */            \
          /* operands */                                                       \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* surface base */ \
          {OPND_OTHER, ISA_TYPE_UD, 0},         /* surface index */            \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* DstData */                  \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* Src0AddrBase */             \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* Src0AddrPitch */ \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* Src1Data */                 \
    }                                                                          \
  }
#define LSC_UNTYPED_BLOCK2D_OP(ISA_OP, MNEMONIC)                               \
  {                                                                            \
    (ISA_OP), ISA_Inst_LSC, (MNEMONIC), 20, {                                  \
      /* execution control */                                                  \
      {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /* execution size */                    \
          {OPND_PRED, ISA_TYPE_UW, 0}, /* predicate */                         \
          /* getPrimitiveOperand(0) */                                         \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_SFID */                         \
          /* caching opts */                                                   \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l1 */               \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l3 */               \
          /* block 2d data shape stuff */                                      \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE_BLOCK2D::size */     \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE_BLOCK2D::order */    \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE_BLOCK2D::blocks */   \
          {OPND_OTHER, ISA_TYPE_UW, 0}, /* LSC_DATA_SHAPE_BLOCK2D::width */    \
          {OPND_OTHER, ISA_TYPE_UW, 0}, /* LSC_DATA_SHAPE_BLOCK2D::height */   \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE_BLOCK2D::vnni */     \
          /* operands */                                                       \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED},   /* DstData */                \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* SurfBase */ \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* SurfWidth */ \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* SurfHeight */ \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* SurfPitch */ \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* SurfOffX */ \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* SurfOffY */ \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* Src1Data */                 \
    }                                                                          \
  }

// All LSC typed ops encode the same
#define LSC_TYPED_OP(ISA_OP, MNEMONIC)                                         \
  {                                                                            \
    (ISA_OP), ISA_Inst_LSC, (MNEMONIC), 23, {                                  \
      /* execution control */                                                  \
      {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /* execution size */                    \
          {OPND_PRED, ISA_TYPE_UW, 0}, /* predicate */                         \
          /* caching opts */                                                   \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l1 */               \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l3 */               \
          /* addr stuff */                                                     \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_ADDR::type */                   \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_ADDR::size */                   \
           /* data shape stuff */                                              \
           /* we keep LSC_DATA_SHAPE::order due to lsc_load_status.tgm */      \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE:size */              \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::order */            \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::elems */            \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::cmask */            \
          /* operands */                                                       \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* surface base */ \
          {OPND_OTHER, ISA_TYPE_UD, 0},         /* surface index */            \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* dst */                      \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* src0 addr Us */             \
          {OPND_OTHER, ISA_TYPE_D, 0},          /* src0 addr Us immoff */      \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* src0 addr Vs */             \
          {OPND_OTHER, ISA_TYPE_D, 0},          /* src0 addr Vs immoff */      \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* src0 addr Rs */             \
          {OPND_OTHER, ISA_TYPE_D, 0},          /* src0 addr Rs immoff */      \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* src0 addr LODs */           \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* src1 data */                \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* src2 data */                \
    }                                                                          \
  }
#define LSC_TYPED_BLOCK2D_OP(ISA_OP, MNEMONIC)                                 \
  {                                                                            \
    (ISA_OP), ISA_Inst_LSC, (MNEMONIC), 16, {                                  \
      /* execution control */                                                  \
      {OPND_EXECSIZE, ISA_TYPE_UB, 0},                 /* execution size */    \
          {OPND_PRED, ISA_TYPE_UW, 0}, /* predicate */                         \
          /* caching opts */                                                   \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l1 */               \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l3 */               \
          /* addr type */                                                      \
          {OPND_OTHER, ISA_TYPE_UB,0}, /* LSC_ADDR::type */                    \
          /* block 2d data shape stuff */                                      \
          {OPND_OTHER, ISA_TYPE_UW, 0}, /* LSC_DATA_SHAPE_TYPED_BLOCK2D::width */ \
          {OPND_OTHER, ISA_TYPE_UW, 0}, /* LSC_DATA_SHAPE_TYPED_BLOCK2D::height */ \
           /* operands */                                                      \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* surface base */ \
          {OPND_OTHER, ISA_TYPE_UD, 0},         /* surface index */            \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* DstData */                  \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* SurfOffX */ \
          {OPND_OTHER, ISA_TYPE_D, 0},          /* ImmOffX */                  \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* SurfOffY */ \
          {OPND_OTHER, ISA_TYPE_D, 0},          /* ImmOffY */                  \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* Src1Data */                 \
    }                                                                          \
  }
#define LSC_UNTYPED_APNDCTR_ATOMIC_OP(ISA_OP, MNEMONIC)                        \
  {                                                                            \
    (ISA_OP), ISA_Inst_LSC, (MNEMONIC), 15, {                                  \
      /* execution control */                                                  \
      {OPND_EXECSIZE, ISA_TYPE_UB, 0}, /* execution size */                    \
          {OPND_PRED, ISA_TYPE_UW, 0}, /* predicate */                         \
          /* getPrimitiveOperand(0) */                                         \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_SFID */                         \
          /* caching opts */                                                   \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l1 */               \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_CACHE_OPTS::l3 */               \
          /* addr type */                                                      \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_ADDR::type */                   \
          /* data shape stuff */                                               \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::size */             \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::order */            \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::elems */            \
          {OPND_OTHER, ISA_TYPE_UB, 0}, /* LSC_DATA_SHAPE::cmask */            \
          /* operands */                                                       \
          {OPND_SRC_GEN | OPND_IMM | OPND_SRC_ADDR, ISA_TYPE_UB, 0}, /* surface base */ \
          {OPND_OTHER, ISA_TYPE_UD, 0},         /* surface index */            \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* DstData */                  \
          {OPND_RAW, ISA_TYPE_UB, GRF_ALIGNED}, /* Src0Data */                 \
    }                                                                          \
  }
#define LSC_OP_INVALID                                                         \
  {}

// clang-format on

static const ISA_SubInst_Desc LscUntypedSubOpcodeDescs[] {
  LSC_UNTYPED_OP(LSC_LOAD, "lsc_load"),
      LSC_UNTYPED_OP(LSC_LOAD_STRIDED, "lsc_load_strided"),
      LSC_UNTYPED_OP(LSC_LOAD_QUAD, "lsc_load_quad"),
      LSC_UNTYPED_BLOCK2D_OP(LSC_LOAD_BLOCK2D, "lsc_load_block2d"),
      LSC_UNTYPED_OP(LSC_STORE, "lsc_store"),
      LSC_UNTYPED_OP(LSC_STORE_STRIDED, "lsc_store_strided"),
      LSC_UNTYPED_OP(LSC_STORE_QUAD, "lsc_store_quad"),
      LSC_UNTYPED_BLOCK2D_OP(LSC_STORE_BLOCK2D, "lsc_store_block2d"),
      LSC_UNTYPED_OP(LSC_ATOMIC_IINC, "lsc_atomic_iinc"),
      LSC_UNTYPED_OP(LSC_ATOMIC_IDEC, "lsc_atomic_idec"),
      LSC_UNTYPED_OP(LSC_ATOMIC_LOAD, "lsc_atomic_load"),
      LSC_UNTYPED_OP(LSC_ATOMIC_STORE, "lsc_atomic_store"),
      LSC_UNTYPED_OP(LSC_ATOMIC_IADD, "lsc_atomic_iadd"),
      LSC_UNTYPED_OP(LSC_ATOMIC_ISUB, "lsc_atomic_isub"),
      LSC_UNTYPED_OP(LSC_ATOMIC_SMIN, "lsc_atomic_smin"),
      LSC_UNTYPED_OP(LSC_ATOMIC_SMAX, "lsc_atomic_smax"),
      LSC_UNTYPED_OP(LSC_ATOMIC_UMIN, "lsc_atomic_umin"),
      LSC_UNTYPED_OP(LSC_ATOMIC_UMAX, "lsc_atomic_umax"),
      LSC_UNTYPED_OP(LSC_ATOMIC_ICAS, "lsc_atomic_icas"),
      LSC_UNTYPED_OP(LSC_ATOMIC_FADD, "lsc_atomic_fadd"),
      LSC_UNTYPED_OP(LSC_ATOMIC_FSUB, "lsc_atomic_fsub"),
      LSC_UNTYPED_OP(LSC_ATOMIC_FMIN, "lsc_atomic_fmin"),
      LSC_UNTYPED_OP(LSC_ATOMIC_FMAX, "lsc_atomic_fmax"),
      LSC_UNTYPED_OP(LSC_ATOMIC_FCAS, "lsc_atomic_fcas"),
      LSC_UNTYPED_OP(LSC_ATOMIC_AND, "lsc_atomic_and"),
      LSC_UNTYPED_OP(LSC_ATOMIC_OR, "lsc_atomic_or"),
      LSC_UNTYPED_OP(LSC_ATOMIC_XOR, "lsc_atomic_xor"),
      //
      LSC_UNTYPED_OP(LSC_LOAD_STATUS, "lsc_load_status"),
      LSC_UNTYPED_OP(LSC_STORE_UNCOMPRESSED, "lsc_store_uncompressed"),
      LSC_UNTYPED_OP(LSC_CCS_UPDATE, "lsc_ccs_update"),
      LSC_OP_INVALID, // lsc_read_state_info only for typed
      LSC_OP_INVALID, // fence handled separately
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // reserved
      LSC_UNTYPED_OP(LSC_APNDCTR_ATOMIC_ADD, "lsc_apndctr_atomic_add"),
      LSC_UNTYPED_OP(LSC_APNDCTR_ATOMIC_SUB, "lsc_apndctr_atomic_sub"),
      LSC_UNTYPED_OP(LSC_APNDCTR_ATOMIC_STORE, "lsc_apndctr_atomic_store"),
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      // msrt instructions need to be recorded under untyped as the parser logic
      // by default queries the untyped table to get the opcode string
      // todo: need to change this as this is uncessary for msrt instructions
      // which are only applicable for typed surfaces
      LSC_TYPED_OP(LSC_LOAD_QUAD_MSRT, "lsc_load_quad_msrt"),
      LSC_TYPED_OP(LSC_STORE_QUAD_MSRT, "lsc_store_quad_msrt"),
      LSC_OP_INVALID,
};

static const ISA_SubInst_Desc LscTypedSubOpcodeDescs[] {
  LSC_OP_INVALID,     // LSC_LOAD only untyped
      LSC_OP_INVALID, // LSC_LOAD_STRIDED only untyped
      LSC_TYPED_OP(LSC_LOAD_QUAD, "lsc_load_quad"),
      LSC_TYPED_BLOCK2D_OP(LSC_LOAD_BLOCK2D, "lsc_load_block2d"),
      LSC_OP_INVALID, // LSC_STORE only untyped
      LSC_OP_INVALID, // LSC_STORE_STRIDED only untyped
      LSC_TYPED_OP(LSC_STORE_QUAD, "lsc_store_quad"),
      LSC_TYPED_BLOCK2D_OP(LSC_STORE_BLOCK2D, "lsc_load_block2d"),
      LSC_TYPED_OP(LSC_ATOMIC_IINC, "lsc_atomic_iinc"),
      LSC_TYPED_OP(LSC_ATOMIC_IDEC, "lsc_atomic_idec"),
      LSC_TYPED_OP(LSC_ATOMIC_LOAD, "lsc_atomic_load"),
      LSC_TYPED_OP(LSC_ATOMIC_STORE, "lsc_atomic_store"),
      LSC_TYPED_OP(LSC_ATOMIC_IADD, "lsc_atomic_iadd"),
      LSC_TYPED_OP(LSC_ATOMIC_ISUB, "lsc_atomic_isub"),
      LSC_TYPED_OP(LSC_ATOMIC_SMIN, "lsc_atomic_smin"),
      LSC_TYPED_OP(LSC_ATOMIC_SMAX, "lsc_atomic_smax"),
      LSC_TYPED_OP(LSC_ATOMIC_UMIN, "lsc_atomic_umin"),
      LSC_TYPED_OP(LSC_ATOMIC_UMAX, "lsc_atomic_umax"),
      LSC_TYPED_OP(LSC_ATOMIC_ICAS, "lsc_atomic_icas"),
      LSC_TYPED_OP(LSC_ATOMIC_FADD, "lsc_atomic_fadd"),
      LSC_TYPED_OP(LSC_ATOMIC_FSUB, "lsc_atomic_fsub"),
      LSC_TYPED_OP(LSC_ATOMIC_FMIN, "lsc_atomic_fmin"),
      LSC_TYPED_OP(LSC_ATOMIC_FMAX, "lsc_atomic_fmax"),
      LSC_TYPED_OP(LSC_ATOMIC_FCAS, "lsc_atomic_fcas"),
      LSC_TYPED_OP(LSC_ATOMIC_AND, "lsc_atomic_and"),
      LSC_TYPED_OP(LSC_ATOMIC_OR, "lsc_atomic_or"),
      LSC_TYPED_OP(LSC_ATOMIC_XOR, "lsc_atomic_xor"),
      //
      LSC_TYPED_OP(LSC_LOAD_STATUS, "lsc_load_status"),
      LSC_TYPED_OP(LSC_STORE_UNCOMPRESSED, "lsc_store_uncompressed"),
      LSC_TYPED_OP(LSC_CCS_UPDATE, "lsc_ccs_update"),
      LSC_TYPED_OP(LSC_READ_STATE_INFO, "lsc_read_state_info"),
      LSC_OP_INVALID, // fence handled separately
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // reserved
      LSC_OP_INVALID, // LSC_APNDCTR_ATOMIC_ADD only untyped
      LSC_OP_INVALID, // LSC_APNDCTR_ATOMIC_SUB only untyped
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_OP_INVALID,
      LSC_TYPED_OP(LSC_LOAD_QUAD_MSRT, "lsc_load_quad_msrt"),
      LSC_TYPED_OP(LSC_STORE_QUAD_MSRT, "lsc_store_quad_msrt"),
};

LscOpInfo LscOpInfoGet(LSC_OP op) {
  LscOpInfo opInfo{};
  if (!LscOpInfoFind(op, opInfo)) {
    vISA_ASSERT(false, "invalid LSC opcode");
  }
  return opInfo;
}

bool LscOpInfoFind(LSC_OP op, LscOpInfo &opInfo) {
  opInfo.kind = LscOpInfo::OTHER;
  opInfo.encoding = 0xFFFFFFFF;
  opInfo.mnemonic = nullptr;
  opInfo.op = op;

  auto loadOp = [&](const char *mne, uint32_t bits) {
    opInfo.kind = LscOpInfo::LOAD;
    opInfo.encoding = bits;
    opInfo.mnemonic = mne;
  };
  auto storeOp = [&](const char *mne, uint32_t bits) {
    opInfo.mnemonic = mne;
    opInfo.kind = LscOpInfo::STORE;
    opInfo.encoding = bits;
  };
  auto atomicOp = [&](const char *mne, uint32_t bits, int extraOps) {
    opInfo.kind = LscOpInfo::ATOMIC;
    opInfo.encoding = bits;
    opInfo.mnemonic = mne;
    opInfo.extraOperands = extraOps;
  };
  auto otherOp = [&](const char *mne, uint32_t bits) {
    opInfo.kind = LscOpInfo::OTHER;
    opInfo.encoding = bits;
    opInfo.mnemonic = mne;
  };
  switch (op) {
  case LSC_LOAD:
    loadOp("lsc_load", 0x00);
    break;
  case LSC_LOAD_STRIDED:
    loadOp("lsc_load_strided", 0x01);
    break;
  case LSC_LOAD_QUAD:
    loadOp("lsc_load_quad", 0x02);
    break;
  case LSC_LOAD_BLOCK2D:
    loadOp("lsc_load_block2d", 0x03);
    break;
  //
  case LSC_STORE:
    storeOp("lsc_store", 0x04);
    break;
  case LSC_STORE_STRIDED:
    storeOp("lsc_store_strided", 0x05);
    break;
  case LSC_STORE_QUAD:
    storeOp("lsc_store_quad", 0x06);
    break;
  case LSC_STORE_BLOCK2D:
    storeOp("lsc_store_block2d", 0x07);
    break;
  //
  case LSC_ATOMIC_IINC:
    atomicOp("lsc_atomic_iinc", 0x08, 0);
    break;
  case LSC_ATOMIC_IDEC:
    atomicOp("lsc_atomic_idec", 0x09, 0);
    break;
  case LSC_ATOMIC_LOAD:
    atomicOp("lsc_atomic_load", 0x0A, 0);
    break;
  case LSC_ATOMIC_STORE:
    atomicOp("lsc_atomic_store", 0x0B, 1);
    break;
  case LSC_ATOMIC_IADD:
    atomicOp("lsc_atomic_iadd", 0x0C, 1);
    break;
  case LSC_ATOMIC_ISUB:
    atomicOp("lsc_atomic_isub", 0x0D, 1);
    break;
  case LSC_ATOMIC_SMIN:
    atomicOp("lsc_atomic_smin", 0x0E, 1);
    break;
  case LSC_ATOMIC_SMAX:
    atomicOp("lsc_atomic_smax", 0x0F, 1);
    break;
  case LSC_ATOMIC_UMIN:
    atomicOp("lsc_atomic_umin", 0x10, 1);
    break;
  case LSC_ATOMIC_UMAX:
    atomicOp("lsc_atomic_umax", 0x11, 1);
    break;
  case LSC_ATOMIC_ICAS:
    atomicOp("lsc_atomic_icas", 0x12, 2);
    break;
  case LSC_ATOMIC_FADD:
    atomicOp("lsc_atomic_fadd", 0x13, 1);
    break;
  case LSC_ATOMIC_FSUB:
    atomicOp("lsc_atomic_fsub", 0x14, 1);
    break;
  case LSC_ATOMIC_FMIN:
    atomicOp("lsc_atomic_fmin", 0x15, 1);
    break;
  case LSC_ATOMIC_FMAX:
    atomicOp("lsc_atomic_fmax", 0x16, 1);
    break;
  case LSC_ATOMIC_FCAS:
    atomicOp("lsc_atomic_fcas", 0x17, 2);
    break;
  case LSC_ATOMIC_AND:
    atomicOp("lsc_atomic_and", 0x18, 1);
    break;
  case LSC_ATOMIC_OR:
    atomicOp("lsc_atomic_or", 0x19, 1);
    break;
  case LSC_ATOMIC_XOR:
    atomicOp("lsc_atomic_xor", 0x1A, 1);
    break;
  //
  case LSC_LOAD_STATUS:
    loadOp("lsc_load_status", 0x1B);
    break;
  case LSC_STORE_UNCOMPRESSED:
    storeOp("lsc_store_uncompressed", 0x1C);
    break;
  case LSC_CCS_UPDATE:
    otherOp("lsc_ccs_update", 0x1D);
    break;
  case LSC_READ_STATE_INFO:
    loadOp("lsc_read_state_info", 0x1E);
    break;
  case LSC_FENCE:
    otherOp("lsc_fence", 0x1F);
    break;
    //
  case LSC_APNDCTR_ATOMIC_ADD:
    atomicOp("lsc_apndctr_atomic_add", 0x28, 1);
    break;
  case LSC_APNDCTR_ATOMIC_SUB:
    atomicOp("lsc_apndctr_atomic_sub", 0x29, 1);
    break;
  case LSC_APNDCTR_ATOMIC_STORE:
    atomicOp("lsc_apndctr_atomic_store", 0x30, 1);
    break;

 case LSC_LOAD_QUAD_MSRT:
    loadOp("lsc_load_quad_msrt", 0x31);
    break;
 case LSC_STORE_QUAD_MSRT:
    storeOp("lsc_store_quad_msrt", 0x32);
    break;
  default:
    return false;
  }
  return true;
}

// retuen the encoding value for descriptor bits[19:17]
bool LscTryEncodeCacheOptsBits17_19(const LscOpInfo &opInfo,
                                    LSC_CACHE_OPTS cacheOpts,
                                    uint32_t &cacheEnc) {
  auto matches = [&](LSC_CACHE_OPT l1, LSC_CACHE_OPT l3) {
    return (cacheOpts.l1 == l1 && cacheOpts.l3 == l3);
  };

  if (matches(LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT)) {
    // same for load/atomic/store
    cacheEnc = 0x0;
  } else if (matches(LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED)) {
    // same for load/atomic/store
    cacheEnc = 0x1;
  } else if (opInfo.isLoad()) {
    if (matches(LSC_CACHING_UNCACHED, LSC_CACHING_CACHED)) {
      cacheEnc = 0x2;
    } else if (matches(LSC_CACHING_CACHED, LSC_CACHING_UNCACHED)) {
      cacheEnc = 0x3;
    } else if (matches(LSC_CACHING_CACHED, LSC_CACHING_CACHED)) {
      cacheEnc = 0x4;
    } else if (matches(LSC_CACHING_STREAMING, LSC_CACHING_UNCACHED)) {
      cacheEnc = 0x5;
    } else if (matches(LSC_CACHING_STREAMING, LSC_CACHING_CACHED)) {
      cacheEnc = 0x6;
    } else if (matches(LSC_CACHING_READINVALIDATE, LSC_CACHING_CACHED)) {
      cacheEnc = 0x7;
    } else {
      return false;
    }
  } else if (opInfo.isAtomic()) {
    if (matches(LSC_CACHING_UNCACHED, LSC_CACHING_WRITEBACK)) {
      cacheEnc = 0x2;
    } else {
      return false;
    }
  } else {
    if (matches(LSC_CACHING_UNCACHED, LSC_CACHING_WRITEBACK)) {
      cacheEnc = 0x2;
    } else if (matches(LSC_CACHING_WRITETHROUGH, LSC_CACHING_UNCACHED)) {
      cacheEnc = 0x3;
    } else if (matches(LSC_CACHING_WRITETHROUGH, LSC_CACHING_WRITEBACK)) {
      cacheEnc = 0x4;
    } else if (matches(LSC_CACHING_STREAMING, LSC_CACHING_UNCACHED)) {
      cacheEnc = 0x5;
    } else if (matches(LSC_CACHING_STREAMING, LSC_CACHING_WRITEBACK)) {
      cacheEnc = 0x6;
    } else if (matches(LSC_CACHING_WRITEBACK, LSC_CACHING_WRITEBACK)) {
      cacheEnc = 0x7;
    } else {
      return false;
    }
  }

  cacheEnc = cacheEnc << 17;
  return true;
}
// retuen the encoding value for descriptor bits[19:16]
bool LscTryEncodeCacheOptsBits16_19(const LscOpInfo &opInfo,
                                    LSC_CACHE_OPTS cacheOpts,
                                    uint32_t &cacheEnc) {
  // Try match bits[19:17] first
  if (LscTryEncodeCacheOptsBits17_19(opInfo, cacheOpts, cacheEnc))
    return true;

  // otherwise, try to match 3 cases for bits[19:16] mode only
  // those cases must be load
  if (!opInfo.isLoad())
    return false;

  auto matches = [&](LSC_CACHE_OPT l1, LSC_CACHE_OPT l3) {
    return (cacheOpts.l1 == l1 && cacheOpts.l3 == l3);
  };

  if (matches(LSC_CACHING_READINVALIDATE, LSC_CACHING_READINVALIDATE)) {
    cacheEnc = 0xE;
  } else if (matches(LSC_CACHING_UNCACHED, LSC_CACHING_CONSTCACHED)) {
    cacheEnc = 0x5;
  } else if (matches(LSC_CACHING_CACHED, LSC_CACHING_CONSTCACHED)) {
    cacheEnc = 0x9;
  } else {
    return false;
  }
  cacheEnc = cacheEnc << 16;
  return true;
}

bool LscTryEncodeCacheOpts(const LscOpInfo &opInfo, LSC_CACHE_OPTS cacheOpts,
                           uint32_t &cacheEn, bool isBits17_19) {
  if (!isBits17_19)
    return LscTryEncodeCacheOptsBits16_19(opInfo, cacheOpts, cacheEn);
  return LscTryEncodeCacheOptsBits17_19(opInfo, cacheOpts, cacheEn);
}

const ISA_SubInst_Desc *getSubInstTable(uint8_t opcode, int &size) {
  const ISA_SubInst_Desc *table = nullptr;
  switch (opcode) {
  case ISA_VA:
    table = VASubOpcodeDesc;
    size = sizeof(VASubOpcodeDesc) / sizeof(VASubOpcodeDesc[0]);
    break;
  case ISA_VA_SKL_PLUS:
    table = VAPlusSubOpcodeDesc;
    size = sizeof(VAPlusSubOpcodeDesc) / sizeof(VAPlusSubOpcodeDesc[0]);
    break;
  case ISA_SVM:
    table = SVMSubOpcodeDesc;
    size = sizeof(SVMSubOpcodeDesc) / sizeof(SVMSubOpcodeDesc[0]);
    break;
  case ISA_LSC_UNTYPED:
    table = LscUntypedSubOpcodeDescs;
    size =
        sizeof(LscUntypedSubOpcodeDescs) / sizeof(LscUntypedSubOpcodeDescs[0]);
    break;
  case ISA_LSC_TYPED:
    table = LscTypedSubOpcodeDescs;
    size = sizeof(LscTypedSubOpcodeDescs) / sizeof(LscTypedSubOpcodeDescs[0]);
    break;
  default:
    table = nullptr;
    size = 0;
    break;
  }
  return table;
}

const ISA_SubInst_Desc &
VISA_INST_Desc::getSubInstDesc(uint8_t subOpcode) const {
  int len;
  const ISA_SubInst_Desc *table = getSubInstTable(opcode, len);
  vISA_ASSERT((int)subOpcode < len, "subop out of bounds");
  return table[subOpcode];
}

const ISA_SubInst_Desc &
VISA_INST_Desc::getSubInstDescByName(const char *symbol) const {
  int len = 0;
  const ISA_SubInst_Desc *table = getSubInstTable(opcode, len);
  for (int i = 0; i < len; i++) {
    if (table[i].name && strcmp(table[i].name, symbol) == 0) {
      return table[i];
    }
  }
  vISA_ASSERT(false, "invalid subop");
  return table[0];
}
