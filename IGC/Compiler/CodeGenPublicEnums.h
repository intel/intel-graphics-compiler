/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CODE_GEN_PUBLIC_ENUMS_H_
#define CODE_GEN_PUBLIC_ENUMS_H_

#include "../common/EmUtils.h"

namespace IGC
{
    enum HullShaderDispatchModes : unsigned int
    {
        SINGLE_PATCH_DISPATCH_MODE,
        DUAL_PATCH_DISPATCH_MODE,
        EIGHT_PATCH_DISPATCH_MODE,
    };

    enum DomainShaderDispatchModes : signed int
    {
        DS_SIMD4x2_DISPATCH_MODE = 0x0,
        DS_SINGLE_PATCH_DISPATCH_MODE = 0x1,
        DS_DUAL_PATCH_DISPATCH_MODE = 0x2
    };

    enum PixelShaderPhaseType
    {
        PSPHASE_COARSE,
        PSPHASE_PIXEL,
        PSPHASE_SAMPLE,
        PSPHASE_LEGACY,
    };

    enum e_interpolation : signed int
    {
        EINTERPOLATION_UNDEFINED,
        EINTERPOLATION_CONSTANT,
        EINTERPOLATION_LINEAR,
        EINTERPOLATION_LINEARCENTROID,
        EINTERPOLATION_LINEARNOPERSPECTIVE,
        EINTERPOLATION_LINEARNOPERSPECTIVECENTROID,
        EINTERPOLATION_LINEARSAMPLE,
        EINTERPOLATION_LINEARNOPERSPECTIVESAMPLE,
        EINTERPOLATION_VERTEX,
        NUMBER_EINTERPOLATION
    };

    enum ShaderOutputType : signed int
    {
        SHADER_OUTPUT_TYPE_DEFAULT = 0,
        SHADER_OUTPUT_TYPE_POSITION,
        SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO,
        SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI,
        SHADER_OUTPUT_TYPE_DEPTHOUT,
        SHADER_OUTPUT_TYPE_STENCIL,
        SHADER_OUTPUT_TYPE_VIEWPORT_ARRAY_INDEX,
        SHADER_OUTPUT_TYPE_RENDER_TARGET_ARRAY_INDEX,
        SHADER_OUTPUT_TYPE_POINTWIDTH,
        SHADER_OUTPUT_TYPE_OMASK,
        SHADER_OUTPUT_TYPE_FINAL_LINE_DENSITY_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_LINE_DETAIL_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_QUAD_U_INSIDE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_QUAD_V_INSIDE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_FINAL_TRI_INSIDE_TESSFACTOR,
        SHADER_OUTPUT_TYPE_COARSE_PIXEL_SIZE,
        SHADER_OUTPUT_TYPE_UNKNOWN
    };

    enum BufferType : short
    {
        CONSTANT_BUFFER = 0,
        UAV,
        RESOURCE,
        SLM,
        POINTER,
        BINDLESS,
        BINDLESS_CONSTANT_BUFFER,
        BINDLESS_TEXTURE,
        SAMPLER,
        BINDLESS_SAMPLER,
        RENDER_TARGET,
        STATELESS,
        STATELESS_READONLY,
        STATELESS_A32,
        SSH_BINDLESS,
        SSH_BINDLESS_CONSTANT_BUFFER,
        SSH_BINDLESS_TEXTURE,
        BINDLESS_READONLY,
        BINDLESS_WRITEONLY,
        BUFFER_TYPE_UNKNOWN
    };

    enum BufferAccessType : short
    {
        ACCESS_READ = 0,
        ACCESS_WRITE,
        ACCESS_READWRITE,
        ACCESS_SAMPLE,
        BUFFER_ACCESS_TYPE_UNKNOWN = 15,
    };

    enum ADDRESS_SPACE : unsigned int
    {
        ADDRESS_SPACE_PRIVATE = 0,
        ADDRESS_SPACE_GLOBAL = 1,
        ADDRESS_SPACE_CONSTANT = 2,
        ADDRESS_SPACE_LOCAL = 3,
        ADDRESS_SPACE_GENERIC = 4,
        ADDRESS_SPACE_THREAD_ARG = 5,

        ADDRESS_SPACE_GLOBAL_OR_PRIVATE = 20,

        ADDRESS_SPACE_NUM_ADDRESSES
    };

    enum AtomicOp : unsigned int
    {
        EATOMIC_IADD,
        EATOMIC_SUB,
        EATOMIC_INC,
        EATOMIC_DEC,
        EATOMIC_MIN,
        EATOMIC_MAX,
        EATOMIC_XCHG,
        EATOMIC_CMPXCHG,
        EATOMIC_AND,
        EATOMIC_OR,
        EATOMIC_XOR,
        EATOMIC_IMIN,
        EATOMIC_IMAX,
        EATOMIC_UMAX,
        EATOMIC_UMIN,
        EATOMIC_PREDEC,
        EATOMIC_FMIN,
        EATOMIC_FMAX,
        EATOMIC_FCMPWR,
        EATOMIC_FADD,
        EATOMIC_FSUB,
        EATOMIC_FADD64,
        EATOMIC_FSUB64,
        EATOMIC_LOAD,
        EATOMIC_STORE,
        //64 bit
        EATOMIC_IADD64,
        EATOMIC_SUB64,
        EATOMIC_INC64,
        EATOMIC_DEC64,
        EATOMIC_MIN64,
        EATOMIC_MAX64,
        EATOMIC_XCHG64,
        EATOMIC_CMPXCHG64,
        EATOMIC_AND64,
        EATOMIC_OR64,
        EATOMIC_XOR64,
        EATOMIC_IMIN64,
        EATOMIC_IMAX64,
        EATOMIC_UMAX64,
        EATOMIC_UMIN64,
        EATOMIC_PREDEC64,
        EATOMIC_UNDEF
    };

    enum class WaveOps : unsigned int
    {
        SUM,
        PROD,
        UMIN,
        UMAX,
        IMIN,
        IMAX,
        OR,
        XOR,
        AND,
        FSUM,
        FPROD,
        FMIN,
        FMAX,
        UNDEF
    };

    enum GroupOpType
    {
        GroupOperationScan,
        GroupOperationClusteredScan,
        GroupOperationReduce,
        GroupOperationClusteredReduce,
        GroupOperationInterleaveReduce
    };

    enum SGVUsage
    {
        POSITION_X,
        POSITION_Y,
        POSITION_Z,
        POSITION_W,
        VFACE,
        PRIMITIVEID,
        GS_INSTANCEID,
        POINT_WIDTH,
        INPUT_COVERAGE_MASK,
        SAMPLEINDEX,
        CLIP_DISTANCE,
        THREAD_ID_X,
        THREAD_ID_Y,
        THREAD_ID_Z,
        THREAD_GROUP_ID_X,
        THREAD_GROUP_ID_Y,
        THREAD_GROUP_ID_Z,
        THREAD_ID_IN_GROUP_X,
        THREAD_ID_IN_GROUP_Y,
        THREAD_ID_IN_GROUP_Z,
        OUTPUT_CONTROL_POINT_ID,
        DOMAIN_POINT_ID_X,
        DOMAIN_POINT_ID_Y,
        DOMAIN_POINT_ID_Z,
        VERTEX_ID,
        REQUESTED_COARSE_SIZE_X,
        REQUESTED_COARSE_SIZE_Y,
        ACTUAL_COARSE_SIZE_X,
        ACTUAL_COARSE_SIZE_Y,
        CLIP_DISTANCE_X,
        CLIP_DISTANCE_Y,
        CLIP_DISTANCE_Z,
        CLIP_DISTANCE_W,
        CLIP_DISTANCE_HI_X,
        CLIP_DISTANCE_HI_Y,
        CLIP_DISTANCE_HI_Z,
        CLIP_DISTANCE_HI_W,
        POSITION_X_OFFSET,
        POSITION_Y_OFFSET,
        RENDER_TARGET_ARRAY_INDEX,
        MSAA_RATE,
        DISPATCH_DIMENSION_X,
        DISPATCH_DIMENSION_Y,
        DISPATCH_DIMENSION_Z,
        INDIRECT_DATA_ADDRESS,
        THREAD_ID_WITHIN_THREAD_GROUP,
        XP0,
        XP1,
        XP2,
        SHADER_TYPE, // For raytracing
        POINT_COORD_X,
        POINT_COORD_Y,
        VIEWPORT_INDEX,
        NUM_SGV,
    };

    enum Float_DenormMode
    {
        FLOAT_DENORM_FLUSH_TO_ZERO = 0,
        FLOAT_DENORM_RETAIN,
    };

    enum ERoundingMode
    {
        ROUND_TO_NEAREST_EVEN,
        ROUND_TO_POSITIVE,
        ROUND_TO_NEGATIVE,
        ROUND_TO_ZERO,

        ROUND_TO_ANY   // dont care
    };

    enum EPreemptionMode
    {
        PREEMPTION_ENABLED,
        PREEMPTION_DISABLED
    };



    enum DISPATCH_SHADER_RAY_INFO_TYPE : unsigned int
    {
        WORLD_RAY_ORG,
        WORLD_RAY_DIR,
        OBJ_RAY_ORG,
        OBJ_RAY_DIR,
        RAY_T_MIN,
        RAY_T_CURRENT,
        PRIMITIVE_INDEX,
        INSTANCE_ID,
        INSTANCE_INDEX,
        RAY_FLAGS,
        OBJECT_TO_WORLD,
        WORLD_TO_OBJECT,
        BARYCENTRICS,
        TRIANGLE_FRONT_FACE,
        CANDIDATE_PROCEDURAL_PRIM_NON_OPAQUE,
        GEOMETRY_INDEX,
        INST_CONTRIBUTION_TO_HITGROUP_INDEX,
        RAY_MASK,
        RAY_INFO_UNKNOWN = 0xFF
    };


    enum class CS_WALK_ORDER : unsigned char {
        WO_XYZ = 0, //012
        WO_XZY = 1, //021
        WO_YXZ = 2, //102
        WO_ZXY = 3, //201 - incorrect enum value, need to swap with next line
        WO_YZX = 4, //120 - incorrect enum value, need to swap with the line above
        WO_ZYX = 5  //210
    };

    enum EMIT_LOCAL_MASK {
        EM_NONE = 0,
        EM_X    = 1,
        EM_XY   = 3,
        EM_XYZ  = 7
    };

    enum ARG_SPACE_RESERVATION_SLOTS {
        RTX_GLOBAL_BUFFER_PTR,

        NUM_ARG_SPACE_RESERVATION_SLOTS
    };
}

#endif //CODE_GEN_PUBLIC_ENUMS_H_
