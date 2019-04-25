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
#include "visa_igc_common_header.h"

#include "Common_ISA.h"
#include "Mem_Manager.h"
#include "VISADefines.h"

#define ALLOC_ASSERT(X)      \
    if (X == NULL) return 1; \

const char * implictKindStrings[IMPLICIT_INPUT_COUNT] = { "EXPLICIT", "LOCAL_SIZE", "GROUP_COUNT", "LOCAL_ID" };

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

const char* vme_stream_mode_str[4] =
{
    "00",
    "01",
    "10",
    "11"
};

const char* vme_search_ctrl_str[4] =
{
    "000",
    "001",
    "011",
    "111"
};

const char* special_opnd_type_str[S_OPND_NUM] =
{
    "00",
    "01",
    "10",
    "11"
};

const char* pred_ctrl_str[9] =
{
    "",
    "any2h",
    "any4h",
    "any8h",
    "any16h",
    "all2h",
    "all4h",
    "all8h",
    "all16h"
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

const char* SAMPLE_OP_3D_NAME[VISA_3D_TOTAL_NUM_OPS] =
{
    "sample_3d",   // 0x0.  The "3d" suffix is added to disambiguate it from the legacy sample
    "sample_b",    // 0x1
    "sample_l",    // 0x2
    "sample_c",    // 0x3
    "sample_d",    // 0x4
    "sample_b_c",  // 0x5
    "sample_l_c",  // 0x6
    "load_3d",     // 0x7.  The "3d" suffix is added to disambiguate it from the legacy sample
    "sample4",     // 0x8
    "",
    "resinfo",     // 0xA
    "sampleinfo",  // 0xB
    "sample+killpix", // 0xC
    "", "", "",
    "sample4_c",   // 0x10
    "sample4_po",  // 0x11
    "sample4_po_c",// 0x12
    "",
    "sample_d_c",  // 0x14
    "", "", "",
    "sample_lz",   // 0x18
    "sample_c_lz", // 0x19
    "load_lz",     // 0x1A
    "",
    "load_2dms_w", // 0x1C
    "load_mcs"     // 0x1D
};

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
    "CM_Y16_FORMAT",
    "CM_Y8_FORMAT"
};

const char * lbp_creation_mode[3] =
{
    "CM_5x5_mode",
    "CM_3x3_mode",
    "CM_BOTH_mode"
};

const char * avs_control_str[4] =
{   "CM_16_FULL",
    "CM_16_DOWN_SAMPLE",
    "CM_8_FULL",
    "CM_8_DOWN_SAMPLE"
};

const char * avs_exec_mode[3] =
{   "CM_AVS_16x4",
    "CM_AVS_INVALID",
    "CM_AVS_16x8"
};

const char * mmf_exec_mode[4] =
{   "CM_MMF_16x4",
    "CM_MMF_INVALID",
    "CM_MMF_16x1",
    "CM_MMF_1x1"
};

const char * mmf_enable_mode[3] =
{   "CM_MINMAX_ENABLE",
    "CM_MAX_ENABLE",
    "CM_MIN_ENABLE"
};

const char * ed_exec_mode[4] =
{   "CM_ED_64x4",
    "CM_ED_32x4",
    "CM_ED_64x1",
    "CM_ED_32x1"
};

const char * conv_exec_mode[4] =
{   "CM_CONV_16x4",
    "CM_CONV_INVALID",
    "CM_CONV_16x1",
    "CM_CONV_1x1"
};

unsigned format_control_byteSize2[4] =
{   4, /// CM_16_FULL
    2, /// CM_16_DOWN_SAMPLE NOT VALID
    2, /// CM_8_FULL
    1  /// CM_8_DOWN_SAMPLE NOT VALID
};

unsigned ed_exec_mode_byte_size[4] =
{   64 * 4 / 8, /// CM_ED_64x4
    32 * 4 / 8, /// CM_ED_32x4
    64 * 1 / 8, /// CM_ED_64x1
    32 * 1 / 8  /// CM_ED_32x1
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
    { ISA_TYPE_HF,  "hf",   2 }
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
        READ_FIELD_FROM_BUF(cisaHdr.kernels[i].name_len, uint8_t);
        memcpy_s(
            cisaHdr.kernels[i].name, COMMON_ISA_MAX_FILENAME_LENGTH, &buf[byte_pos],
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


        cisaHdr.kernels[i].scratch = NULL;
        cisaHdr.kernels[i].cisa_binary_buffer = NULL;
        cisaHdr.kernels[i].genx_binary_buffer = NULL;
        cisaHdr.kernels[i].stack_call = false;
        cisaHdr.kernels[i].num_callers = 0;
    }

    READ_FIELD_FROM_BUF(cisaHdr.num_filescope_variables, uint16_t);

    cisaHdr.num_extern_variables = cisaHdr.num_static_variables = cisaHdr.num_global_variables = 0;

    if (cisaHdr.num_filescope_variables > 0) {
        cisaHdr.filescope_variables =
            (filescope_var_info_t *)mem->alloc(
            sizeof(filescope_var_info_t)*
            cisaHdr.num_filescope_variables);
        ALLOC_ASSERT(cisaHdr.filescope_variables);
    }
    else {
        cisaHdr.filescope_variables = NULL;
    }

    for (int i = 0; i < cisaHdr.num_filescope_variables; i++) {
        READ_FIELD_FROM_BUF(cisaHdr.filescope_variables[i].linkage, uint8_t);
        if (cisaHdr.filescope_variables[i].linkage == CISA_LINKAGE_EXTERN)
            cisaHdr.num_extern_variables++;
        else if (cisaHdr.filescope_variables[i].linkage == CISA_LINKAGE_STATIC)
            cisaHdr.num_static_variables++;
        else
            cisaHdr.num_global_variables++;

        READ_FIELD_FROM_BUF(
            cisaHdr.filescope_variables[i].name_len, uint16_t);

        if (cisaHdr.filescope_variables[i].name_len) {
            cisaHdr.filescope_variables[i].name =
                (uint8_t*)mem->alloc(
                (cisaHdr.filescope_variables[i].name_len + 1) *
                sizeof(char));
            ALLOC_ASSERT(cisaHdr.filescope_variables[i].name);
        }
        else {
            cisaHdr.filescope_variables[i].name = NULL;
        }

        memcpy_s(
            cisaHdr.filescope_variables[i].name, cisaHdr.filescope_variables[i].name_len + 1, &buf[byte_pos],
            cisaHdr.filescope_variables[i].name_len * sizeof(uint8_t));
        cisaHdr.filescope_variables[i].name[
            cisaHdr.filescope_variables[i].name_len] = '\0';
            byte_pos += cisaHdr.filescope_variables[i].name_len;
            READ_FIELD_FROM_BUF(
                cisaHdr.filescope_variables[i].bit_properties, uint8_t);
            READ_FIELD_FROM_BUF(
                cisaHdr.filescope_variables[i].num_elements, uint16_t);
            READ_FIELD_FROM_BUF(
                cisaHdr.filescope_variables[i].attribute_count,
                uint8_t);

            if (cisaHdr.filescope_variables[i].attribute_count) {
                cisaHdr.filescope_variables[i].attributes =
                    (attribute_info_t *)mem->alloc
                    (sizeof(attribute_info_t)*
                    cisaHdr.filescope_variables[i].attribute_count);
                ALLOC_ASSERT(cisaHdr.filescope_variables[i].attributes);
            }
            else {
                cisaHdr.filescope_variables[i].attributes = NULL;
            }

            // FIXME: this is wrong and needs to be fixed to call
            //        IR_Builder's code

            for (int j = 0;
                j < cisaHdr.filescope_variables[i].attribute_count;
                j++) {
                READ_FIELD_FROM_BUF(
                    cisaHdr.filescope_variables[i].attributes[j].nameIndex,
                    unsigned short);
                READ_FIELD_FROM_BUF(
                    cisaHdr.filescope_variables[i].attributes[j].size,
                    unsigned char);
                cisaHdr.filescope_variables[i].attributes[j].value.stringVal =
                    (char*)mem->alloc
                    (sizeof(char)*
                    cisaHdr.filescope_variables[i].attributes[j].size);
            }

            cisaHdr.filescope_variables[i].scratch = NULL;
    }

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

    cisaHdr.num_extern_functions = cisaHdr.num_static_functions = cisaHdr.num_global_functions = 0;

    for (int i = 0; i < cisaHdr.num_functions; i++) {
        READ_FIELD_FROM_BUF(cisaHdr.functions[i].linkage, uint8_t);
        if (cisaHdr.functions[i].linkage == CISA_LINKAGE_EXTERN)
            cisaHdr.num_extern_functions++;
        else if (cisaHdr.functions[i].linkage == CISA_LINKAGE_STATIC)
            cisaHdr.num_static_functions++;
        else
            cisaHdr.num_global_functions++;

        READ_FIELD_FROM_BUF(cisaHdr.functions[i].name_len, uint8_t);
        memcpy_s(
            cisaHdr.functions[i].name, COMMON_ISA_MAX_FILENAME_LENGTH, &buf[byte_pos],
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

            cisaHdr.functions[i].scratch = NULL;
            cisaHdr.functions[i].cisa_binary_buffer = NULL;
            cisaHdr.functions[i].genx_binary_buffer = NULL;
            cisaHdr.functions[i].stack_call = false;
            cisaHdr.functions[i].num_callers = 0;
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
