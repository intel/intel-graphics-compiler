/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef STRUCTURE_ALINGMENT_VERIFICATION
#pragma once
#endif

#include "usc_gen8.h"

namespace USC
{


/*****************************************************************************\
ENUM:   BLEND_OPTIMIZATION_MODE
\*****************************************************************************/
enum BLEND_OPTIMIZATION_MODE
{
    BLEND_OPTIMIZATION_NONE,                        // No optimizations
    BLEND_OPTIMIZATION_SRC_ALPHA,                   // Discard: src.a == 0, Fill: src.a == 1
    BLEND_OPTIMIZATION_INV_SRC_ALPHA,               // Discard: src.a == 1, Fill: src.a == 0
    BLEND_OPTIMIZATION_SRC_ALPHA_DISCARD_ONLY,      // Discard: src.a == 0
    BLEND_OPTIMIZATION_SRC_ALPHA_FILL_ONLY,         // Fill:    src.a == 1
    BLEND_OPTIMIZATION_SRC_COLOR_ZERO,              // Discard: src.rgb == 0
    BLEND_OPTIMIZATION_SRC_COLOR_ONE,               // Discard: src.rgb == 1
    BLEND_OPTIMIZATION_SRC_BOTH_ZERO,               // Discard: src.rgba == 0
    BLEND_OPTIMIZATION_SRC_BOTH_ONE,                // Discard: src.rgba == 1
    BLEND_OPTIMIZATION_SRC_ALPHA_OR_COLOR_ZERO,     // Discard: src.a == 0 || src.rgb == 0
    BLEND_OPTIMIZATION_SRC_COLOR_ZERO_ALPHA_ONE,    // Discard: src.rgb == 0 && src.a == 1
    BLEND_OPTIMIZATION_SRC_COLOR_ZERO_ALPHA_IGNORE, // Discard: src.rgb == 0 and don't compute src.a
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputVertexShader_Gen9 : public SCompilerOutputVertexShader_Gen8
{
    bool         m_EnableVertexReordering;

    PADDING_1_BYTE
    PADDING_2_BYTES
    PADDING_4_BYTES
};



/*****************************************************************************\
STRUCT: SCompilerInputHullShader
\*****************************************************************************/
struct SCompilerInputHullShader : public SCompilerInputCommon
{
    float                                       m_TessellationFactorScale;
};

/* Structures for compile shader inputs*/

struct SCompilerInputVertexShader : public SCompilerInputCommon
{
};
struct SCompilerInputDomainShader : public SCompilerInputCommon
{
};
struct SCompilerInputGeometryShader : public SCompilerInputCommon
{
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputHullShader_Gen9 : public SCompilerOutputHullShader_Gen8
{
    // ### DW7 3DSTATE_HS ###
    unsigned int   DispatchMode;                        // HS dispatch mode         (DW7, BITFIELD_RANGE( 17,18 ))
    unsigned int   IncludePrimitiveID;                  // HS primitive ID          (DW7, BITFIELD(0))
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputDomainShader_Gen9 : public SCompilerOutputDomainShader_Gen7
{
    // ### DW7 3DSTATE_DS ###
    unsigned int    DispatchMode;                       //(DW7, bits  4...3)

    // ### DW8 3DSTATE_DS ###
    unsigned int    m_VertexURBEntryOutputReadOffset;   //(DW8, bits 26..21)
    unsigned int    m_VertexURBEntryOutputLength;       //(DW8, bits 20..16)

    // ### DW9 3DSTATE_DS ###
    unsigned int    m_DualPatchKernelProgramSize;
    void*           m_pDualPatchKernelProgram;          //(DW9, bits 31...6)

    bool            m_HasPrimitiveIdInput;

    PADDING_1_BYTE
    PADDING_2_BYTES
    PADDING_4_BYTES
    PADDING_4_BYTES_x32_ONLY
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputGeometryShader_Gen9 : public SCompilerOutputGeometryShader_Gen8
{
    // identical to Gen8
};

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputPixelShader_Gen9 : public SCompilerOutputPixelShader_Gen8
{
    GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT attributeActiveComponent[ NUM_PSHADER_INPUT_REGISTERS ];
    
    unsigned int OutputUseMask[ NUM_PSHADER_OUTPUT_REGISTERS ];

    // ### DW1 3DSTATE_PS_EXTRA ###
    GFX3DSTATE_PSEXTRA_INPUT_COVERAGE_MASK_MODE  m_InputCoverageMaskMode; // (bit 1..0)
    unsigned int m_BarycentricParamsArePulled; // (bit 3)
    unsigned int m_PixelShaderComputesStencil; // (bit 5)

    unsigned int m_FP16PayloadEnable;          // (bit 17)
    unsigned int m_CRastBinaryHeaderSize;
    unsigned int m_SampleCmpToDiscardOptimizationSlot;
    bool m_NeedsWAForSampleLInLoopPS;
    bool m_SampleCmpToDiscardOptimizationPossible;
    bool m_NeedPSSync;
    PADDING_1_BYTE
    PADDING_4_BYTES
};

/*****************************************************************************\
STRUCT: SCompilerInputComputeShader_Gen9
\*****************************************************************************/
struct SCompilerInputComputeShader_Gen9 : public SCompilerInputCommon_Gen7
{
    // False - legacy
    bool PooledEUMode;

    // Maximum size of available pool in physical (EU) threads
    unsigned int EUThreadsInPoolMax;
};

/*****************************************************************************\
\*****************************************************************************/
struct SCompilerOutputComputeShader_Gen9 : public SCompilerOutputComputeShader_Gen8
{
    // identical to Gen8
};

} // namespace USC
