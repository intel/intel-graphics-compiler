/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IBiF_intel_rt_utils.cl"

inline uint float_to_unorm24(float f)
{
    if (isnan(f)) f = 0.f;
    f = clamp(f, 0.f, 1.f);
    return (uint)(round(f * 0xFFFFFF));
}

inline float unorm24_to_float(uint u)
{
    return (float)(u & 0xFFFFFF) * (1.f / 0xFFFFFF);
}


// === --------------------------------------------------------------------===
// === TraceRayCtrl
// === --------------------------------------------------------------------===
typedef enum
{
    TRACE_RAY_INITIAL    = 0,  // Initializes hit and initializes traversal state
    TRACE_RAY_INSTANCE   = 1,  // Loads committed hit and initializes traversal state
    TRACE_RAY_COMMIT     = 2,  // Loads potential hit and loads traversal state
    TRACE_RAY_CONTINUE   = 3,  // Loads committed hit and loads traversal state
    TRACE_RAY_INITIAL_MB = 4,  // Loads committed hit
    TRACE_RAY_DONE       = -1, // For internal use only
} TraceRayCtrl;


// === --------------------------------------------------------------------===
// === NodeType
// === --------------------------------------------------------------------===
typedef enum
{
    NODE_TYPE_MIXED =
        0x0, // identifies a mixed internal node where each child can have a different type
    NODE_TYPE_INTERNAL   = 0x0, // internal BVH node with 6 children
    NODE_TYPE_INSTANCE   = 0x1, // instance leaf
    NODE_TYPE_PROCEDURAL = 0x3, // procedural leaf
    NODE_TYPE_QUAD       = 0x4, // quad leaf
    NODE_TYPE_QUAD128    = 0x5, // quad leaf (128 bytes)
    NODE_TYPE_INVALID    = 0x7  // indicates invalid node
} NodeType;

// === --------------------------------------------------------------------===
// === SubType definition for each NodeType
// === --------------------------------------------------------------------===
typedef enum
{
    SUB_TYPE_QUAD_MBLUR = 4,          // motion blur quad leaf (128 bytes)
} SubType;

// === --------------------------------------------------------------------===
// === HWAccel
// === --------------------------------------------------------------------===
typedef struct __attribute__((packed))
{
    ulong reserved;
    float bounds[2][3]; // bounding box of the BVH
    uint  reserved0[8];
    uint  numTimeSegments;
    uint  reserved1[13];
    ulong dispatchGlobalsPtr;
} HWAccel;


// === --------------------------------------------------------------------===
// === MemRay
// === --------------------------------------------------------------------===
typedef struct __attribute__((packed, aligned(32)))
{
    // 32 B
    float org[3];
    float dir[3];
    float tnear;
    float tfar;

    // 32 B
    ulong data[4];
    // [0]  0:48 [48] - rootNodePtr  root node to start traversal at
    //     48:64 [16] - rayFlags     see RayFlags structure
    //
    // [1]  0:48 [48] - hitGroupSRBasePtr  base of hit group shader record array (16-bytes alignment)
    //     48:64 [16] - hitGroupSRStride   stride of hit group shader record array (16-bytes alignment)
    //
    // [2]  0:48 [48] - missSRPtr              pointer to miss shader record to invoke on a miss (8-bytes alignment)
    //     48:56 [ 6] - padding                -
    //     56:64 [ 8] - shaderIndexMultiplier  shader index multiplier
    //
    // [3]  0:48 [48] - instLeafPtr  the pointer to instance leaf in case we traverse an instance (64-bytes alignment)
    //     48:56 [ 8] - rayMask      ray mask used for ray masking
    //     56:64 [ 8] - padding      -

    // XE3+:
    // [0]  0:64 [64] - rootNodePtr                root node to start traversal at (64-byte alignment)
    //
    // [1]  0:64 [64] - instLeafPtr                the pointer to instance leaf in case we traverse an instance (64-bytes alignment)
    //
    // [2]  0:16 [16] - rayFlags                   ray flags (see RayFlag structure)
    //     16:24 [ 8] - rayMask                    ray mask used for ray masking
    //     24:31 [ 7] - ComparisonValue            to be compared with Instance.ComparisonMask
    //     31:32 [ 1] - padding                    -
    //     32:64 [32] - hitGroupIndex              hit group shader index
    //
    // [3]  0:16 [16] - missShaderIndex            index of miss shader to invoke on a miss
    //     16:20 [ 4] - shaderIndexMultiplier      shader index multiplier
    //     20:24 [ 4] - padding                    -
    //     24:32 [ 8] - internalRayFlags           Xe3: internal ray flags (see InternalRayFlags enum)
    //     32:64 [32] - time                       ray time in range [0,1]
} MemRay;

// === MemRay getters
inline ulong MemRay_getRootNodePtr(MemRay* memray)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return memray->data[0];
    return __getBits64(memray->data[0],  0, 48);
}

inline ulong MemRay_getRayFlags(MemRay* memray)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return __getBits64(memray->data[2], 0, 16);
    return __getBits64(memray->data[0], 48, 16);
}

inline ulong MemRay_getShaderIndexMultiplier(MemRay* memray)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return __getBits64(memray->data[3], 16, 4);
    return __getBits64(memray->data[2], 56, 8);
}

inline ulong MemRay_getInstLeafPtr(MemRay* memray)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return memray->data[1];
    return __getBits64(memray->data[3], 0, 48);
}

inline ulong MemRay_getRayMask(MemRay* memray)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return __getBits64(memray->data[2], 16, 8);
    return __getBits64(memray->data[3], 48, 8);
}

inline float MemRay_getTime(MemRay* memray)
{
    uint truncVal = (uint)__getBits64(memray->data[3], 32, 32);
    return as_float(truncVal);
}


// === MemRay setters
inline void MemRay_setRootNodePtr(MemRay* memray, ulong val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        memray->data[0] = val;
        return;
    }
    memray->data[0] = __setBits64(memray->data[0], val, 0, 48);
}

inline void MemRay_setRayFlags(MemRay* memray, ulong val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        memray->data[2] = __setBits64(memray->data[2], val, 0, 16);
        return;
    }
    memray->data[0] = __setBits64(memray->data[0], val, 48, 16);
}

inline void MemRay_setShaderIndexMultiplier(MemRay* memray, ulong val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        memray->data[3] = __setBits64(memray->data[3], val, 16, 4);
        return;
    }
    memray->data[2] = __setBits64(memray->data[2], val, 56, 8);
}

inline void MemRay_setInstLeafPtr(MemRay* memray, ulong val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        memray->data[1] = val;
        return;
    }
    memray->data[3] = __setBits64(memray->data[3], val, 0, 48);
}

inline void MemRay_setRayMask(MemRay* memray, ulong val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        memray->data[2] = __setBits64(memray->data[2], val, 16, 8);
        return;
    }
    memray->data[3] = __setBits64(memray->data[3], val, 48, 8);
}

inline void MemRay_setTime(MemRay* memray, float val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        memray->data[3] = __setBits64(memray->data[3], as_uint(val), 32, 32);
        return;
    }
}


// === --------------------------------------------------------------------===
// === MemHit
// === --------------------------------------------------------------------===
typedef struct __attribute__((packed, aligned(32)))
{
    // 12 B
    float t;         // hit distance of current hit (or initial traversal distance)
    uint  dataUV[2]; // barycentric hit coordinates

    // 20 B
    uint  data0;
    ulong data1[2];
    // dataUV[0]  0:32 [32] - barycentric u hit coordinates
    // dataUV[1]  0:32 [32] - barycentric v hit coordinates
    //
    // data0  0:16 [16] - primIndexDelta  prim index delta for compressed meshlets and quads
    //       16:17 [ 1] - valid           set if there is a hit
    //       17:20 [ 3] - leafType        type of node primLeafPtr is pointing to
    //       20:24 [ 4] - primLeafIndex   index of the hit primitive inside the leaf
    //       24:27 [ 3] - bvhLevel        the instancing level at which the hit occured
    //       27:28 [ 1] - frontFace       whether we hit the front-facing side of a triangle (also used to pass opaque flag when calling intersection shaders)
    //       28:29 [ 1] - done            used in sync mode to indicate that traversal is done
    //       29:32 [ 3] - padding         -
    //
    // data1[0]  0:42 [42] - primLeafAddr     address of BVH leaf node (multiple of 64 bytes)
    //          42:64 [16] - hitGroupRecPtr0  LSB of hit group record of the hit triangle (multiple of 16 bytes)
    //
    // data1[1]  0:42 [42] - instLeafAddr     address of BVH instance leaf node (in multiple of 64 bytes)
    //          42:64 [16] - hitGroupRecPtr1  MSB of hit group record of the hit triangle (multiple of 16 bytes)

    // XE3+:
    // dataUV[0]  0:24 [24] - u              barycentric u hit coordinate stored as 24 bit unorm
    //           24:32 [ 8] - hitGroupIndex0 1st bits of hitGroupIndex
    //
    // dataUV[1]  0:24 [24] - v              barycentric u hit coordinate stored as 24 bit unorm
    //           24:32 [ 8] - hitGroupIndex1 2nd bits of hitGroupIndex
    //
    // data0   0:5 [ 5] - primIndexDelta  prim index delta for compressed meshlets and quads
    //        5:12 [ 7] - pad1            MBZ
    //       12:16 [ 4] - leafNodeSubType sub-type of leaf node
    //       16:17 [ 1] - valid           set if there is a hit
    //       17:20 [ 3] - leafType        type of node primLeafPtr is pointing to
    //       20:24 [ 4] - primLeafIndex   index of the hit primitive inside the leaf
    //       24:27 [ 3] - bvhLevel        the instancing level at which the hit occured
    //       27:28 [ 1] - frontFace       whether we hit the front-facing side of a triangle (also used to pass opaque flag when calling intersection shaders)
    //       28:29 [ 1] - done            used in sync mode to indicate that traversal is done
    //       29:30 [ 1] - needSWSTOC      If set, any-hit shader must perform a SW fallback STOC test
    //       30:32 [ 2] - reserved        unused bit
    //
    // data1[0]   0:6 [ 6] - hitGroupIndex2  3rd bits of hitGroupIndex
    //           6:64 [58] - primLeafPtr     pointer to BVH leaf node (MSBs of 64b pointer aligned to 64B)
    // data1[1]   0:6 [ 6] - hitGroupIndex3  4th bits of hit group index
    //           6:64 [58] - instLeafPtr     pointer to BVH instance leaf node (MSBs of 64b pointer aligned to 64B)
} MemHit;


// === MemHit getters
inline uint MemHit_getPrimIndexDelta(MemHit* memhit)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return __getBits32(memhit->data0, 0, 5);
    return __getBits32(memhit->data0, 0, 16);
}

inline long MemHit_getPrimLeafAddr(MemHit* memhit)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return __getSignExtendedBits64(memhit->data1[0], 6, 58);
    return __getSignExtendedBits64(memhit->data1[0], 0, 42);
}

inline long MemHit_getInstLeafAddr(MemHit* memhit)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return __getSignExtendedBits64(memhit->data1[1], 6, 58);
    return __getSignExtendedBits64(memhit->data1[1], 0, 42);
}

inline intel_float2 MemHit_getUV(MemHit* memhit)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return (intel_float2){unorm24_to_float(memhit->dataUV[0]), unorm24_to_float(memhit->dataUV[1])};
    return (intel_float2){((float*)memhit->dataUV)[0], ((float*)memhit->dataUV)[1]};
}

inline uint MemHit_getValid(MemHit* memhit)         { return __getBits32(memhit->data0, 16, 1); }
inline uint MemHit_getLeafType(MemHit* memhit)      { return __getBits32(memhit->data0, 17, 3); }
inline uint MemHit_getPrimLeafIndex(MemHit* memhit) { return __getBits32(memhit->data0, 20, 4); }
inline uint MemHit_getBvhLevel(MemHit* memhit)      { return __getBits32(memhit->data0, 24, 3); }
inline uint MemHit_getFrontFace(MemHit* memhit)     { return __getBits32(memhit->data0, 27, 1); }
inline uint MemHit_getDone(MemHit* memhit)          { return __getBits32(memhit->data0, 28, 1); }
inline uint MemHit_getLeafNodeSubType(MemHit* memhit) { return __getBits32(memhit->data0, 12, 4); }


// === MemHit setters
inline void MemHit_clearUV(MemHit* memhit)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        memhit->dataUV[0] &= 0xFF000000;
        memhit->dataUV[1] &= 0xFF000000;
        return;
    }
    memhit->dataUV[0] = 0;
    memhit->dataUV[1] = 0;
}

inline void MemHit_setUV(MemHit* memhit, float u, float v)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        memhit->dataUV[0] &= 0xFF000000;
        memhit->dataUV[0] |= float_to_unorm24(u);
        memhit->dataUV[1] &= 0xFF000000;
        memhit->dataUV[1] |= float_to_unorm24(v);
        return;
    }
    ((float*)memhit->dataUV)[0] = u;
    ((float*)memhit->dataUV)[1] = v;
}

inline void MemHit_setValid(MemHit* memhit, bool value) { memhit->data0 = __setBits32(memhit->data0, value ? 1 : 0, 16, 1); }
inline void MemHit_setDone(MemHit* memhit, bool value)  { memhit->data0 = __setBits32(memhit->data0, value ? 1 : 0, 28, 1); }


// === MemHit methods
inline global void* MemHit_getPrimLeafPtr(MemHit* memhit)
{
    return to_global((void*)(MemHit_getPrimLeafAddr(memhit) * 64));
}

inline global void* MemHit_getInstanceLeafPtr(MemHit* memhit)
{
    return to_global((void*)(MemHit_getInstLeafAddr(memhit) * 64));
}


// === --------------------------------------------------------------------===
// === RTStack
// === --------------------------------------------------------------------===
typedef struct __attribute__ ((packed, aligned(64)))
{
    enum HitType
    {
        COMMITTED = 0,
        POTENTIAL = 1
    };

    // 64 B
    MemHit hit[2];
    // hit[0] committed hit
    // hit[1] potential hit

    // 128 B
    MemRay ray[2];

    // 64 B
    char   travStack[32 * 2];
} RTStack;

// === RTStack Accessors
inline MemHit* get_query_hit(intel_ray_query_t rayquery, intel_hit_type_t ty)
{
    global RTStack* rtStack = __builtin_IB_intel_query_rt_stack(rayquery);
    return &rtStack->hit[ty];
}

inline MemHit* get_rt_stack_hit(void* rtstack, intel_hit_type_t ty)
{
    RTStack* rtStack = rtstack;
    return &rtStack->hit[ty];
}

inline MemRay* get_rt_stack_ray(void* rtstack, uchar raynum)
{
    RTStack* rtStack = rtstack;
    return &rtStack->ray[raynum];
}

// === --------------------------------------------------------------------===
// === PrimLeafDesc
// === --------------------------------------------------------------------===
typedef struct __attribute__((packed,aligned(8)))
{
#define MAX_GEOM_INDEX ((uint)(0x3FFFFFFF));
#define MAX_SHADER_INDEX ((uint)(0xFFFFFF));

    // For a node type of NODE_TYPE_PROCEDURAL we support enabling
    // and disabling the opaque/non_opaque culling.
    enum Type
    {
        TYPE_NONE = 0,
        TYPE_OPACITY_CULLING_ENABLED  = 0,
        TYPE_OPACITY_CULLING_DISABLED = 1
    };

    uint data[2];
    // data[0]  0:24 [24] - shaderIndex          shader index used for shader record calculations
    //         24:32 [ 8] - geomMask             geometry mask used for ray masking
    //
    // data[1]  0:24 [24] - geomIndex            the geometry index specifies the n'th geometry of the scene
    //         24:28 [ 4] - MBZ
    //         28:29 [ 1] - reserved bit (MBZ)
    //         29:30 [ 1] - DisableOpacityCull   disables opacity culling
    //         30:31 [ 1] - OpaqueGeometry       determines if geometry is opaque
    //         31:32 [ 1] - MBZ
    //
    // XE3+:
    // data[0]  0:24 [24] - shaderIndex          shader index used for shader record calculations
    //         24:32 [ 8] - geomMask             geometry mask used for ray masking
    //
    // data[1]  0:24 [24] - geomIndex            the geometry index specifies the n'th geometry of the scene
    //         24:28 [ 4] - subType              geometry sub-type
    //         28:29 [ 1] - reserved bit (MBZ)
    //         29:30 [ 1] - DisableOpacityCull   disables opacity culling
    //         30:31 [ 1] - OpaqueGeometry       determines if geometry is opaque
    //         31:32 [ 1] - IgnoreRayMultiplier  ignores ray geometry multiplier

} PrimLeafDesc;

// === PrimLeafDesc getters
inline uint PrimLeafDesc_getShaderIndex(PrimLeafDesc* leaf) { return __getBits32(leaf->data[0],  0, 24); }
inline uint PrimLeafDesc_getGeomMask(PrimLeafDesc* leaf)    { return __getBits32(leaf->data[0], 24,  8); }
inline uint PrimLeafDesc_getGeomIndex(PrimLeafDesc* leaf)   { return __getBits32(leaf->data[1],  0, 24); }

// === PrimLeafDesc setters
inline uint PrimLeafDesc_setShaderIndex(PrimLeafDesc* leaf, uint val) { leaf->data[0] = __setBits32(leaf->data[0], val,  0, 24); }
inline uint PrimLeafDesc_setGeomMask(PrimLeafDesc* leaf, uint val)    { leaf->data[0] = __setBits32(leaf->data[0], val, 24,  8); }
inline uint PrimLeafDesc_setGeomIndex(PrimLeafDesc* leaf, uint val)   { leaf->data[1] = __setBits32(leaf->data[1], val,  0, 24); }


// === --------------------------------------------------------------------===
// === QuadLeaf
// === --------------------------------------------------------------------===
typedef struct __attribute__((packed,aligned(64)))
{
    PrimLeafDesc leafDesc;
    unsigned int primIndex0;

    uint data;
    //  0:16 [16] - primIndex1Delta  delta encoded primitive index of second triangle
    // 16:18 [ 2] - j0               specifies first vertex of second triangle
    // 18:20 [ 2] - j1               specified second vertex of second triangle
    // 20:22 [ 2] - j2               specified third vertex of second triangle
    // 22:23 [ 1] - last             true if the second triangle is the last triangle in a leaf list
    // 23:32 [ 9] - padding          -

    // XE3+:
    // data   0:5 [ 5] - primIndex1Delta    offset of primID of second triangle
    //        5:6 [ 1] - stoc1_tri0_swstoc  indicates that software STOC emulation for triangle 0 is required in AHS
    //        6:7 [ 1] - stoc1_tri1_swstoc  indicates that software STOC emulation for triangle 1 is required in AHS
    //        7:8 [ 1] - pad                reserved (MBZ)
    //       8:12 [ 4] - stoc1_tri0_opaque  STOC level 1 sub-triangle opaque      bits for triangle 0
    //      12:16 [ 4] - stoc1_tri0_transp  STOC level 1 sub-triangle transparent bits for triangle 0
    //      16:18 [ 2] - j0
    //      18:20 [ 2] - j1
    //      20:22 [ 2] - j2
    //      22:23 [ 1] - last               last quad in BVH leaf
    //      23:24 [ 1] - lastInMeshlet      last quad in meshlet
    //      24:28 [ 4] - stoc1_tri1_opaque  STOC level 1 sub-triangle opaque      bit for triangle 1
    //      28:32 [ 4] - stoc1_tri1_transp  STOC level 1 sub-triangle transparent bit for triangle 1

    float v[4][3];
} QuadLeaf;

// === QuadLeaf getters
inline uint QuadLeaf_getPrimIndex1Delta(QuadLeaf* leaf)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return __getBits32(leaf->data, 0, 5);
    return __getBits32(leaf->data, 0, 16);
}

inline uint QuadLeaf_getJ0(QuadLeaf* leaf)   { return __getBits32(leaf->data, 16,  2); }
inline uint QuadLeaf_getJ1(QuadLeaf* leaf)   { return __getBits32(leaf->data, 18,  2); }
inline uint QuadLeaf_getJ2(QuadLeaf* leaf)   { return __getBits32(leaf->data, 20,  2); }
inline uint QuadLeaf_getLast(QuadLeaf* leaf) { return __getBits32(leaf->data, 22,  1); }

// === QuadLeaf setters
inline void QuadLeaf_setPrimIndex1Delta(QuadLeaf* leaf, uint val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        leaf->data = __setBits32(leaf->data, val, 0, 5);
        return;
    }
    leaf->data = __setBits32(leaf->data, val, 0, 16);
}

inline void QuadLeaf_setJ0(QuadLeaf* leaf, uint val)   { leaf->data = __setBits32(leaf->data, val, 16,  2); }
inline void QuadLeaf_setJ1(QuadLeaf* leaf, uint val)   { leaf->data = __setBits32(leaf->data, val, 18,  2); }
inline void QuadLeaf_setJ2(QuadLeaf* leaf, uint val)   { leaf->data = __setBits32(leaf->data, val, 20,  2); }
inline void QuadLeaf_setLast(QuadLeaf* leaf, uint val) { leaf->data = __setBits32(leaf->data, val, 22,  1); }

// === --------------------------------------------------------------------===
// === QuadLeaf_MBlur
// === --------------------------------------------------------------------===
typedef struct __attribute__((packed, aligned(64)))
{
    PrimLeafDesc leafDesc;
    unsigned int primIndex0;

    uint data;
    //  0:5  [5] - primIndex1Delta    offset of primID of second triangle
    //  5:6  [1] - stoc1_tri0_swstoc  indicates that software STOC emulation for triangle 0 is required in AHS
    //  6:7  [1] - stoc1_tri0_swstoc  indicates that software STOC emulation for triangle 0 is required in AHS
    //  7:8  [1] - padding            -
    //  8:12 [4] - stoc1_tri0_opaque  STOC level 1 sub-triangle opaque      bits for triangle 0
    // 12:16 [4] - stoc1_tri0_transp  STOC level 1 sub-triangle transparent bits for triangle 0
    // 16:18 [2] - j0
    // 18:20 [2] - j1
    // 20:22 [2] - j2
    // 22:23 [1] - last               last quad in BVH leaf
    // 23:24 [1] - lastInMeshlet      last quad in meshlet
    // 24:28 [4] - stoc1_tri0_opaque  STOC level 1 sub-triangle opaque      bit for triangle 1
    // 28:32 [4] - stoc1_tri0_transp  STOC level 1 sub-triangle transparent bit for triangle 1

    float v0[3];
    float v0_diff[3];
    float v1[3];
    float v1_diff[3];

    int pad0;
    int pad1;

    float start_time;
    float end_time;

    float v2[3];
    float v2_diff[3];
    float v3[3];
    float v3_diff[3];
} QuadLeaf_MBlur;

inline uint QuadLeaf_MBlur_getJ0(QuadLeaf_MBlur* leaf)
{
    return __getBits32(leaf->data, 16, 2);
}
inline uint QuadLeaf_MBlur_getJ1(QuadLeaf_MBlur* leaf)
{
    return __getBits32(leaf->data, 18, 2);
}
inline uint QuadLeaf_MBlur_getJ2(QuadLeaf_MBlur* leaf)
{
    return __getBits32(leaf->data, 20, 2);
}

inline uint QuadLeaf_MBlur_setJ0(QuadLeaf_MBlur* leaf, uint val)
{
    leaf->data = __setBits32(leaf->data, val, 16, 2);
}
inline uint QuadLeaf_MBlur_setJ1(QuadLeaf_MBlur* leaf, uint val)
{
    leaf->data = __setBits32(leaf->data, val, 18, 2);
}
inline uint QuadLeaf_MBlur_setJ2(QuadLeaf_MBlur* leaf, uint val)
{
    leaf->data = __setBits32(leaf->data, val, 20, 2);
}

float3 QuadLeaf_MBlur_getVertex(QuadLeaf_MBlur* leaf, unsigned int j)
{
    if (j == 0)
        return (float3){leaf->v0[0], leaf->v0[1], leaf->v0[2]};
    else if (j == 1)
        return (float3){leaf->v1[0], leaf->v1[1], leaf->v1[2]};
    else if (j == 2)
        return (float3){leaf->v2[0], leaf->v2[1], leaf->v2[2]};
    else // j == 3
        return (float3){leaf->v3[0], leaf->v3[1], leaf->v3[2]};
}

float3 QuadLeaf_MBlur_getVertexDiff(QuadLeaf_MBlur* leaf, unsigned int j)
{
    if (j == 0)
        return (float3){leaf->v0_diff[0], leaf->v0_diff[1], leaf->v0_diff[2]};
    else if (j == 1)
        return (float3){leaf->v1_diff[0], leaf->v1_diff[1], leaf->v1_diff[2]};
    else if (j == 2)
        return (float3){leaf->v2_diff[0], leaf->v2_diff[1], leaf->v2_diff[2]};
    else // j == 3
        return (float3){leaf->v3_diff[0], leaf->v3_diff[1], leaf->v3_diff[2]};
}


// === --------------------------------------------------------------------===
// === ProceduralLeaf
// === --------------------------------------------------------------------===
typedef struct __attribute__((packed,aligned(64)))
{
#define PROCEDURAL_N 13

    PrimLeafDesc leafDesc; // leaf header identifying the geometry

    uint data;
    //    0:4    [     4] - numPrimitives  number of stored primitives
    //    4:32-N [32-N-4] - padding        -
    // 32-N:32   [     N] - last           bit vector with a last bit per primitive

    uint _primIndex[PROCEDURAL_N]; // primitive indices of all primitives stored inside the leaf
} ProceduralLeaf;

// === ProceduralLeaf accessors

inline uint ProceduralLeaf_getNumPrimitives(ProceduralLeaf* leaf) { return __getBits32(leaf->data,                 0,             4); }
inline uint ProceduralLeaf_getLast(ProceduralLeaf* leaf)          { return __getBits32(leaf->data, 32 - PROCEDURAL_N,  PROCEDURAL_N); }

inline uint ProceduralLeaf_setNumPrimitives(ProceduralLeaf* leaf, uint val) { leaf->data = __setBits32(leaf->data, val,                 0,             4); }
inline uint ProceduralLeaf_setLast(ProceduralLeaf* leaf, uint val)          { leaf->data = __setBits32(leaf->data, val, 32 - PROCEDURAL_N,  PROCEDURAL_N); }


// === --------------------------------------------------------------------===
// === InstanceLeaf
// === --------------------------------------------------------------------===
typedef struct
{
    /* first 64 bytes accessed during traversal by hardware */
    struct Part0
    {
        uint  data0[2];
        ulong data1;
        // data0[0]  0:24 [24] - shaderIndex  shader index used to calculate instancing shader in case of software instancing
        //          24:32 [ 8] - geomMask     geometry mask used for ray masking
        //
        // data0[1]  0:24 [24] - instanceContribution  instance contribution to hit group index
        //          24:29 [ 5] - padding
        //          29:30 [ 1] - DisableOpacityCull    disables opacity culling
        //          30:31 [ 1] - OpaqueGeometry        determines if geometry is opaque
        //          31:32 [ 1] - padding
        //
        // data1     0:48 [48] - startNodePtr     start node where to continue traversal of the instanced object
        //          48:56 [ 8] - instFlags        flags for the instance (see InstanceFlags)

        // XE3+:
        // data0[0]  0:24 [24] - instanceContribution  instance contribution to hit group index
        //          24:32 [ 8] - geomMask              geometry mask used for ray masking
        //
        // data0[1]   0:8 [ 8] - instFlags             lags for the instance (see InstanceFlags)
        //            8:9 [ 1] - ComparisonMode        0 for <=, 1 for > comparison
        //           9:16 [ 7] - ComparisonValue       to be compared with ray.ComparionMask
        //          16:24 [ 8] - pad0                  reserved (MBZ)
        //          24:28 [ 4] - subType               geometry sub-type
        //          28:29 [ 1] - pad1                  reserved (MBZ)
        //          29:30 [ 1] - DisableOpacityCull    disables opacity culling
        //          30:31 [ 1] - OpaqueGeometry        determines if geometry is opaque
        //          31:32 [ 1] - IgnoreRayMultiplier   ignores ray geometry multiplier
        //
        // data1     0:64 [64] - startNodePtr          64 bit start node of instanced object

        float world2obj_vx[3]; // 1st column of Worl2Obj transform
        float world2obj_vy[3]; // 2nd column of Worl2Obj transform
        float world2obj_vz[3]; // 3rd column of Worl2Obj transform
        float obj2world_p[3];  // translation of Obj2World transform (on purpose in first 64 bytes)
    } part0;

    /* second 64 bytes accessed during shading */
    struct Part1
    {
        ulong data;
        // data  0:48 [48] - bvhPtr  pointer to BVH where start node belongs to
        // data 48:64 [16] - pad     -
        // XE3+:
        // data  0:64 [64] - bvhPtr  pointer to BVH where start node belongs to

        uint instanceID;    // user defined value per DXR spec
        uint instanceIndex; // geometry index of the instance (n'th geometry in scene)

        float obj2world_vx[3]; // 1st column of Obj2World transform
        float obj2world_vy[3]; // 2nd column of Obj2World transform
        float obj2world_vz[3]; // 3rd column of Obj2World transform
        float world2obj_p[3];  // translation of World2Obj transform
    } part1;
} InstanceLeaf;

// === InstanceLeaf getters
inline ulong InstanceLeaf_getStartNodePtr(InstanceLeaf* leaf)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return leaf->part0.data1;
    return __getBits64(leaf->part0.data1, 0, 48);
}

inline ulong InstanceLeaf_getInstFlags(InstanceLeaf* leaf)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return __getBits32(leaf->part0.data0[1], 0, 8);
    return __getBits64(leaf->part0.data1, 48, 8);
}

inline ulong InstanceLeaf_getBvhPtr(InstanceLeaf* leaf)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE) return leaf->part1.data;
    return __getBits64(leaf->part1.data, 0, 48);
}

inline uint InstanceLeaf_getGeomMask(InstanceLeaf* leaf)           { return __getBits32(leaf->part0.data0[0], 24,  8); }
inline uint InstanceLeaf_getDisableOpacityCull(InstanceLeaf* leaf) { return __getBits32(leaf->part0.data0[1], 29,  1); }


// === InstanceLeaf setters
inline void InstanceLeaf_setStartNodePtr(InstanceLeaf* leaf, ulong val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        leaf->part0.data1 = val;
        return;
    }
    leaf->part0.data1 = __setBits64(leaf->part0.data1, val, 0, 48);
}

inline void InstanceLeaf_setInstFlags(InstanceLeaf* leaf, ulong val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        leaf->part0.data0[1] = __setBits64(leaf->part0.data0[1], val, 0, 8);
        return;
    }
    leaf->part0.data1 = __setBits64(leaf->part0.data1, val, 48, 8);
}

inline void InstanceLeaf_setBvhPtr(InstanceLeaf* leaf, ulong val)
{
    if (BIF_FLAG_CTRL_GET(RenderFamily) >= IGFX_XE3_CORE)
    {
        leaf->part1.data = val;
        return;
    }
    leaf->part1.data = __setBits64(leaf->part1.data, val, 0, 48);
}

inline void InstanceLeaf_setGeomMask(InstanceLeaf* leaf, uint val)           { leaf->part0.data0[0] = __setBits32(leaf->part0.data0[0], val, 24,  8); }
inline void InstanceLeaf_setDisableOpacityCull(InstanceLeaf* leaf, uint val) { leaf->part0.data0[1] = __setBits32(leaf->part0.data0[1], val, 29,  1); }
