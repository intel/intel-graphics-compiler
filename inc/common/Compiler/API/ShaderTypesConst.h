/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef STRUCTURE_ALINGMENT_VERIFICATION
#pragma once
#endif


/*****************************************************************************\
MACRO: MAX
\*****************************************************************************/
#ifndef MAX
#define MAX( x, y ) (((x)>=(y))?(x):(y))
#endif

namespace USC
{

/*****************************************************************************\

\*****************************************************************************/
const unsigned int NUM_COLOR_BUFFER_SLOTS          = 8;
const unsigned int NUM_VIEWPORT_SLOTS              = 16;

const unsigned int BITS_PER_BYTE = 8;
const unsigned int BITS_PER_QWORD = BITS_PER_BYTE * sizeof( unsigned long long );
const unsigned int INPUT_RESOURCE_SLOT_COUNT = 128;
const unsigned int NUM_SHADER_RESOURCE_VIEW_SIZE   = ( INPUT_RESOURCE_SLOT_COUNT + 1 ) / BITS_PER_QWORD;

const unsigned int NUM_CONSTANT_REGISTER_SLOTS_OCL     = 96;
const unsigned int NUM_CONSTANT_REGISTER_SLOTS_NONOCL  = 32;
const unsigned int NUM_CONSTANT_REGISTER_SLOTS         =
    MAX( NUM_CONSTANT_REGISTER_SLOTS_OCL,
         NUM_CONSTANT_REGISTER_SLOTS_NONOCL );

const unsigned int NUM_SAMPLER_SLOTS_IN_DESCRIPTOR = 16;
const unsigned int NUM_SAMPLER_SLOTS               = 32;
const unsigned int NUM_SAMPLER_SLOTS_DECLARED_NOT_OPENGL   = 16; // Only ever declare 16 sampler slots for non gl
const unsigned int NUM_SURFACE_SLOTS               = 128;
const unsigned int NUM_TEXTURE_COORDINATES         = 16;
const unsigned int NUM_INDEX_BUFFER_SLOTS          = 1;
const unsigned int NUM_VERTEX_BUFFER_SLOTS         = 33;
const unsigned int NUM_VERTEX_ELEMENT_SLOTS        = 34;
const unsigned int NUM_STREAM_OUTPUT_BUFFER_SLOTS  = 4;
const unsigned int NUM_TEXTURE_BLEND_STAGES        = 8;
const unsigned int NUM_UNORDERED_ACCESS_VIEW_SLOTS = 159;
const unsigned int NUM_TGSM_SLOTS                  = 1;
const unsigned int NUM_JOURNAL_SLOTS               = 1;
const unsigned int NUM_TEXTURE_SLOTS               = 32;
const unsigned int NUM_EXTRA_PLANES                = 2;

// OGL: __GL_MAX_STAGE_SHADER_STORAGE_BLOCKS = 16
// DX: D3D11_1_UAV_SLOT_COUNT = 64
const unsigned int MAX_NUM_SHADER_UAV_SLOTS        = 64;

/*****************************************************************************\
CONST: SHADER_REGISTERS
\*****************************************************************************/
const unsigned int NUM_GSHADER_INPUT_VERTICES      = 6;
const unsigned int NUM_GSHADER_INPUT_VERTICES_3_0  = 32;

const unsigned int NUM_VSHADER_INPUT_REGISTERS     = 34;
const unsigned int NUM_VSHADER_INPUT_REGISTERS_2_0 = 18;
const unsigned int NUM_VSHADER_INPUT_REGISTERS_2_1 = 34;
const unsigned int NUM_VSHADER_INPUT_REGISTERS_PACKAGEABLE = 32;
const unsigned int NUM_HSHADER_INPUT_REGISTERS     = 32;
const unsigned int NUM_DSHADER_INPUT_REGISTERS     = 32;
const unsigned int NUM_GSHADER_INPUT_REGISTERS     = 34*NUM_GSHADER_INPUT_VERTICES;
const unsigned int NUM_GSHADER_INPUT_REGISTERS_2_0 = 18*NUM_GSHADER_INPUT_VERTICES;
const unsigned int NUM_GSHADER_INPUT_REGISTERS_2_1 = 34*NUM_GSHADER_INPUT_VERTICES;
const unsigned int NUM_GSHADER_INPUT_REGISTERS_3_0 = 34*NUM_GSHADER_INPUT_VERTICES_3_0;
const unsigned int NUM_PSHADER_INPUT_REGISTERS     = 32;

// There are five possible kinds of input declarations in compute shader:
// ThreadID
// ThreadIDInGroup
// ThreadIDInGroupFlattened (==ThreadIndex)
// ThreadGroupID
// ThreadGroupsCount - OpenGL 4.3 gl_NumWorkGroups built-in variable
const unsigned int NUM_CSHADER_INPUT_REGISTERS     = 5;
const unsigned int MAX_NUM_SHADER_INPUT_REGISTERS  =
    MAX( MAX( MAX( MAX( MAX( NUM_VSHADER_INPUT_REGISTERS,
              NUM_GSHADER_INPUT_REGISTERS ),
              NUM_PSHADER_INPUT_REGISTERS ),
              NUM_HSHADER_INPUT_REGISTERS ),
              NUM_DSHADER_INPUT_REGISTERS ),
              NUM_CSHADER_INPUT_REGISTERS );

const unsigned int NUM_CP_SCRATCH_SOURCES           = 34;
const unsigned int NUM_VSHADER_OUTPUT_REGISTERS     = 34;

const unsigned int NUM_VSHADER_OUTPUT_REGISTERS_2_0 = 18;
const unsigned int NUM_VSHADER_OUTPUT_REGISTERS_2_1 = 34;
const unsigned int NUM_HSHADER_OUTPUT_REGISTERS     = 32;
const unsigned int NUM_DSHADER_OUTPUT_REGISTERS     = 32;
const unsigned int NUM_GSHADER_OUTPUT_REGISTERS     = 32;
const unsigned int NUM_GSHADER_BUFFERED_OUTPUT_REGISTERS_AND_HEADER = NUM_GSHADER_OUTPUT_REGISTERS + 2; // +2 is for DW0-DW7 of VUE
const unsigned int NUM_GEN5_75_GSHADER_BUFFERED_OUTPUT_REGISTERS_AND_HEADER =
    NUM_GSHADER_BUFFERED_OUTPUT_REGISTERS_AND_HEADER + 2; // +2 is for DW8-DW15 of VUE for ILK

const unsigned int NUM_PSHADER_OUTPUT_COLOR_REGISTERS  = NUM_COLOR_BUFFER_SLOTS;
const unsigned int NUM_PSHADER_OUTPUT_DEPTH_REGISTERS  = 1;
const unsigned int NUM_PSHADER_OUTPUT_OMASK_REGISTERS  = 1;

const unsigned int NUM_PSHADER_OUTPUT_REGISTERS =
    ( NUM_PSHADER_OUTPUT_COLOR_REGISTERS +
      NUM_PSHADER_OUTPUT_DEPTH_REGISTERS +
      NUM_PSHADER_OUTPUT_OMASK_REGISTERS );

const unsigned int MAX_NUM_SHADER_OUTPUT_REGISTERS  =
    MAX( MAX( MAX( MAX( NUM_VSHADER_OUTPUT_REGISTERS,
              NUM_GSHADER_OUTPUT_REGISTERS ),
              NUM_PSHADER_OUTPUT_REGISTERS ),
              NUM_HSHADER_OUTPUT_REGISTERS ),
              NUM_DSHADER_OUTPUT_REGISTERS );

typedef unsigned int ShaderRegisterIndexType;
const ShaderRegisterIndexType SHADER_REGISTER_INDEX_NONE = 0xffffffff; // max uint

const unsigned int NUM_SHADER_INDEXED_TEMP_ARRAYS            = 4096;
const unsigned int NUM_SHADER_CONSTANT_REGISTERS_1_0         = 32;
const unsigned int NUM_SHADER_CONSTANT_REGISTERS_2_0_OCL     = 96;
// Max number of entries in 3DSTATE_GATHER_CONSTANT_*S is 254,
// so number of constant registers must be 63 -> (63*4 <= 254).
const unsigned int NUM_SHADER_CONSTANT_REGISTERS_2_0_NONOCL  = 63;
const unsigned int NUM_SHADER_CONSTANT_REGISTERS_2_0 =
    MAX( NUM_SHADER_CONSTANT_REGISTERS_2_0_OCL,
         NUM_SHADER_CONSTANT_REGISTERS_2_0_NONOCL );

const unsigned int NUM_SHADER_CONSTANT_REGISTERS =
    MAX( NUM_SHADER_CONSTANT_REGISTERS_1_0,
         NUM_SHADER_CONSTANT_REGISTERS_2_0 );

// Max number of constant buffers is 14 per Resource Binding HLD (for pre-Gen9)
// and per SM4 Shader Constants description: "You can bind up to
// 14 constant buffers per pipeline stage (2 additional slots are reserved for internal use)"
// USC uses 15th buffer as immediate constant buffer
const unsigned int NUM_SHADER_CONSTANT_BUFFER_REGISTERS    = 16;

const unsigned int NUM_SHADER_CONSTANT_BUFFER_ELEMENTS     = 4096;

const unsigned int NUM_SHADER_PREDICATE_REGISTERS          = 1;

const unsigned int NUM_SHADER_SAMPLER_REGISTERS    = NUM_SAMPLER_SLOTS;
const unsigned int NUM_SHADER_RESOURCE_REGISTERS   = NUM_SURFACE_SLOTS;
const unsigned int NUM_SHADER_UAV_REGISTERS        = NUM_UNORDERED_ACCESS_VIEW_SLOTS;
const unsigned int NUM_SHADER_JOURNAL_REGISTERS    = NUM_JOURNAL_SLOTS;
const unsigned int ISA2ILMAP_SIMD8_SLOT            = 0;
const unsigned int ISA2ILMAP_SIMD16_SLOT           = 1;
const unsigned int ISA2ILMAP_SIMD32_SLOT           = 2;
const unsigned int ISA2ILMAP_MAX_SLOTS             = 3;
const unsigned int ISA2ILMAP_INVALID_INDEX         = 0xFFFFFFFF;
const unsigned int ISA2ILMAP_INVALID_LINE_NUMBER   = 0;
const unsigned int ISA2ILMAP_INVALID_COLUMN_NUMBER = 0;
const unsigned int ISA2ILMAP_INVALID_FILE_INDEX    = 0;
const unsigned int ISA2ILMAP_PROFILING_INSTRUCTION = 0xFFFFFFFE;
const unsigned int ISA2ILMAP_PROLOG                = 0xFFFFFFFD;

// Max number of unsigned ints of registers that can be shared among a group of threads
const unsigned int NUM_SHADER_TGSM_REGISTERS       = 16384;    // 16k DWORDs SLM per half slice in IVB+

const unsigned int NUM_SHADER_TPM_REGISTERS        = 8192;

const unsigned int MAX_THREAD_GROUP_DIMENSION_2_1  = 1024;
const unsigned int MAX_THREAD_GROUP_SIZE_2_1       = 1024;

const unsigned int MAX_THREAD_GROUP_DIMENSION_3_0  = 1024;
const unsigned int MAX_THREAD_GROUP_SIZE_3_0       = 1024;

const unsigned int MAX_SHADER_FUNCTION_CALL_DEPTH  = 32;

const unsigned int NUM_2X_MULTISAMPLE_POSITIONS = 2;
const unsigned int NUM_4X_MULTISAMPLE_POSITIONS = 4;
const unsigned int NUM_8X_MULTISAMPLE_POSITIONS = 8;
const unsigned int NUM_16X_MULTISAMPLE_POSITIONS = 16;

const unsigned int VERTEX_GEN5_75_HEADER_STRIDE = 3;

const unsigned int PATCH_HEADER_SIZE = 1;

const unsigned int MAX_PATCH_CONSTANT_SHADERS = 128;

const unsigned int MAX_VERTICES_PER_PRIMITIVE_FOR_DUAL_OBJECT = 16;

const unsigned int MAX_SHADER_INPUT_DECL_GRF_USAGE = 15;

const unsigned int MAX_SHADER_INSTRUCTION_COUNT_JMP_USAGE = 8192;

// DX11 spec only defines max number of interfaces and call sites
// but the number of allowed function bodies and function tables
// is unlimited.
const unsigned int NUM_SHADER_INTERFACES       = 253;
const unsigned int NUM_SHADER_CALLSITES        = 4096;
// Initial size for declarations arrays, will grow at runtime if needed
const unsigned int NUM_SHADER_FBODY_INITSIZE   = 512;
const unsigned int NUM_SHADER_FTABLE_INITSIZE  = 128;

const unsigned int INDEXED_TEMP_ARRAY_TO_GRF_MAX_REGS = 40;
const unsigned int INDEXED_OUTPUTS_MAX_IN_REGS        = 32;

const unsigned int PS_NOS_FOG_COLOR_SUBREG            = 0;
const unsigned int PS_NOS_FOG_COEFFA_SUBREG           = 4;
const unsigned int PS_NOS_FOG_COEFFB_SUBREG           = 5;
const unsigned int PS_NOS_FOG_COEFFC_SUBREG           = 6;
const unsigned int PS_NOS_FOG_COEFFD_SUBREG           = 7;

// GRF subregister numbers for bump mapping matrix.
const unsigned int PS_NOS_BUMP_MATRIX0_SUBREG      = 0;
const unsigned int PS_NOS_BUMP_MATRIX1_SUBREG      = 4;
const unsigned int PS_NOS_BUMP_MATRIX2_SUBREG      = 0;
const unsigned int PS_NOS_BUMP_MATRIX3_SUBREG      = 4;
const unsigned int PS_NOS_BUMP_MATRIX4_SUBREG      = 0;
const unsigned int PS_NOS_BUMP_MATRIX5_SUBREG      = 4;
const unsigned int PS_NOS_BUMP_MATRIX6_SUBREG      = 0;
const unsigned int PS_NOS_BUMP_MATRIX7_SUBREG      = 4;

const unsigned int PS_NOS_BUMP_LUMINANCE0_SUBREG   = 0;
const unsigned int PS_NOS_BUMP_LUMINANCE1_SUBREG   = 2;
const unsigned int PS_NOS_BUMP_LUMINANCE2_SUBREG   = 4;
const unsigned int PS_NOS_BUMP_LUMINANCE3_SUBREG   = 6;
const unsigned int PS_NOS_BUMP_LUMINANCE4_SUBREG   = 0;
const unsigned int PS_NOS_BUMP_LUMINANCE5_SUBREG   = 2;
const unsigned int PS_NOS_BUMP_LUMINANCE6_SUBREG   = 4;
const unsigned int PS_NOS_BUMP_LUMINANCE7_SUBREG   = 6;

// GRF subregister numbers for bump mapping luminance parameters.
const unsigned int PS_NOS_BUMP_LSCALE_SUBREG       = 0;
const unsigned int PS_NOS_BUMP_LOFFSET_SUBREG      = 1;

// GRF number in NOS Constant Buffer
const unsigned int PS_NOS_FOG_COLOR_REG            = 0;
const unsigned int PS_NOS_FOG_COEFFA_REG           = 0;
const unsigned int PS_NOS_FOG_COEFFB_REG           = 0;
const unsigned int PS_NOS_FOG_COEFFC_REG           = 0;
const unsigned int PS_NOS_FOG_COEFFD_REG           = 0;

const unsigned int PS_NOS_BUMP_LUMINANCE0_REG      = 1;
const unsigned int PS_NOS_BUMP_LUMINANCE1_REG      = 1;
const unsigned int PS_NOS_BUMP_LUMINANCE2_REG      = 1;
const unsigned int PS_NOS_BUMP_LUMINANCE3_REG      = 1;

const unsigned int PS_NOS_BUMP_LUMINANCE4_REG      = 2;
const unsigned int PS_NOS_BUMP_LUMINANCE5_REG      = 2;
const unsigned int PS_NOS_BUMP_LUMINANCE6_REG      = 2;
const unsigned int PS_NOS_BUMP_LUMINANCE7_REG      = 2;

const unsigned int PS_NOS_BUMP_MATRIX0_REG         = 3;
const unsigned int PS_NOS_BUMP_MATRIX1_REG         = 3;
const unsigned int PS_NOS_BUMP_MATRIX2_REG         = 4;
const unsigned int PS_NOS_BUMP_MATRIX3_REG         = 4;
const unsigned int PS_NOS_BUMP_MATRIX4_REG         = 5;
const unsigned int PS_NOS_BUMP_MATRIX5_REG         = 5;
const unsigned int PS_NOS_BUMP_MATRIX6_REG         = 6;
const unsigned int PS_NOS_BUMP_MATRIX7_REG         = 6;

const unsigned int VS_NOS_USERCLIPPLANES_REG       = 0;
const unsigned int VS_NOS_USERCLIPPLANES_MASK_REG  = 3;

// unsigned int number in specified GRF register
const unsigned int PS_NOS_FOG_FUNCTION_SUBREG         = 0;
const unsigned int PS_NOS_FOG_SRC_CHANNEL_SUBREG      = 1;

const unsigned int VS_NOS_USERCLIPPLANES_MASK_SUBREG  = 0;

// Offset in NOS Constant Buffer
const unsigned int PS_NOS_FOG_COLOR_OFFSET            = PS_NOS_FOG_COLOR_REG            * 32 + PS_NOS_FOG_COLOR_SUBREG  * 4;
const unsigned int PS_NOS_FOG_COEFFA_OFFSET           = PS_NOS_FOG_COEFFA_REG           * 32 + PS_NOS_FOG_COEFFA_SUBREG * 4;
const unsigned int PS_NOS_FOG_COEFFB_OFFSET           = PS_NOS_FOG_COEFFB_REG           * 32 + PS_NOS_FOG_COEFFB_SUBREG * 4;
const unsigned int PS_NOS_FOG_COEFFC_OFFSET           = PS_NOS_FOG_COEFFC_REG           * 32 + PS_NOS_FOG_COEFFC_SUBREG * 4;
const unsigned int PS_NOS_FOG_COEFFD_OFFSET           = PS_NOS_FOG_COEFFD_REG           * 32 + PS_NOS_FOG_COEFFD_SUBREG * 4;
const unsigned int PS_NOS_BUMP_LUMINANCE0_OFFSET      = PS_NOS_BUMP_LUMINANCE0_REG      * 32;
const unsigned int PS_NOS_BUMP_LSCALE0_OFFSET         = PS_NOS_BUMP_LUMINANCE0_REG      * 32 + PS_NOS_BUMP_LSCALE_SUBREG * 4;
const unsigned int PS_NOS_BUMP_LOFFSET0_OFFSET        = PS_NOS_BUMP_LUMINANCE0_REG      * 32 + PS_NOS_BUMP_LOFFSET_SUBREG * 4;
const unsigned int PS_NOS_BUMP_MATRIX0_OFFSET         = PS_NOS_BUMP_MATRIX0_REG         * 32 + PS_NOS_BUMP_MATRIX0_SUBREG * 4;

const unsigned int VS_NOS_USERCLIPPLANES_OFFSET       = VS_NOS_USERCLIPPLANES_REG       * 32;
const unsigned int VS_NOS_USERCLIPPLANES_MASK_OFFSET  = VS_NOS_USERCLIPPLANES_MASK_REG  * 32 + VS_NOS_USERCLIPPLANES_MASK_SUBREG * 4;

/*****************************************************************************\
CONST: Maximum number of vertex elements to be pushed into GRF in a Hull Shader
\*****************************************************************************/
static const unsigned int g_cMaxHSSinglePatchPushVertexElements = 128;

/*****************************************************************************\
CONST: Maximum number of vertex elements to be pushed into GRF in a Hull Shader
\*****************************************************************************/
static const unsigned int g_cMaxHSDualEightPatchPushVertexElements = 110;

/*****************************************************************************\
CONST: Maximum number of Hull Shader thread instances
\*****************************************************************************/
static const unsigned int g_cMaxHSThreadInstances = 16;

/*****************************************************************************\
CONST: Minimum number of Hull Shader mini-shader instructions to not get
       merged and allowed to be a separate shader
\*****************************************************************************/
static const unsigned int g_cMinHSMiniShaderInstructions = 16;

/*****************************************************************************\
CONST: Maximum number of Hull Shader Control Point Shader instructions to
       be considered a pass-through shader.
\*****************************************************************************/
static const unsigned int g_cMaxHSPassthroughInstructions = 64;

}
