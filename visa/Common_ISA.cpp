/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "visa_igc_common_header.h"

#include "Common_ISA.h"
#include "Mem_Manager.h"
#include "VISADefines.h"

#include "IGC/common/StringMacros.hpp"

#define ALLOC_ASSERT(X)      \
    if (X == NULL) return 1;

const char *implictKindStrings[IMPLICIT_INPUT_COUNT] = {
    "EXPLICIT", "LOCAL_SIZE", "GROUP_COUNT", "LOCAL_ID", "PSEUDO_INPUT"};

const char* Rel_op_str[ISA_CMP_UNDEF + 1] =
{
    "eq",  // equal
    "ne", // not equal
    "gt",  // greater
    "ge", // greater or equal
    "lt",  // less
    "le", // less or equal
    " "
};

const char* media_ld_mod_str[MEDIA_LD_Mod_NUM] =
{
    "nomod",
    "modified",
    "top",
    "bottom",
    "top_mod",
    "bottom_mod"
};

const char* media_st_mod_str[MEDIA_ST_Mod_NUM] =
{
    "nomod",
    "reserved", // this is useless since it is for MEDIA_ST_reserved
    "top",
    "bottom"
};

const char* channel_mask_str[CHANNEL_MASK_NUM] =
{
    "",    // 0000
    "R",   // 0001
    "G",   // 0010
    "RG",  // 0011
    "B",   // 0100
    "RB",  // 0101
    "GB",  // 0110
    "RGB", // 0111
    "A",   // 1000
    "RA",  // 1001
    "GA",  // 1010
    "RGA", // 1011
    "BA",  // 1100
    "RBA", // 1101
    "GBA", // 1110
    "RGBA" // 1111
};

const char* channel_mask_slm_str[CHANNEL_MASK_NUM] =
{
    "RGBA",
    "GBA",
    "RBA",
    "BA",
    "RGA",
    "GA",
    "RA",
    "A",
    "RGB",
    "GB",
    "RB",
    "B",
    "RG",
    "G",
    "R",
    "0000"
};

const char* sampler_channel_output_str[4] =
{
    "16-full",
    "16-downsampled",
    "8-full",
    "8-downsampled"
};

const char* vme_op_mode_str[VME_OP_MODE_NUM] =
{
    "inter",
    "intra",
    "both"
};

const char* emask_str[vISA_NUM_EMASK+1] =
{
    "M1",
    "M2",
    "M3",
    "M4",
    "M5",
    "M6",
    "M7",
    "M8",
    "M1_NM",
    "M2_NM",
    "M3_NM",
    "M4_NM",
    "M5_NM",
    "M6_NM",
    "M7_NM",
    "M8_NM",
    "NoMask"
};


static const char* getSampleOp3DNameOrNull(VISASampler3DSubOpCode opcode, TARGET_PLATFORM platform)
{
    switch (opcode)
    {
    case VISA_3D_SAMPLE:        // 0x0
        return "sample_3d";
    case VISA_3D_SAMPLE_B:      // 0x1
        return "sample_b";
    case VISA_3D_SAMPLE_L:      // 0x2
        return "sample_l";
    case VISA_3D_SAMPLE_C:      // 0x3
        return "sample_c";
    case VISA_3D_SAMPLE_D:      // 0x4
        return "sample_d";
    case VISA_3D_SAMPLE_B_C:    // 0x5
        return "sample_b_c";
    case VISA_3D_SAMPLE_L_C:    // 0x6
        return "sample_l_c";
    case VISA_3D_LD:            // 0x7
        return "load_3d";
    case VISA_3D_GATHER4:       // 0x8
        return "sample4";
    case VISA_3D_LOD:           // 0x9
        return "lod";
    case VISA_3D_RESINFO:       // 0xA
        return "resinfo";
    case VISA_3D_SAMPLEINFO:    // 0xB
        return "sampleinfo";
    case VISA_3D_SAMPLE_KILLPIX:// 0xC
        return "sample+killpix";
    case VISA_3D_GATHER4_C:     // 0x10
        return "sample4_c";
    case VISA_3D_GATHER4_PO:    // 0x11
        return "sample4_po";
    case VISA_3D_GATHER4_PO_C:  // 0x12
        return "sample4_po_c";
    case VISA_3D_SAMPLE_D_C:    // 0x14
        return "sample_d_c";
    case VISA_3D_SAMPLE_LZ:     // 0x18
        return "sample_lz";
    case VISA_3D_SAMPLE_C_LZ:   // 0x19
        return "sample_c_lz";
    case VISA_3D_LD_LZ:         // 0x1A
        return "load_lz";
    case VISA_3D_LD2DMS_W:      // 0x1C
        return "load_2dms_w";
    case VISA_3D_LD_MCS:        // 0x1D
        return "load_mcs";
    default:
        return nullptr;
    }
}
const char* getSampleOp3DName(VISASampler3DSubOpCode opcode, TARGET_PLATFORM platform)
{
    const char *name = getSampleOp3DNameOrNull(opcode, platform);
    assert(name && "invalid sampler opcode");
    if (!name)
        return "sampler_unknown";
    return name;
}
VISASampler3DSubOpCode getSampleOpFromName(const char *str, TARGET_PLATFORM platform)
{
    for (int i = 0; i < ISA_NUM_OPCODE; i++) {
        const char *symI = getSampleOp3DNameOrNull((VISASampler3DSubOpCode)i, platform);
        if (symI && strcmp(symI, str) == 0)
            return (VISASampler3DSubOpCode)i;
    }
    return (VISASampler3DSubOpCode)-1;
}

const char * va_sub_names[26] =
{
    "avs"                , //0x0
    "convolve"           , //0x1
    "minmax"             , //0x2
    "minmaxfilter"       , //0x3
    "erode"              , //0x4
    "dilate"             , //0x5
    "boolcentroid"       , //0x6
    "centroid"           , //0x7
    "CONV_1D_HORIZONTAL" , //0x8
    "CONV_1D_VERTICAL"   , //0x9
    "CONV_1PIXEL"        , //0x10
    "FLOOD_FILL"         , //0x11
    "LBP_CREATION"       , //0x12
    "LBP_CORRELATION"    , //0x13
    ""                   , //0x14
    "CORRELATION_SEARCH" , //0x15
    "HDC_CONVOLVE_2D"    , //0x10
    "HDC_MIN_MAX_FILTER" , //0x11
    "HDC_ERODE"          , //0x12
    "HDC_DILATE"         , //0x13
    "HDC_LBP_CORRELATION", //0x14
    "HDC_LBP_CREATION"   , //0x15
    "HDC_CONVOLVE_1D_H"  , //0x16
    "HDC_CONVOLVE_1D_V"  , //0x17
    "HDC_CONVOLVE_1P"    , //0x18
    "UNDEFINED"            //0x19
};

const char * pixel_size_str[2] =
{
    "VA_Y16_FORMAT",
    "VA_Y8_FORMAT"
};

const char * lbp_creation_mode[3] =
{
    "VA_5x5_mode",
    "VA_3x3_mode",
    "VA_BOTH_mode"
};

const char * avs_control_str[4] =
{   "AVS_16_FULL",
    "AVS_16_DOWN_SAMPLE",
    "AVS_8_FULL",
    "AVS_8_DOWN_SAMPLE"
};

const char * avs_exec_mode[3] =
{   "AVS_16x4",
    "AVS_INVALID",
    "AVS_16x8"
};

const char * mmf_exec_mode[4] =
{   "VA_MMF_16x4",
    "VA_MMF_INVALID",
    "VA_MMF_16x1",
    "VA_MMF_1x1"
};

const char * mmf_enable_mode[3] =
{   "VA_MINMAX_ENABLE",
    "VA_MAX_ENABLE",
    "VA_MIN_ENABLE"
};

const char * ed_exec_mode[4] =
{   "VA_ED_64x4",
    "VA_ED_32x4",
    "VA_ED_64x1",
    "VA_ED_32x1"
};

const char * conv_exec_mode[4] =
{   "VA_CONV_16x4",
    "VA_CONV_INVALID",
    "VA_CONV_16x1",
    "VA_CONV_1x1"
};

unsigned format_control_byteSize2[4] =
{   4, /// AVS_16_FULL
    2, /// AVS_16_DOWN_SAMPLE NOT VALID
    2, /// AVS_8_FULL
    1  /// AVS_8_DOWN_SAMPLE NOT VALID
};

unsigned ed_exec_mode_byte_size[4] =
{   64 * 4 / 8, /// VA_ED_64x4
    32 * 4 / 8, /// VA_ED_32x4
    64 * 1 / 8, /// VA_ED_64x1
    32 * 1 / 8  /// VA_ED_32x1
};

unsigned conv_exec_mode_size[4] =
{   16 * 4, /// 16x4
         1, /// invalid
    16 * 1, /// 16x1
         1  /// 1x1 1pixelconvovle only
};

unsigned mmf_exec_mode_size[4] =
{   16 * 4, /// 16x4
         1, /// invalid
    16 * 1, /// 16x1
    1  * 1  /// 1x1
};

unsigned lbp_creation_exec_mode_size[3] =
{   16 * 8, /// BOTH
    16 * 4, /// 3x3
    16 * 4  /// 5x5
};

unsigned lbp_correlation_mode_size[3] =
{   16 * 4, /// 16x4
         1, /// invalid
    16 * 1  /// 16x1
};

unsigned mmf_exec_mode_bit_size[4] =
{   16, /// 16x4 -- 16 bits
     1, /// invalid
    16, /// 16x1 -- 16 bits
     8  /// 1x1  -- 8  bits
};

unsigned output_format_control_size[4] =
{   16,
    16,
     8,
     8
};

// NOTE: keep the order consistent with CMAtomicOperations
const char* CISAAtomicOpNames[] = {
    "add",      // ATOMIC_ADD       = 0x0,
    "sub",      // ATOMIC_SUB       = 0x1,
    "inc",      // ATOMIC_INC       = 0x2,
    "dec",      // ATOMIC_DEC       = 0x3,
    "min",      // ATOMIC_MIN       = 0x4,
    "max",      // ATOMIC_MAX       = 0x5,
    "xchg",     // ATOMIC_XCHG      = 0x6,
    "cmpxchg",  // ATOMIC_CMPXCHG   = 0x7,
    "and",      // ATOMIC_AND       = 0x8,
    "or",       // ATOMIC_OR        = 0x9,
    "xor",      // ATOMIC_XOR       = 0xa,
    "minsint",  // ATOMIC_IMIN      = 0xb,
    "maxsint",  // ATOMIC_IMAX      = 0xc,
    "",         //              [SKIP 0xd]
    "",         //              [SKIP 0xe]
    "",         //              [SKIP 0xf]
    "fmax",     // ATOMIC_FMAX      = 0x10,
    "fmin",     // ATOMIC_FMIN      = 0x11,
    "fcmpwr",   // ATOMIC_FCMPWR    = 0x12,
    "fadd",     // ATOMIC_FADD      = 0x13,
    "fsub",     // ATOMIC_FSUB      = 0x14
};

CISATypeInfo CISATypeTable[ISA_TYPE_NUM] =
{
    { ISA_TYPE_UD,  "ud",   4 },
    { ISA_TYPE_D,   "d",    4 },
    { ISA_TYPE_UW,  "uw",   2 },
    { ISA_TYPE_W,   "w",    2 },
    { ISA_TYPE_UB,  "ub",   1 },
    { ISA_TYPE_B,   "b",    1 },
    { ISA_TYPE_DF,  "df",   8 },
    { ISA_TYPE_F,   "f",    4 },
    { ISA_TYPE_V,   "v",    4 },
    { ISA_TYPE_VF,  "vf",   4 },
    { ISA_TYPE_BOOL,"bool", 1 },
    { ISA_TYPE_UQ,  "uq",   8 },
    { ISA_TYPE_UV,  "uv",   4 },
    { ISA_TYPE_Q,   "q",    8 },
    { ISA_TYPE_HF,  "hf",   2 },
    { ISA_TYPE_BF,  "bf",   2 }
};

int processCommonISAHeader(
    common_isa_header& cisaHdr,
    unsigned& byte_pos,
    const void* cisaBuffer,
    vISA::Mem_Manager* mem)
{
    const char *buf = (const char *)cisaBuffer;
    READ_FIELD_FROM_BUF(cisaHdr.magic_number, uint32_t);
    READ_FIELD_FROM_BUF(cisaHdr.major_version, uint8_t);
    READ_FIELD_FROM_BUF(cisaHdr.minor_version, uint8_t);
    READ_FIELD_FROM_BUF(cisaHdr.num_kernels, uint16_t);

    MUST_BE_TRUE(cisaHdr.major_version >= 3, "only vISA version 3.0 and above are supported");

    if (cisaHdr.num_kernels) {
        cisaHdr.kernels =
            (kernel_info_t *)mem->alloc(
                    sizeof(kernel_info_t) * cisaHdr.num_kernels);
        ALLOC_ASSERT(cisaHdr.kernels);
    }
    else {
        cisaHdr.kernels = NULL;
    }

    for (int i = 0; i < cisaHdr.num_kernels; i++) {
        if (cisaHdr.major_version == 3 && cisaHdr.minor_version < 7)
        {
            READ_FIELD_FROM_BUF(cisaHdr.kernels[i].name_len, uint8_t);
        }
        else
        {
            READ_FIELD_FROM_BUF(cisaHdr.kernels[i].name_len, uint16_t);
        }
        cisaHdr.kernels[i].name = (char*)mem->alloc(cisaHdr.kernels[i].name_len + 1);
        memcpy_s(
            cisaHdr.kernels[i].name, cisaHdr.kernels[i].name_len * sizeof(uint8_t), &buf[byte_pos],
                cisaHdr.kernels[i].name_len * sizeof(uint8_t));
        cisaHdr.kernels[i].name[cisaHdr.kernels[i].name_len] = '\0';
        byte_pos += cisaHdr.kernels[i].name_len;
        READ_FIELD_FROM_BUF(cisaHdr.kernels[i].offset, uint32_t);
        READ_FIELD_FROM_BUF(cisaHdr.kernels[i].size, uint32_t);
        READ_FIELD_FROM_BUF(cisaHdr.kernels[i].input_offset, uint32_t);

        READ_FIELD_FROM_BUF(
            cisaHdr.kernels[i].variable_reloc_symtab.num_syms,
            uint16_t);
        assert(cisaHdr.kernels[i].variable_reloc_symtab.num_syms == 0 && "relocation symbols not allowed");
        cisaHdr.kernels[i].variable_reloc_symtab.reloc_syms = nullptr;

        READ_FIELD_FROM_BUF(
            cisaHdr.kernels[i].function_reloc_symtab.num_syms,
            uint16_t);

        assert(cisaHdr.kernels[i].function_reloc_symtab.num_syms == 0 && "relocation symbols not allowed");
        cisaHdr.kernels[i].function_reloc_symtab.reloc_syms = nullptr;
        READ_FIELD_FROM_BUF(cisaHdr.kernels[i].num_gen_binaries, uint8_t);
        cisaHdr.kernels[i].gen_binaries =
            (gen_binary_info*)mem->alloc(cisaHdr.kernels[i].num_gen_binaries * sizeof(gen_binary_info));
        for (int j = 0; j < cisaHdr.kernels[i].num_gen_binaries; j++) {
            READ_FIELD_FROM_BUF(cisaHdr.kernels[i].gen_binaries[j].platform, uint8_t);
            READ_FIELD_FROM_BUF(cisaHdr.kernels[i].gen_binaries[j].binary_offset, uint32_t);
            READ_FIELD_FROM_BUF(cisaHdr.kernels[i].gen_binaries[j].binary_size, uint32_t);
        }

        cisaHdr.kernels[i].cisa_binary_buffer = NULL;
        cisaHdr.kernels[i].genx_binary_buffer = NULL;
    }

    READ_FIELD_FROM_BUF(cisaHdr.num_filescope_variables, uint16_t);
    assert(cisaHdr.num_filescope_variables == 0 && "file scope variables are no longer supported");

    READ_FIELD_FROM_BUF(cisaHdr.num_functions, uint16_t);

    if (cisaHdr.num_functions) {
        cisaHdr.functions =
            (function_info_t *)mem->alloc(
            sizeof(function_info_t)* cisaHdr.num_functions);
        ALLOC_ASSERT(cisaHdr.functions);
    }
    else {
        cisaHdr.functions = NULL;
    }

    for (int i = 0; i < cisaHdr.num_functions; i++) {
        // field is deprecated
        READ_FIELD_FROM_BUF(cisaHdr.functions[i].linkage, uint8_t);

        if (cisaHdr.major_version == 3 && cisaHdr.minor_version < 7)
        {
            READ_FIELD_FROM_BUF(cisaHdr.functions[i].name_len, uint8_t);
        }
        else
        {
            READ_FIELD_FROM_BUF(cisaHdr.functions[i].name_len, uint16_t);
        }
        cisaHdr.functions[i].name = (char*)mem->alloc(cisaHdr.functions[i].name_len + 1);
        memcpy_s(
            cisaHdr.functions[i].name, cisaHdr.functions[i].name_len * sizeof(uint8_t), &buf[byte_pos],
            cisaHdr.functions[i].name_len * sizeof(uint8_t));
        cisaHdr.functions[i].name[
            cisaHdr.functions[i].name_len] = '\0';
            byte_pos += cisaHdr.functions[i].name_len;
            READ_FIELD_FROM_BUF(cisaHdr.functions[i].offset, uint32_t);
            READ_FIELD_FROM_BUF(cisaHdr.functions[i].size, uint32_t);

            READ_FIELD_FROM_BUF(
                cisaHdr.functions[i].variable_reloc_symtab.num_syms,
                uint16_t);
            assert(cisaHdr.functions[i].variable_reloc_symtab.num_syms == 0 && "variable relocation not supported");
            cisaHdr.functions[i].variable_reloc_symtab.reloc_syms = nullptr;

            READ_FIELD_FROM_BUF(
                cisaHdr.functions[i].function_reloc_symtab.num_syms,
                uint16_t);
            assert(cisaHdr.functions[i].function_reloc_symtab.num_syms == 0 && "function relocation not supported");
            cisaHdr.functions[i].function_reloc_symtab.reloc_syms = nullptr;

            cisaHdr.functions[i].cisa_binary_buffer = NULL;
            cisaHdr.functions[i].genx_binary_buffer = NULL;
    }

    return 0;
}

const char *ChannelMask::Names[] = {
    "0000", // ABGR
    "R",    // 0001
    "G",    // 0010
    "RG",   // 0011
    "B",    // 0100
    "RB",   // 0101
    "GB",   // 0110
    "RGB",  // 0111
    "A",    // 1000
    "RA",   // 1001
    "GA",   // 1010
    "RGA",  // 1011
    "BA",   // 1100
    "RBA",  // 1101
    "GBA",  // 1110
    "RGBA"  // 1111
};
