/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "visa_igc_common_header.h"

enum class SIMDMode : unsigned char;

namespace IGC
{

    /*****************************************************************************\
    ENUM: EU_MESSAGE_TARGET
    \*****************************************************************************/
    enum EU_MESSAGE_TARGET
    {
        EU_MESSAGE_TARGET_NULL = 0,
        EU_MESSAGE_TARGET_MATHBOX = 1,
        EU_MESSAGE_TARGET_SAMPLER = 2,
        EU_MESSAGE_TARGET_GATEWAY = 3,
        EU_MESSAGE_TARGET_DATA_PORT_READ = 4,
        EU_MESSAGE_TARGET_DATA_PORT_WRITE = 5,
        EU_MESSAGE_TARGET_URB = 6,
        EU_MESSAGE_TARGET_THREAD_SPAWNER = 7,
        EU_MESSAGE_TARGET_SFID_VME = 8,
        EU_MESSAGE_TARGET_SFID_BTD = 7,
        EU_MESSAGE_TARGET_SFID_RTA = 8,
        EU_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_READ_ONLY = 9,
        EU_GEN7_MESSAGE_TARGET_DATA_PORT_DATA_CACHE = 10,
        EU_GEN7_MESSAGE_TARGET_PIXEL_INTERPOLATOR = 11,
        EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1 = 12,
        EU_MESSAGE_TARGET_SFID_CRE = 13,
        NUM_EU_MESSAGE_TARGETS
    };

    /*****************************************************************************\
    ENUM: EU_GEN7_5_VME_MESSAGE_TYPE
    \*****************************************************************************/
    enum EU_GEN7_5_VME_MESSAGE_TYPE
    {
        EU_GEN7_5_VME_MESSAGE_SIC = 1,
        EU_GEN7_5_VME_MESSAGE_IME = 2,
        EU_GEN7_5_VME_MESSAGE_FBR = 3
    };

    /*****************************************************************************\
    ENUM: EU_GEN6_SAMPLER_MESSAGE_TYPE
    \*****************************************************************************/
    enum EU_GEN6_SAMPLER_MESSAGE_TYPE
    {
        EU_GEN6_SAMPLER_MESSAGE_SAMPLE = 0,
        EU_GEN6_SAMPLER_MESSAGE_SAMPLE_B,
        EU_GEN6_SAMPLER_MESSAGE_SAMPLE_L,
        EU_GEN6_SAMPLER_MESSAGE_SAMPLE_C,
        EU_GEN6_SAMPLER_MESSAGE_SAMPLE_D,
        EU_GEN6_SAMPLER_MESSAGE_SAMPLE_BC,
        EU_GEN6_SAMPLER_MESSAGE_SAMPLE_LC,
        EU_GEN6_SAMPLER_MESSAGE_LD,
        EU_GEN6_SAMPLER_MESSAGE_LOAD4,
        EU_GEN6_SAMPLER_MESSAGE_LOD,
        EU_GEN6_SAMPLER_MESSAGE_RESINFO,
        EU_GEN6_SAMPLER_MESSAGE_SAMPLEINFO,
        EU_GEN6_SAMPLER_MESSAGE_SAMPLE_KILLPIX,

        EU_GEN6_SAMPLER_MESSAGE_GATHER4_C = 16,
        EU_GEN6_SAMPLER_MESSAGE_GATHER4_PO = 17,
        EU_GEN6_SAMPLER_MESSAGE_GATHER4_PO_C = 18,

        EU_GEN6_SAMPLER_MESSAGE_SAMPLE_D_C = 20,

        EU_GEN6_SAMPLER_MESSAGE_LD2DMS_W = 28,
        EU_GEN6_SAMPLER_MESSAGE_LD_MCS = 29,
        EU_GEN6_SAMPLER_MESSAGE_LD2DMS = 30,
        EU_GEN6_SAMPLER_MESSAGE_SAMPLE2DMS = 31
    };

    enum EU_PIXEL_INTERPOLATOR_MESSAGE_TYPE
    {
        EU_PI_MESSAGE_EVAL_PER_MESSAGE_OFFSET = 0,
        EU_PI_MESSAGE_EVAL_SAMPLE_POSITION = 1,
        EU_PI_MESSAGE_EVAL_CENTROID_POSITION = 2,
        EU_PI_MESSAGE_EVAL_PER_SLOT_OFFSET = 3
    };

    enum EU_PIXEL_INTERPOLATOR_INTERPOLATION_MODE
    {
        EU_PI_MESSAGE_PERSPECTIVE_INTERPOLATION = 0,
        EU_PI_MESSAGE_LINEAR_INTERPOLATION = 1
    };

    enum EU_PIXEL_INTERPOLATOR_SIMD_MODE
    {
        EU_PI_MESSAGE_SIMD8 = 0,
        EU_PI_MESSAGE_SIMD16 = 1
    };

    /*****************************************************************************\
    STRUCT: SEUPixelInterpolatorSampleIndexMessageDescriptorGen7_0
    \*****************************************************************************/
    struct SEUPixelInterpolatorSampleIndexMessageDescriptorGen7_0 //Gen8 uses the same
    {
        union _DW0
        {
            struct _All
            {
                DWORD : 4;
                        DWORD       SampleIndex : 4;
                        DWORD : 3;
                                DWORD       SlotGroupSelect : 1;
                                DWORD       MessageType : 2;
                                DWORD       InterpolationMode : 1;
                                DWORD : 1;
                                        DWORD       SIMDMode : 1;
                                        DWORD : 2;
                                                DWORD       HeaderPresent : 1;
                                                DWORD       ResponseLength : 5;
                                                DWORD       MessageLength : 4;
                                                DWORD : 2;
                                                        DWORD       EndOfThread : 1;    // bool
            } All;

            DWORD   Value;
        } DW0;
    };

    /*****************************************************************************\
    STRUCT: SEUPixelInterpolatorOffsetMessageDescriptorGen7_0
    \*****************************************************************************/
    struct SEUPixelInterpolatorOffsetMessageDescriptorGen7_0 //Gen8 uses the same
    {
        union _DW0
        {
            struct _All
            {
                DWORD       PerMessageXOffset : 4;
                DWORD       PerMessageYOffset : 4;
                DWORD : 3;
                        DWORD       SlotGroupSelect : 1;
                        DWORD       MessageType : 2;
                        DWORD       InterpolationMode : 1;
                        DWORD : 1;
                                DWORD       SIMDMode : 1;
                                DWORD : 2;
                                        DWORD       HeaderPresent : 1;
                                        DWORD       ResponseLength : 5;
                                        DWORD       MessageLength : 4;
                                        DWORD : 2;
                                                DWORD       EndOfThread : 1;    // bool
            } All;

            DWORD   Value;
        } DW0;
    };

    /*****************************************************************************\
    STRUCT: SEUPixelInterpolatorMessageDescriptorGen7_0
    \*****************************************************************************/
    struct SEUPixelInterpolatorMessageDescriptorGen7_0 //Gen8 uses the same
    {
        union _DW0
        {
            struct _All
            {
                DWORD       MessageSpecificControl : 8;
                DWORD : 3;
                        DWORD       SlotGroupSelect : 1;
                        DWORD       MessageType : 2;
                        DWORD       InterpolationMode : 1;
                        DWORD       ShadingRate : 1;
                        DWORD       SIMDMode : 1;
                        DWORD : 2;
                                DWORD       HeaderPresent : 1;
                                DWORD       ResponseLength : 5;
                                DWORD       MessageLength : 4;
                                DWORD : 2;
                                        DWORD       EndOfThread : 1;    // bool
            } All;

            DWORD   Value;
        } DW0;
    };

    /*****************************************************************************\
    ENUM: EU_DATA_PORT_WRITE_MESSAGE_TYPE
    \*****************************************************************************/
    enum EU_DATA_PORT_WRITE_MESSAGE_TYPE
    {
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_OWORD_BLOCK_WRITE = 0,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_OWORD_DUAL_BLOCK_WRITE = 1,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_DWORD_BLOCK_WRITE = 2,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_DWORD_SCATTERED_WRITE = 3,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_RENDER_TARGET_WRITE = 4,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_STREAMED_VERTEX_BUFFER_WRITE = 5,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_RENDERTARGET_UNORM_WRITE = 6, // Only for Gen4.5 onwards
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_FLUSH_RENDER_CACHE = 7,

        EU_DATA_PORT_WRITE_MESSAGE_TYPE_DWORD_ATOMIC_WRITE_MESSAGE = 8, // For Gen6

        // Gen7.0 onwards
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_BYTE_SCATTERED_WRITE = 8,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE = 9,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_TYPED_SURFACE_WRITE = 10,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION = 11,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION = 12,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_MEMORY_FENCE = 13,

        // Gen7.5
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_COUNTER_ATOMIC_OPERATION = 14,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_COUNTER_ATOMIC_OPERATION_SIMD4x2 = 15,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION_SIMD4x2 = 16,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION_SIMD4x2 = 17,

        // Gen 8
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_UNTYPED_SURFACE_WRITE = 18,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_SCATTERED_WRITE = 19,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_OPERATION = 20,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_OPERATION_SIMD4X2 = 21,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_BLOCK_WRITE = 22,

        // Gen 9 onwards
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_ATOMIC_FLOAT_OPERATION = 23,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_ATOMIC_FLOAT_OPERATION_SIMD4X2 = 24,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_FLOAT_OPERATION = 25,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_FLOAT_OPERATION_SIMD4X2 = 26,
        EU_DATA_PORT_WRITE_SCALED_MESSAGE_TYPE_BYTE_SCATTERED_WRITE = 27,

        NUM_EU_DATA_PORT_WRITE_MESSAGE_TYPES
    };

    /*****************************************************************************\
    ENUM: EU_DATA_PORT_A64_BYTE_SCATTERED_BLOCK_CONTROL
    \*****************************************************************************/
    enum EU_DATA_PORT_A64_BYTE_SCATTERED_BLOCK_CONTROL
    {
        EU_DATA_PORT_A64_BYTE_SCATTERED_BLOCK_CONTROL_8BYTES = 0,
        EU_DATA_PORT_A64_BYTE_SCATTERED_BLOCK_CONTROL_16BYTES = 16,
        EU_DATA_PORT_A64_BYTE_SCATTERED_BLOCK_CONTROL_8SHORTS = 4,
        EU_DATA_PORT_A64_BYTE_SCATTERED_BLOCK_CONTROL_16SHORTS = 20,
        EU_DATA_PORT_A64_BYTE_SCATTERED_BLOCK_CONTROL_8DWORDS = 8,
        EU_DATA_PORT_A64_BYTE_SCATTERED_BLOCK_CONTROL_16DWORDS = 24,
        EU_DATA_PORT_A64_BYTE_SCATTERED_BLOCK_CONTROL_8QWORDS = 2,
    };

    /*****************************************************************************\
    CONST: INVALID_MESSAGE_TYPE
    \*****************************************************************************/
    static const unsigned int INVALID_MESSAGE_TYPE = 0xFFFFFFFF;

    /*****************************************************************************\
    CONST: SCRATCH_SPACE_BTI
    \*****************************************************************************/
    static const unsigned int SCRATCH_SPACE_BTI = 255;

    /*****************************************************************************\
    CONST: STATELESS_BTI
    \*****************************************************************************/
    static const unsigned int STATELESS_BTI = 255;

    /*****************************************************************************\
    CONST: SLM_BTI
    \*****************************************************************************/
    static const unsigned int SLM_BTI = 254;

    /*****************************************************************************\
    CONST: STATELESS_NONCOHERENT_BTI
    \*****************************************************************************/
    static const unsigned int STATELESS_NONCOHERENT_BTI = 253;

    /*****************************************************************************\
    CONST: BINDLESS_BTI
    \*****************************************************************************/
    static const unsigned int BINDLESS_BTI = 252;

    /*****************************************************************************\
    CONST: SSH_BINDLESS_BTI
    \*****************************************************************************/
    static const unsigned int SSH_BINDLESS_BTI = 251;
    /*****************************************************************************\
    ENUM: EU_GEN7_DATA_CACHE_MESSAGE_TYPE
    \*****************************************************************************/
    enum EU_GEN7_DATA_CACHE_MESSAGE_TYPE
    {
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_OWORD_BLOCK_READ = 0,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ = 1,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ = 2,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_DWORD_SCATTERED_READ = 3,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_BYTE_SCATTERED_READ = 4,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_UNTYPED_SURFACE_READ = 5,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION = 6,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_MEMORY_FENCE = 7,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_OWORD_BLOCK_WRITE = 8,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_OWORD_DUAL_BLOCK_WRITE = 10,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_DWORD_SCATTERED_WRITE = 11,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_BYTE_SCATTERED_WRITE = 12,
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE = 13,
    };

    /*****************************************************************************\
    ENUM: EU_GEN7_RENDER_CACHE_MESSAGE_TYPE
    \*****************************************************************************/
    enum EU_GEN7_RENDER_CACHE_MESSAGE_TYPE
    {
        EU_GEN7_RENDER_CACHE_MESSAGE_TYPE_MEDIA_BLOCK_READ = 4,
        EU_GEN7_RENDER_CACHE_MESSAGE_TYPE_TYPED_SURFACE_READ = 5,
        EU_GEN7_RENDER_CACHE_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION = 6,
        EU_GEN7_RENDER_CACHE_MESSAGE_TYPE_MEMORY_FENCE = 7,
        EU_GEN7_RENDER_CACHE_MESSAGE_TYPE_MEDIA_BLOCK_WRITE = 10,
        EU_GEN7_RENDER_CACHE_MESSAGE_TYPE_RENDER_TARGET_WRITE = 12,
        EU_GEN7_RENDER_CACHE_MESSAGE_TYPE_TYPED_SURFACE_WRITE = 13,
    };

    /*****************************************************************************\
    ENUM: EU_GEN9_RENDER_CACHE_MESSAGE_TYPE
    \*****************************************************************************/
    enum EU_GEN9_RENDER_CACHE_MESSAGE_TYPE
    {
        EU_GEN9_RENDER_CACHE_MESSAGE_TYPE_RENDER_TARGET_WRITE = 12,
        EU_GEN9_RENDER_CACHE_MESSAGE_TYPE_RENDER_TARGET_READ = 13,
    };

    /*****************************************************************************\
    ENUM: EU_GEN9_DATA_PORT_RENDER_TARGET_READ_CONTROL
    \*****************************************************************************/
    enum EU_GEN9_DATA_PORT_RENDER_TARGET_READ_CONTROL
    {
        EU_GEN9_DATA_PORT_RENDER_TARGET_READ_CONTROL_SIMD16_SINGLE_SOURCE = 0,
        EU_GEN9_DATA_PORT_RENDER_TARGET_READ_CONTROL_SIMD8_SINGLE_SOURCE_LOW = 1,
        EU_GEN9_DATA_PORT_RENDER_TARGET_READ_CONTROL_PER_SAMPLE_ENABLE = 32
    };

    /*****************************************************************************\
    ENUM: EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL
    \*****************************************************************************/
    enum EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL
    {
        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD16_SINGLE_SOURCE = 0,
        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD16_SINGLE_SOURCE_REPLICATED = 1,
        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_DUAL_SOURCE_LOW = 2,
        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_DUAL_SOURCE_HIGH = 3,
        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_SINGLE_SOURCE_LOW = 4,
        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_IMAGE_WRITE = 5
    };

    /*****************************************************************************\
    ENUM: EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_SLOT_GROUP_SELECT
    \*****************************************************************************/
    enum EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_SLOT_GROUP_SELECT
    {
        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_SLOTGRP_LO = 0,
        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_SLOTGRP_HI = 8,
    };

    enum EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE
    {
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_TRANSPOSE_READ = 0,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_SURFACE_READ = 1,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION = 2,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION_SIMD4X2 = 3,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_MEDIA_BLOCK_READ = 4,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_TYPED_SURFACE_READ = 5,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION = 6,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION_SIMD4X2 = 7,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE = 9,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_MEDIA_BLOCK_WRITE = 10,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_ATOMIC_COUNTER_OPERATION = 11,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_ATOMIC_COUNTER_OPERATION_SIMD4x2 = 12,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_TYPED_SURFACE_WRITE = 13,
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_SCATTERED_READ = 16,
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_UNTYPED_SURFACE_READ = 17,
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_OPERATION = 18,
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_OPERATION_SIMD4X2 = 19,
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_BLOCK_READ = 20,
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_BLOCK_WRITE = 21,
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_FLOAT_ADD = 24,
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_UNTYPED_SURFACE_WRITE = 25,
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_SCATTERED_WRITE = 26,
        EU_GEN8_DATA_PORT_1_MESSAGE_TYPE_UNTYPED_ATOMIC_FLOAT = 27,
    };

    enum EU_GEN12_DATA_CACHE_1_MESSAGE_TYPE
    {
        EU_GEN12_DATA_CACHE_1_MESSAGE_TYPE_WORD_UNTYPED_ATOMIC_INTEGER = 3,
        EU_GEN12_DATA_CACHE_1_MESSAGE_TYPE_WORD_TYPED_ATOMIC_INTEGER = 7,
        EU_GEN12_DATA_CACHE_1_MESSAGE_TYPE_WORD_ATOMIC_COUNTER = 12,
        EU_GEN12_DATA_CACHE_1_MESSAGE_TYPE_A64_WORD_UNTYPED_ATOMIC_INTEGER = 19,
        EU_GEN12_DATA_PORT_1_MESSAGE_TYPE_WORD_UNTYPED_ATOMIC_FLOAT = 28,
        EU_GEN12_DATA_CACHE_1_MESSAGE_TYPE_A64_WORD_UNTYPED_ATOMIC_FLOAT = 30,
    };

    enum EU_GEN7_SAMPLER_CACHE_MESSAGE_TYPE
    {
        EU_GEN8_SAMPLER_CACHE_MESSAGE_TYPE_SURFACE_INFO = 0,
        EU_GEN7_SAMPLER_CACHE_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ = 1,
        EU_GEN7_SAMPLER_CACHE_MESSAGE_TYPE_MEDIA_BLOCK_READ = 4,
    };

    enum EU_GEN7_CONSTANT_CACHE_MESSAGE_TYPE
    {
        EU_GEN7_CONSTANT_CACHE_MESSAGE_TYPE_OWORD_BLOCK_READ = 0,
        EU_GEN7_CONSTANT_CACHE_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ = 1,
        EU_GEN7_CONSTANT_CACHE_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ = 2,
        EU_GEN7_CONSTANT_CACHE_MESSAGE_TYPE_DWORD_SCATTERED_READ = 3,
        EU_GEN9_CONSTANT_CACHE_MESSAGE_SURFACE_INFO = 6,
    };

    enum EU_DATA_PORT_INVALIDATE_AFTER_READ
    {
        EU_DATA_PORT_INVALIDATE_AFTER_READ_ENABLE = 32
    };


    enum DATA_PORT_TARGET_CACHE
    {
        DATA_PORT_TARGET_DATA_CACHE = 0,
        DATA_PORT_TARGET_RENDER_CACHE = 1,
        DATA_PORT_TARGET_SAMPLER_CACHE = 2,
        DATA_PORT_TARGET_CONSTANT_CACHE = 3,
        DATA_PORT_TARGET_DATA_CACHE_1 = 4
    };

    enum EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_PIXEL_SCOREBOARD
    {
        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_PIXEL_SCOREBOARD_CLEAR = 16
    };

    enum EU_DATA_PORT_UNTYPED_SURFACE_SIMD_MODE
    {
        EU_DATA_PORT_UNTYPED_SURFACE_SIMD_SIMD4X2 = 0,  // read message only
        EU_DATA_PORT_UNTYPED_SURFACE_SIMD_16 = 16,
        EU_DATA_PORT_UNTYPED_SURFACE_SIMD_8 = 32
    };

    enum EU_DATA_PORT_READ_MESSAGE_TYPE

    {
        EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_BLOCK_READ = 0,
        EU_DATA_PORT_READ_MESSAGE_TYPE_OWORD_DUAL_BLOCK_READ = 1,
        EU_DATA_PORT_READ_MESSAGE_TYPE_DWORD_BLOCK_READ = 2,
        EU_DATA_PORT_READ_MESSAGE_TYPE_DWORD_SCATTERED_READ = 3,

        // Gen4.5 onwards
        EU_DATA_PORT_READ_MESSAGE_TYPE_RENDERTARGET_UNORM_READ = 4,
        EU_DATA_PORT_READ_MESSAGE_TYPE_AVC_LOOPFILTER_READ = 5,

        // Gen6.0 onwards
        EU_DATA_PORT_READ_MESSAGE_TYPE_UNALIGNED_OWORD_BLOCK_READ = 6,

        // Gen7.0 onwards
        EU_DATA_PORT_READ_MESSAGE_TYPE_BYTE_SCATTERED_READ = 7,
        EU_DATA_PORT_READ_MESSAGE_TYPE_UNTYPED_SURFACE_READ = 8,
        EU_DATA_PORT_READ_MESSAGE_TYPE_TYPED_SURFACE_READ = 9,

        // Gen8.0 onwards
        EU_DATA_PORT_READ_MESSAGE_TYPE_A64_UNTYPED_SURFACE_READ = 10,
        EU_DATA_PORT_READ_MESSAGE_TYPE_A64_SCATTERED_READ = 11,
        EU_DATA_PORT_READ_MESSAGE_TYPE_A64_BLOCK_READ = 12,

        // Gen9.0 onwards
        EU_DATA_PORT_READ_MESSAGE_TYPE_TRANSPOSE_READ = 13,
        EU_DATA_PORT_READ_MESSAGE_TYPE_RENDER_TARGET_READ = 14,

        EU_DATA_PORT_READ_MESSAGE_TYPE_SURFACE_INFO_READ = 15,

        NUM_EU_DATA_PORT_READ_MESSAGE_TYPES
    };

    /*****************************************************************************\
    ENUM: EU_SAMPLER_SIMD_MODE
    \*****************************************************************************/
    enum EU_SAMPLER_SIMD_MODE
    {
        EU_SAMPLER_SIMD_SIMD4x2 = 0,
        EU_SAMPLER_SIMD_SIMD8 = 1,
        EU_SAMPLER_SIMD_SIMD16 = 2,
        EU_SAMPLER_SIMD_SIMD32 = 3,
        EU_SAMPLER_SIMD_SIMD64 = 3
    };

    /*****************************************************************************\
    STRUCT: SEUSamplerMessageDescriptorGen7
    \*****************************************************************************/
    struct SEUSamplerMessageDescriptorGen7 //Gen8 uses the same
    {
        union _DW0
        {
            struct _All
            {
                unsigned int       BindingTableIndex : 8;
                unsigned int       SamplerIndex : 4;
                unsigned int       MessageType : 5;
                unsigned int       SIMDMode : 2;
                unsigned int       HeaderPresent : 1;
                unsigned int       ResponseLength : 5;
                unsigned int       MessageLength : 4;
                unsigned int       FP16Input : 1;
                unsigned int       FP16Return : 1;
                unsigned int       EndOfThread : 1;    // bool
            } All;

            unsigned int   Value;
        } DW0;
    };

    const unsigned int cConvertDataPortWriteMessageType[NUM_EU_DATA_PORT_WRITE_MESSAGE_TYPES] =
    {
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_OWORD_BLOCK_WRITE,               // EU_DATA_PORT_WRITE_MESSAGE_TYPE_OWORD_BLOCK_WRITE
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_OWORD_DUAL_BLOCK_WRITE,          // EU_DATA_PORT_WRITE_MESSAGE_TYPE_OWORD_DUAL_BLOCK_WRITE
        INVALID_MESSAGE_TYPE,                                            // EU_DATA_PORT_WRITE_MESSAGE_TYPE_DWORD_BLOCK_WRITE
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_DWORD_SCATTERED_WRITE,           // EU_DATA_PORT_WRITE_MESSAGE_TYPE_DWORD_SCATTERED_WRITE
        EU_GEN9_RENDER_CACHE_MESSAGE_TYPE_RENDER_TARGET_WRITE,           // EU_DATA_PORT_WRITE_MESSAGE_TYPE_RENDER_TARGET_WRITE
        INVALID_MESSAGE_TYPE,                                            // EU_DATA_PORT_WRITE_MESSAGE_TYPE_STREAMED_VERTEX_BUFFER_WRITE
        INVALID_MESSAGE_TYPE,                                            // EU_DATA_PORT_WRITE_MESSAGE_TYPE_RENDERTARGET_UNORM_WRITE
        INVALID_MESSAGE_TYPE,                                            // EU_DATA_PORT_WRITE_MESSAGE_TYPE_FLUSH_RENDER_CACHE
        EU_GEN7_DATA_CACHE_MESSAGE_TYPE_BYTE_SCATTERED_WRITE,            // EU_DATA_PORT_WRITE_MESSAGE_TYPE_BYTE_SCATTERED_WRITE
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE,       // EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_TYPED_SURFACE_WRITE,         // EU_DATA_PORT_WRITE_MESSAGE_TYPE_TYPED_SURFACE_WRITE
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION,    // EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION,      // EU_DATA_PORT_WRITE_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION
        EU_GEN7_RENDER_CACHE_MESSAGE_TYPE_MEMORY_FENCE,                  // EU_DATA_PORT_WRITE_MESSAGE_TYPE_MEMORY_FENCE
        // EU_GEN7_DATA_CACHE_MESSAGE_TYPE_MEMORY_FENCE and EU_GEN7_RENDER_CACHE_MESSAGE_TYPE_MEMORY_FENCE are the same constants
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_ATOMIC_COUNTER_OPERATION,     // EU_DATA_PORT_WRITE_MESSAGE_TYPE_COUNTER_ATOMIC_OPERATION
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_ATOMIC_COUNTER_OPERATION_SIMD4x2, // EU_DATA_PORT_WRITE_MESSAGE_TYPE_COUNTER_ATOMIC_OPERATION_SIMD4x2
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION_SIMD4X2, // EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION_SIMD4x2
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION_SIMD4X2,   // EU_DATA_PORT_WRITE_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION_SIMD4x2
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_UNTYPED_SURFACE_WRITE,     // EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_UNTYPED_SURFACE_WRITE
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_SCATTERED_WRITE,           // EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_SCATTERED_WRITE
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_OPERATION,  // EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_OPERATION
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_OPERATION_SIMD4X2,  // EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_UNTYPED_ATOMIC_OPERATION_SIMD4X2
        EU_GEN8_DATA_CACHE_1_MESSAGE_TYPE_A64_BLOCK_WRITE,               // EU_DATA_PORT_WRITE_MESSAGE_TYPE_A64_BLOCK_WRITE
    };

    enum EU_DATA_PORT_DWORD_SCATTERED_BLOCK_CONTROL
    {
        EU_DATA_PORT_DWORD_SCATTERED_BLOCK_CONTROL_8DWORDS = 2,
        EU_DATA_PORT_DWORD_SCATTERED_BLOCK_CONTROL_16DWORDS = 3
    };

    /*****************************************************************************\
    ENUM: EU_DATA_PORT_ATOMIC_OPERATION_TYPE
    \*****************************************************************************/
    enum EU_DATA_PORT_ATOMIC_OPERATION_TYPE
    {
        EU_DATA_PORT_ATOMIC_OPERATION_AND = 1,
        EU_DATA_PORT_ATOMIC_OPERATION_OR = 2,
        EU_DATA_PORT_ATOMIC_OPERATION_XOR = 3,
        EU_DATA_PORT_ATOMIC_OPERATION_MOV = 4,
        EU_DATA_PORT_ATOMIC_OPERATION_INC = 5,
        EU_DATA_PORT_ATOMIC_OPERATION_DEC = 6,
        EU_DATA_PORT_ATOMIC_OPERATION_ADD = 7,
        EU_DATA_PORT_ATOMIC_OPERATION_SUB = 8,
        EU_DATA_PORT_ATOMIC_OPERATION_REVSUB = 9,
        EU_DATA_PORT_ATOMIC_OPERATION_IMAX = 10,
        EU_DATA_PORT_ATOMIC_OPERATION_IMIN = 11,
        EU_DATA_PORT_ATOMIC_OPERATION_UMAX = 12,
        EU_DATA_PORT_ATOMIC_OPERATION_UMIN = 13,
        EU_DATA_PORT_ATOMIC_OPERATION_CMPWR = 14,
        EU_DATA_PORT_ATOMIC_OPERATION_PREDEC = 15,

        // A64 Atomic Operations
        EU_DATA_PORT_A64_ATOMIC_OPERATION_AND = 16,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_OR = 17,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_XOR = 18,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_MOV = 19,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_INC = 20,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_DEC = 21,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_ADD = 22,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_SUB = 23,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_REVSUB = 24,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_IMAX = 25,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_IMIN = 26,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_UMAX = 27,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_UMIN = 28,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_CMPWR = 29,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_PREDEC = 30,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_CMPWR8B = 31,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_CMPWR16B = 32,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_FMIN = 33,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_FMAX = 34,
        EU_DATA_PORT_A64_ATOMIC_OPERATION_FCMPWR = 35,
    };

    /*****************************************************************************\
    STRUCT: SEUDataPortMessageDescriptorGen8_0
    \*****************************************************************************/
    struct SEUDataPortMessageDescriptorGen8_0
    {
        union _DW0
        {
            struct _All
            {
                unsigned int       BindingTableIndex : 8;
                unsigned int       MessageSpecificControl : 6;
                unsigned int       MessageType : 5;

                // cache message type is represented by 5-bits and for messages
                // that use Data Port Data Cache0 message type is represented
                // by 4-bits and should be using SEUDataPortMessageDescriptorGen7_0.
                unsigned int       HeaderPresent : 1;  // bool
                unsigned int       ResponseLength : 5;
                unsigned int       MessageLength : 4;
                unsigned int : 2;
                unsigned int       EndOfThread : 1;  // bool
            } All;

            unsigned int   Value;
        } DW0;
    };

    /*****************************************************************************\
    ENUM: EU_GW_FENCE_PORTS. Spec: Enumeration_GW_FENCE_PORTS.
    \*****************************************************************************/
    enum EU_GW_FENCE_PORTS
    {
        EU_GW_FENCE_PORTS_None = 0x0,
        EU_GW_FENCE_PORTS_SLM = 0x1,
        EU_GW_FENCE_PORTS_UGM = 0x2,
        EU_GW_FENCE_PORTS_UGML = 0x4,
        EU_GW_FENCE_PORTS_TGM = 0x8,
    };

    /*****************************************************************************\
    STRUCT: URBFenceGen12. Spec: Instruction_URBFence (symbol: MSD_URBFENCE).
    \*****************************************************************************/
    struct URBFenceGen12
    {
        union _DW0
        {
            struct _All
            {
                unsigned int       URBOpcode : 4; // [3:0]
                unsigned int : 16;
                unsigned int       ResponseLength : 5; // [24:20]
                unsigned int       MessageLength : 4; // [28:25]
                unsigned int : 3;
            } All;

            unsigned int   Value;
        } DW0;
    };

    /*****************************************************************************\
    STRUCT: EOTMessageDescriptorGen12. Spec: Instruction_EOT (symbol: MSD_EOT).
    \*****************************************************************************/
    struct EOTMessageDescriptorGen12
    {
        union _DW0
        {
            struct _All
            {
                unsigned int       EOTSubfunction : 3; // [2:0]
                unsigned int : 9;
                unsigned int       FenceDataPorts : 4; // [15:12]
                unsigned int : 4;
                unsigned int       ResponseLength : 5; // [24:20]
                unsigned int       MessageLength : 4; // [28:25]
                unsigned int : 3;
            } All;

            unsigned int   Value;
        } DW0;
    };

    /*****************************************************************************\
    STRUCT: TraceRayBTDMessageDescriptorGen12
    \*****************************************************************************/
    struct TraceRayBTDMessageDescriptorGen12
    {
        union _DW0
        {
            struct _All
            {
                unsigned int                      : 8;
                unsigned int       SIMDMode       : 1; // [8]
                unsigned int                      : 5;
                unsigned int       MessageType    : 4; // [17:14]
                unsigned int                      : 1;
                unsigned int       HeaderPresent  : 1; // [19]
                unsigned int       ResponseLength : 5; // [20:24]
                unsigned int       MessageLength  : 4; // [28:25]
                unsigned int : 3;
            } All;

            unsigned int   Value;
        } DW0;
    };

    /*****************************************************************************\
    STRUCT: VMEMessageDescriptorGen8_0
    \*****************************************************************************/
    struct VMEMessageDescriptorGen8_0
    {
        union _DW0
        {
            struct {
                uint32_t BindingTableIndex : 8;  // bit[7:0]
                uint32_t : 5;  // bit[12:8]
                           uint32_t MessageType : 2;  // bit[14:13]
                           uint32_t StreamOutEnable : 1;  // bit[15:15]
                           uint32_t StreamInEnable : 1;  // bit[16:16]
                           uint32_t : 2;  // bit[18:17]
                                      uint32_t HeaderPresent : 1;  // bit[19:19]
                                      uint32_t ResponseLength : 5;  // bit[24:20]
                                      uint32_t MessageLength : 4;  // bit[28:25]
            } All;

            uint32_t   Value;
        } DW0;
    };

    /*****************************************************************************\
    STRUCT: SEUPixelDataPortMessageDescriptorGen8_0
    \*****************************************************************************/
    struct SEUPixelDataPortMessageDescriptorGen8_0
    {
        union _DW0
        {
            struct _All
            {
                unsigned int       BindingTableIndex : 8;
                unsigned int       MessageSubType : 3;
                unsigned int       Slot : 1;
                unsigned int       LastRT : 1;
                unsigned int       PerSample : 1;
                unsigned int       MessageType : 4;
                unsigned int       PerCoarse : 1;
                unsigned int       HeaderPresent : 1;
                unsigned int       ResponseLength : 5;
                unsigned int       MessageLength : 4;
                unsigned int       Reserved : 1;
                unsigned int       PrecisionSubType : 1;
                unsigned int : 1;
            } All;

            unsigned int   Value;
        } DW0;
    };

    /*****************************************************************************\
    STRUCT: SEUURBMessageDescriptorGen8_0
    \*****************************************************************************/
    struct SEUURBMessageDescriptorGen8_0
    {
        union _DW0
        {
            struct _Simd8
            {
                unsigned int       URBOpcode : 4;
                unsigned int       GlobalOffset : 11;
                unsigned int       ChannelMaskPresent : 1;
                unsigned int : 1;
                unsigned int       PerSlotOffset : 1;
                unsigned int : 1;
                unsigned int       HeaderPresent : 1;
                unsigned int       ResponseLength : 5;
                unsigned int       MessageLength : 4;
                unsigned int : 2;
                unsigned int       reserved : 1;
            } Simd8;

            unsigned int   Value;
        } DW0;
    };

    enum EU_URB_OPCODE
    {
        EU_URB_OPCODE_WRITE_HWORD = 0,
        EU_URB_OPCODE_WRITE_OWORD = 1,
        EU_URB_OPCODE_READ_HWORD = 2,
        EU_URB_OPCODE_READ_OWORD = 3,
        EU_URB_OPCODE_ATOMIC_MOV = 4,
        EU_URB_OPCODE_ATOMIC_INC = 5,
        EU_URB_OPCODE_ATOMIC_ADD = 6,
        EU_URB_OPCODE_SIMD8_WRITE = 7,
        EU_URB_OPCODE_SIMD8_READ = 8,
        EU_URB_OPCODE_FENCE = 9,
    };

    EU_GEN6_SAMPLER_MESSAGE_TYPE GetSampleMessage(EOPCODE opCode);

    EU_SAMPLER_SIMD_MODE samplerSimdMode(SIMDMode simd);
    EU_PIXEL_INTERPOLATOR_SIMD_MODE pixelInterpolatorSimDMode(SIMDMode simd);
    EU_DATA_PORT_ATOMIC_OPERATION_TYPE getHwAtomicOpEnum(AtomicOp op);

    uint encodeMessageDescriptorForAtomicUnaryOp(
        const unsigned int  messageLength,
        const unsigned int  responseLength,
        bool headerPresent,
        const uint message_type,
        const bool returnData,
        const SIMDMode simdMode,
        EU_DATA_PORT_ATOMIC_OPERATION_TYPE atomic_op_type,
        uint binding_table_index);

    uint encodeMessageSpecificControlForReadWrite(
        const EU_DATA_PORT_READ_MESSAGE_TYPE messageType,
        const VISAChannelMask mask,
        const SIMDMode simdMode);

    uint encodeMessageSpecificControlForReadWrite(
        const EU_DATA_PORT_WRITE_MESSAGE_TYPE messageType,
        const VISAChannelMask mask,
        const SIMDMode simdMode);

    unsigned int Sampler(
        unsigned int messageLength,
        unsigned int responseLength,
        bool  headerPresent,
        EU_SAMPLER_SIMD_MODE executionMode,
        EU_GEN6_SAMPLER_MESSAGE_TYPE messageType,
        unsigned int samplerIndex,
        unsigned int resourceIndex,
        bool endOfThread,
        bool FP16Input,
        bool FP16Return);

    unsigned int DataPortWrite(
        const uint   messageLength,
        const uint   responseLength,
        const bool   headerPresent,
        const bool   endOfThread,
        const EU_DATA_PORT_WRITE_MESSAGE_TYPE messageType,
        const uint   messageSpecificControl,
        const bool   invalidateAfterReadEnable,
        const uint   bindingTableIndex);

    unsigned int PixelDataPort(
        const bool   precisionSubType,
        const uint   messageLength,
        const uint   responseLength,
        const bool   headerPresent,
        const bool   perCoarse,
        const bool   perSample,
        const bool   lastRT,
        const bool   secondHalf,
        const EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL messageSubType,
        const uint   bindingTableIndex);

    unsigned int DataPortRead(
        const unsigned int messageLength,
        const unsigned int responseLength,
        const bool headerPresent,
        const EU_DATA_PORT_READ_MESSAGE_TYPE messageType,
        const uint messageSpecificControl,
        const bool invalidateAfterReadEnableHint,
        const DATA_PORT_TARGET_CACHE targetCache,
        const unsigned int bindingTableIndex);

    unsigned int UrbMessage(
        const unsigned int  messageLength,
        const unsigned int  responseLength,
        const bool   endOfThread,
        const bool   perSlotOffset,
        const bool   channelMaskPresent,
        const unsigned int  globalOffset,
        const EU_URB_OPCODE urbOpcode);

    unsigned int PixelInterpolator(
        const unsigned int messageLength,
        const unsigned int responseLength,
        const unsigned int pass,
        EU_PIXEL_INTERPOLATOR_SIMD_MODE executionMode,
        EU_PIXEL_INTERPOLATOR_MESSAGE_TYPE messageType,
        EU_PIXEL_INTERPOLATOR_INTERPOLATION_MODE interpolationMode,
        const unsigned int sampleindex);

    unsigned int PixelInterpolator(
        unsigned int messageLength,
        unsigned int responseLength,
        unsigned int pass,
        EU_PIXEL_INTERPOLATOR_SIMD_MODE executionMode,
        EU_PIXEL_INTERPOLATOR_MESSAGE_TYPE messageType,
        EU_PIXEL_INTERPOLATOR_INTERPOLATION_MODE interpolationMode,
        unsigned int perMessageXOffset,
        unsigned int perMessageYOffset);

    unsigned int PixelInterpolator(
        const DWORD messageLength,
        const DWORD responseLength,
        const DWORD pass,
        bool  IsCoarse,
        EU_PIXEL_INTERPOLATOR_SIMD_MODE executionMode,
        EU_PIXEL_INTERPOLATOR_MESSAGE_TYPE messageType,
        EU_PIXEL_INTERPOLATOR_INTERPOLATION_MODE interpolationMode);

    unsigned int PIPullPixelPayload(
        EU_PIXEL_INTERPOLATOR_SIMD_MODE executionMode,
        DWORD responseLength,
        DWORD messageLenght,
        bool inputCoverage,
        bool linearCentroidBary,
        bool linearCenterBary,
        bool perspectiveCentroid,
        bool perspectiveCenter,
        bool OutputCoverageMask);

    uint32_t VMEDescriptor(
        COMMON_ISA_VME_STREAM_MODE streamMode,
        const uint32_t bti,
        const uint32_t msgType,
        const uint32_t regs2snd,
        const uint32_t regs2rcv
    );

    uint URBFence();

    uint EOTGateway(
        const EU_GW_FENCE_PORTS fencePorts);

    uint BindlessThreadDispatch(
        const uint   messageLength,
        const uint   SIMDMode,
        const bool   IsTraceMessage,
        const bool   IsRayQueryMessage);

}
