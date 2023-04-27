/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IBiF_intel_rt_utils.cl"


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
    NODE_TYPE_INVALID    = 0x7  // indicates invalid node
} NodeType;


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

} MemRay;

// === MemRay getters
inline ulong MemRay_getRootNodePtr(MemRay* memray)
{
    return __getBits64(memray->data[0],  0, 48);
}

inline ulong MemRay_getRayFlags(MemRay* memray)
{
    return __getBits64(memray->data[0], 48, 16);
}

inline ulong MemRay_getShaderIndexMultiplier(MemRay* memray)
{
    return __getBits64(memray->data[2], 56, 8);
}

inline ulong MemRay_getInstLeafPtr(MemRay* memray)
{
    return __getBits64(memray->data[3], 0, 48);
}

inline ulong MemRay_getRayMask(MemRay* memray)
{
    return __getBits64(memray->data[3], 48, 8);
}



// === MemRay setters
inline void MemRay_setRootNodePtr(MemRay* memray, ulong val)
{
    memray->data[0] = __setBits64(memray->data[0], val, 0, 48);
}

inline void MemRay_setRayFlags(MemRay* memray, ulong val)
{
    memray->data[0] = __setBits64(memray->data[0], val, 48, 16);
}

inline void MemRay_setShaderIndexMultiplier(MemRay* memray, ulong val)
{
    memray->data[2] = __setBits64(memray->data[2], val, 56, 8);
}

inline void MemRay_setInstLeafPtr(MemRay* memray, ulong val)
{
    memray->data[3] = __setBits64(memray->data[3], val, 0, 48);
}

inline void MemRay_setRayMask(MemRay* memray, ulong val)
{
    memray->data[3] = __setBits64(memray->data[3], val, 48, 8);
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

} MemHit;


// === MemHit getters
inline uint MemHit_getPrimIndexDelta(MemHit* memhit)
{
    return __getBits32(memhit->data0, 0, 16);
}

inline ulong MemHit_getPrimLeafAddr(MemHit* memhit)
{
    return __getBits64(memhit->data1[0], 0, 42);
}

inline ulong MemHit_getInstLeafAddr(MemHit* memhit)
{
    return __getBits64(memhit->data1[1], 0, 42);
}

inline intel_float2 MemHit_getUV(MemHit* memhit)
{
    return (intel_float2){((float*)memhit->dataUV)[0], ((float*)memhit->dataUV)[1]};
}

inline uint MemHit_getValid(MemHit* memhit)         { return __getBits32(memhit->data0, 16, 1); }
inline uint MemHit_getLeafType(MemHit* memhit)      { return __getBits32(memhit->data0, 17, 3); }
inline uint MemHit_getPrimLeafIndex(MemHit* memhit) { return __getBits32(memhit->data0, 20, 4); }
inline uint MemHit_getBvhLevel(MemHit* memhit)      { return __getBits32(memhit->data0, 24, 3); }
inline uint MemHit_getFrontFace(MemHit* memhit)     { return __getBits32(memhit->data0, 27, 1); }
inline uint MemHit_getDone(MemHit* memhit)          { return __getBits32(memhit->data0, 28, 1); }


// === MemHit setters
inline void MemHit_clearUV(MemHit* memhit)
{
    memhit->dataUV[0] = 0;
    memhit->dataUV[1] = 0;
}

inline void MemHit_setUV(MemHit* memhit, float u, float v)
{
    ((float*)memhit->dataUV)[0] = u;
    ((float*)memhit->dataUV)[1] = v;
}

inline void MemHit_setValid(MemHit* memhit, bool value) { memhit->data0 = __setBits32(memhit->data0, value ? 1 : 0, 16, 1); }
inline void MemHit_setDone(MemHit* memhit, bool value)  { memhit->data0 = __setBits32(memhit->data0, value ? 1 : 0, 28, 1); }


// === MemHit methods
inline global void* MemHit_getPrimLeafPtr(MemHit* memhit)
{
    return to_global((void*)((ulong)MemHit_getPrimLeafAddr(memhit) * 64));
}

inline global void* MemHit_getInstanceLeafPtr(MemHit* memhit)
{
    return to_global((void*)((ulong)MemHit_getInstLeafAddr(memhit) * 64));
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

    float v[4][3];
} QuadLeaf;

// === QuadLeaf getters
inline uint QuadLeaf_getPrimIndex1Delta(QuadLeaf* leaf)
{
    return __getBits32(leaf->data, 0, 16);
}

inline uint QuadLeaf_getJ0(QuadLeaf* leaf)   { return __getBits32(leaf->data, 16,  2); }
inline uint QuadLeaf_getJ1(QuadLeaf* leaf)   { return __getBits32(leaf->data, 18,  2); }
inline uint QuadLeaf_getJ2(QuadLeaf* leaf)   { return __getBits32(leaf->data, 20,  2); }
inline uint QuadLeaf_getLast(QuadLeaf* leaf) { return __getBits32(leaf->data, 22,  1); }

// === QuadLeaf setters
inline void QuadLeaf_setPrimIndex1Delta(QuadLeaf* leaf, uint val)
{
    leaf->data = __setBits32(leaf->data, val, 0, 16);
}

inline void QuadLeaf_setJ0(QuadLeaf* leaf, uint val)   { leaf->data = __setBits32(leaf->data, val, 16,  2); }
inline void QuadLeaf_setJ1(QuadLeaf* leaf, uint val)   { leaf->data = __setBits32(leaf->data, val, 18,  2); }
inline void QuadLeaf_setJ2(QuadLeaf* leaf, uint val)   { leaf->data = __setBits32(leaf->data, val, 20,  2); }
inline void QuadLeaf_setLast(QuadLeaf* leaf, uint val) { leaf->data = __setBits32(leaf->data, val, 22,  1); }


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
    return __getBits64(leaf->part0.data1, 0, 48);
}

inline ulong InstanceLeaf_getInstFlags(InstanceLeaf* leaf)
{
    return __getBits64(leaf->part0.data1, 48, 8);
}

inline ulong InstanceLeaf_getBvhPtr(InstanceLeaf* leaf)
{
    return __getBits64(leaf->part1.data, 0, 48);
}

inline uint InstanceLeaf_getGeomMask(InstanceLeaf* leaf)           { return __getBits32(leaf->part0.data0[0], 24,  8); }
inline uint InstanceLeaf_getDisableOpacityCull(InstanceLeaf* leaf) { return __getBits32(leaf->part0.data0[1], 29,  1); }


// === InstanceLeaf setters
inline void InstanceLeaf_setStartNodePtr(InstanceLeaf* leaf, ulong val)
{
    leaf->part0.data1 = __setBits64(leaf->part0.data1, val, 0, 48);
}

inline void InstanceLeaf_setInstFlags(InstanceLeaf* leaf, ulong val)
{
    leaf->part0.data1 = __setBits64(leaf->part0.data1, val, 48, 8);
}

inline void InstanceLeaf_setBvhPtr(InstanceLeaf* leaf, ulong val)
{
    leaf->part1.data = __setBits64(leaf->part1.data, val, 0, 48);
}

inline void InstanceLeaf_setGeomMask(InstanceLeaf* leaf, uint val)           { leaf->part0.data0[0] = __setBits32(leaf->part0.data0[0], val, 24,  8); }
inline void InstanceLeaf_setDisableOpacityCull(InstanceLeaf* leaf, uint val) { leaf->part0.data0[1] = __setBits32(leaf->part0.data0[1], val, 29,  1); }
