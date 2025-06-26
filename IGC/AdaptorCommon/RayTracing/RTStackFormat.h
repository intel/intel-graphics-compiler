/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This file contains a definition of the structure of the ray tracing
/// stack as seen by a ray.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "API/RayDispatchGlobalData.h"
#include "ConstantsEnums.h"

#include <stdint.h>
#include <stddef.h>


// This code defines a set of macros, that allows us to intentionally disable certain warnings
// in a way that is both concise and portable across compilers.
// We need to disable the warnings in such way because this C++ file may be compiled with Clang, GCC, and Microsoft Visual Studio Compiler
// This works around (at least) two known problems:
//  - QuickBuild treats compiler warnings as errors
//  - RTStackReflection.exe generates warnings when compiling this file
//
// The only macros that are used by the main code are:
//     DISABLE_WARNING_PUSH
//     DISABLE_WARNING_POP
//     and the macros for the individual warnings
// The others are lower level internal utilities, used only by higher level macros.
//
// Explanation of code in original resource : https://www.fluentcpp.com/2019/08/30/how-to-disable-a-warning-in-cpp/

// clang-format off
#if defined(__clang__)
    #define DO_PRAGMA_DIAG(X) _Pragma(#X)                                                     // internal utility
    #define DISABLE_WARNING_PUSH         DO_PRAGMA_DIAG(GCC diagnostic push)
    #define DISABLE_WARNING_POP          DO_PRAGMA_DIAG(GCC diagnostic pop)
    #define DISABLE_WARNING(warningName) DO_PRAGMA_DIAG(GCC diagnostic ignored #warningName)  // internal utility
    // Add warnings that you want to disable here:
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION   \
        DISABLE_WARNING(-Wgnu-anonymous-struct)  // anonymous structs are a GNU extension
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION \
        DISABLE_WARNING(-Wnested-anon-types)     // anonymous types declared in an anonymous union are an extension
#elif defined(__GNUC__)
    #define DO_PRAGMA_DIAG(X) _Pragma(#X)                                                     // internal utility
    #define DISABLE_WARNING_PUSH         DO_PRAGMA_DIAG(GCC diagnostic push)
    #define DISABLE_WARNING_POP          DO_PRAGMA_DIAG(GCC diagnostic pop)
    // Treats only this header file as a system header, AKA disables all warnings in this header file only.
    // The effect does not extend into any file that includes this header file.
    #pragma GCC system_header
    // define the macros that are used in the code down below to prevent compiler failure
    // NOTE: Apparently GCC compiler doesn't support disabling individual warnings.
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION
#elif defined(_MSC_VER)
    #define DISABLE_WARNING_PUSH           __pragma(warning( push ))
    #define DISABLE_WARNING_POP            __pragma(warning( pop ))
    #define DISABLE_WARNING(warningNumber) __pragma(warning( disable : warningNumber ))  // internal utility
    // Add warnings that you want to disable here:
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION   \
        DISABLE_WARNING(4201)  // nonstandard extension used: nameless struct/union
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT \
        DISABLE_WARNING(4820)  // 'MemTravStack::<unnamed-tag>': '4' bytes padding added after data member 'MemTravStack::<unnamed-tag>::offset'
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION
#else
    // define the macros that are used in the code down below to prevent compiler failure
    // NOTE: internal utility macros should not be defined here
    #define DISABLE_WARNING_PUSH
    #define DISABLE_WARNING_POP
    // Add warnings that you want to disable here:
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION
#endif
// clang-format on

// save the current pragma state, save the current compiler settings select the
// warnings that we want to disable, for this file only
DISABLE_WARNING_PUSH
DISABLE_WARNING_ANONYMOUS_STRUCT_UNION
DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION


namespace IGC {

// will be patched with the global root signature at compile time
struct alignas(8) TypeHoleGlobalRootSig
{
    char __Padding[16];
};

// This is currently all of the cross-thread constant data that will be populated
// in the indirect data by the UMD (See D3D12RaytracingDispatch.h for more info).
struct RTGlobalsAndRootSig
{
    RayDispatchGlobalData RTGlobals;
    TypeHoleGlobalRootSig GlobalRootSig;
};

} // namespace IGC

namespace RTStackFormat {

uint32_t getRTStackHeaderSize(uint32_t MaxBVHLevels);

constexpr static uint32_t MAX_BVH_LEVELS = 2;
constexpr static uint32_t MEM_STACK_SIZE = 4;

// DXR uses two BVH levels: a top-level acceleration structure and a
// bottom-level acceleration structure.
constexpr static uint32_t TOP_LEVEL_BVH    = 0;
constexpr static uint32_t BOTTOM_LEVEL_BVH = 1;

// Auxiliary types to make update of the structs easier.
using uint3  = uint32_t[3];
using uint4  = uint32_t[4];
using Vec3f  = float[3];
using float4 = float[4];

static_assert(sizeof(uint3) == 12, "size mismatch");
static_assert(sizeof(uint4) == 16, "size mismatch");
static_assert(sizeof(Vec3f) == 12, "size mismatch");
static_assert(sizeof(float4) == 16, "size mismatch");

// This is taken from the functional model.  It's not clear that we have an
// immediate use to read these fields but it is useful to have here for
// documentation purposes.
struct KSP
{
    /* returns the base address of the shader record this KSP is part of */
    void* getShaderRecord() {
        return (char*)this + 8 * _offset - 32; // ShaderIdentifier is 32 bytes large
    }

    /* checks if this KSP is a NULL KSP */
    bool isNull() const {
        return *(uint64_t*)this == NullValue;
    }

    static constexpr uint64_t NullValue = 0;

public:
    uint64_t _offset               : 3;   // offset in 8-byte blocks to the start of shader parameters
    uint64_t _push_constant_enable : 1;   // If this field is enabled,
                                          // 8DWs are pushed from the Local
                                          // Arguments at 32B offset from the
                                          // start of the shader record address.

    uint64_t _dispatch_mode        : 1;   // 0 = SIMD16, 1 = SIMD8
    uint64_t _reserved0            : 1;
    uint64_t _shader               : 26;  // shader function pointer
    uint64_t _reserved1            : 32;  // unused padding bytes
};

static_assert(sizeof(KSP) == 8, "changed?");



struct NodeInfo
{
    enum class Bits : uint8_t
    {
        type     = 3,
        parent   = 1,

        cur_prim = 4,
    };

    using T = uint32_t;

    /* NodeType */
    uint8_t  type     : (T)Bits::type;
    uint8_t  parent   : (T)Bits::parent;   // Indicates a culled stack entry where a single parent node
                                           // is stored in place of multiple child nodes
    uint8_t  cur_prim : (T)Bits::cur_prim;
};

/**
 * MemTravStack originally used to have two different definitions in two different files.
 * Here these two definitions are merged into one for preserving backwards compatibility.
 *
 * Basically these two definitions have different internal layouts, different fields, but they have the same size.
 * So we can put these two definitions inside an anonymous union inside the main structure definition.
 * The anonymous union allows both these definitions to be accessed at once.
 * And the anonymous structs force all the fields in a particular definition to be non-overlapping.
 * Since the NoneInfo nodeInfo[MEM_STACK_SIZE] cannot be defined in both these anonymous structs, it doesn't matter which one we put that field in.
 *
 * Any one of these fields can be used, as long as one is consistent with which "overlapping definition" is being used.
 * Or both can be used at once for type punning, to set the value of the full variable, and then get a particular sub-section of that value as a bit field.
 * We maybe having code which accesses the fields in the first definition, and also code somewhere else in the project which accesses the fields in the second definition.
 */
struct StackEntry
{
    enum class Bits : uint8_t
    {
        offset    = 31,
        lastChild = 1,
    };

    using T = uint32_t;

    uint32_t offset    : (T)Bits::offset;    // in multiples of 64B. max 2^29 prims i.e. max 2^30 nodes. one extra bit
    uint32_t lastChild : (T)Bits::lastChild;
};

struct MemTravStack
{
    union {
        ///  The original definition from RTStackFormat.h
        struct {
            uint32_t indexArray0;   // Restart information for the stack. Described in the background document.
            uint32_t indexArray1;
            uint32_t indexArray2;
            StackEntry stackEntry[MEM_STACK_SIZE];
            NodeInfo nodeInfo[MEM_STACK_SIZE];
        };

        ///  The original definition from ocl_raytracing_structures.h
        struct {
            uint64_t curDepth      : 5;        // current depth in the restart trail
            uint64_t restartTrail0 : 29;       // lower bits of restart trail
            uint64_t restartTrail1 : 29;       // higher bits of restart trail
            uint64_t lastChild0    : 1;        // last child bit for node 0
            uint32_t restartTrail2 : 29;       // highest bits of restart trail
            uint32_t lastChild123  : 3;        // last child bit for nodes 1/2/3

            int32_t offset[MEM_STACK_SIZE];    // Signed offset relative to BVH root in multiples of 64B.
            NodeInfo pad[MEM_STACK_SIZE];      // padding to make sure two structs of this union are the same size
        };
    };
};

// Define structs Xe, used to specialize other template structs, like MemHit or RTStack, via GenT arg
#define STYLE(X) struct X;
#include "RayTracingMemoryStyle.h"
#undef STYLE

template <typename GenT> struct MemHit;
template <typename GenT> struct MemRay;
template <typename GenT> struct PrimLeafDesc;
template <typename GenT> struct QuadLeaf;
template <typename GenT> struct InstanceLeaf;

template <>
struct MemHit<Xe>
{
    float    t;                    // hit distance of current hit (or initial traversal distance)
    float    u, v;                 // barycentric hit coordinates

    enum class Bits : uint8_t
    {
        primIndexDelta  = 16,
        valid           = 1,
        leafType        = 3,
        primLeafIndex   = 4,
        bvhLevel        = 3,
        frontFace       = 1,
        done            = 1,
        pad0            = 3,

        primLeafPtr     = 42,
        hitGroupRecPtr0 = 22,

        instLeafPtr     = 42,
        hitGroupRecPtr1 = 22,
    };

    // This is the offset within the bitfield
    enum class Offset : uint8_t
    {
        // Add as needed
        done = 28,
    };

    using T = uint32_t;

    uint32_t primIndexDelta : (T)Bits::primIndexDelta; // prim index delta for compressed meshlets and quads
    uint32_t valid          : (T)Bits::valid;          // set if there is a hit
    uint32_t leafType       : (T)Bits::leafType;       // type of node primLeafPtr is pointing to
    uint32_t primLeafIndex  : (T)Bits::primLeafIndex;  // index of the hit primitive inside the leaf
    uint32_t bvhLevel       : (T)Bits::bvhLevel;       // the instancing level at which the hit occured
    uint32_t frontFace      : (T)Bits::frontFace;      // whether we hit the front-facing side of a triangle (also used to pass opaque flag when calling intersection shaders)
    uint32_t done           : (T)Bits::done;           // used in sync mode to indicate that traversal is done (HW will only set this to 0)
    uint32_t pad0           : (T)Bits::pad0;           // unused bits (explicit padding)

    uint64_t primLeafPtr     : (T)Bits::primLeafPtr;     // pointer to BVH leaf node (multiple of 64 bytes)
    uint64_t hitGroupRecPtr0 : (T)Bits::hitGroupRecPtr0; // LSB of hit group record of the hit triangle (multiple of 16 bytes)

    uint64_t instLeafPtr     : (T)Bits::instLeafPtr;     // pointer to BVH instance leaf node (in multiple of 64 bytes)
    uint64_t hitGroupRecPtr1 : (T)Bits::hitGroupRecPtr1; // MSB of hit group record of the hit triangle (multiple of 16 bytes)

    static_assert((uint32_t)Bits::primLeafPtr == (uint32_t)Bits::instLeafPtr,
        "Size changed?");
};
static_assert(sizeof(MemHit<Xe>) == 32, "MemHit has to be 32 bytes large");

template <>
struct MemRay<Xe>
{
    // 32 B
    Vec3f org;         // the origin of the ray
    Vec3f dir;         // the direction of the ray
    float tnear;       // the start of the ray
    float tfar;        // the end of the ray

    using T = uint32_t;
    enum class Bits : uint8_t
    {
        rootNodePtr           = 48,
        rayFlags              = 16,

        hitGroupSRBasePtr     = 48,
        hitGroupSRStride      = 16,

        missSRPtr             = 48,
        pad                   = 1,
        shaderIndexMultiplier = 8,

        instLeafPtr           = 48,
        rayMask               = 8,

        ComparisonValue       = 7,
        pad2 = 8,
    };

    // 32 B

    // RayFlags:
    //
    // There are 3 sets of flags, from the application point of view,
    // available during the ray traversal:
    // 1. RayFlags set in the shader before the TraceRay functions is called.
    // 2. Flags applied to the entire Pipeline.
    // 3. Flags applied per instances and geometry in the BVH
    //
    // All the Ray data is stored in the MemRayStructures which have two instances:
    // 1. TopLevel (Ray0)
    // 2. BottomLevel (Ray1)
    //
    // TopLevel is written by SW when the Ray is cast (before calling TraceRay).
    // When the traversal reaches the instance and enters related BottomLevel BVH,
    // TopLevel MemRay is copied by HW. Flags from entered instance are applied to
    // the RayFlags.
    // The BottomLevel MemRay is written only by HW, when AnyHit or Intersection
    // Shaders are called.
    //
    // As HW can only apply flags from TracRay (1) and the BVH (3), flags from
    // the Pipeline (2) has to applied by SW. It is done by applying them to
    // the flags set before the TraceRay (1). But the application can read
    // both the Pipeline flags (2) and the RayFlags (1) independently.
    // So the original RayFlags (1) are additionally stored in the MemRay memory.
    // As there is no dedicated space for them, the instLeafPtr from TopLevel MemRay
    // is used, as it is irrelevant to the traversal at the TopLevel.
    //
    // When the AnyHit or Intersection shader are called, there are 3 sets of flags
    // available:
    // 1. RayFlags combined with Pipeline Flags - Ray0.rayFlags
    // 2. RayFlags - Ray0.instLeafPtr aka Ray0.flagsFromTraceRay.rayFlags
    // 3. RayFlags + Pipeline Flags + Instance/GeometryFlags - Ray1.rayFlags
    //
    // rayFlags defined here is used to access:
    // 1. Ray0.rayFlags
    // 2. Ray1.rayFlags
    //
    // Additionally an alias for 16-bit access is added.
    // As we must avoid structure members alignment, to properly generate
    // offsets in MemRayStructure, rayFlags has 64bit type. To simplify the
    // access, a 16bit alias is created (rayFlags16BitAccessAlias.rayFlags).
    // It points to same part of MemHit structure as just rayFlags.
    union
    {
        struct
        {
            uint16_t uw0;
            uint16_t uw1;
            uint16_t uw2;
            uint16_t rayFlags;
        } rayFlags16BitTypeAlias;

        struct
        {
            uint64_t rootNodePtr : (T)Bits::rootNodePtr; // root node to start traversal at
            uint64_t rayFlags : (T)Bits::rayFlags;    // ray flags (see RayFlags structure)
        };
    };

    uint64_t hitGroupSRBasePtr : (T)Bits::hitGroupSRBasePtr; // base of hit group shader record array (16-bytes alignment)
    uint64_t hitGroupSRStride  : (T)Bits::hitGroupSRStride;  // stride of hit group shader record array (16-bytes alignment)

    uint64_t missSRPtr             : (T)Bits::missSRPtr;             // pointer to miss shader record to invoke on a miss (8-bytes alignment)
    // DG2
    //uint64_t pad                 : 8;                              // explicit padding bits
    uint64_t pad                   : (T)Bits::pad;                   // explicit padding bits
    uint64_t ComparisonValue       : (T)Bits::ComparisonValue;       // to be compared with Instance.ComparisonValue
    uint64_t shaderIndexMultiplier : (T)Bits::shaderIndexMultiplier; // shader index multiplier

    union
    {
        // This just an alias for 16-bit type.
        // As we must avoid structure members alignment, to properly generate
        // offsets in MemRayStructure, rayFlags has 64bit type. To simplify the
        // access, a 16bit alias is created (Ray0.flagsFromTraceRay16BitTypeAlias.rayFlags).
        // It points to same part of MemHit structure as just Ray0.flagsFromTraceRay.rayFlags.
        struct
        {
            uint16_t rayFlags;
            uint16_t uw1;
            uint16_t uw2;
            uint16_t uw3;
        } flagsFromTraceRay16BitTypeAlias;

        struct
        {
            // This is just an alias for instLeafPtr used to access
            // RayFlags written by the application in the shader, before the TraceRay.
            // This structure is used to access Ray0.flagsFromTraceRay.rayFlags
            // from the above RayFlags description.
            uint64_t rayFlags : (T)Bits::instLeafPtr;
            uint64_t pad          : 16;
        } flagsFromTraceRay;

        struct {
            uint64_t instLeafPtr : (T)Bits::instLeafPtr;  // the pointer to instance leaf in case we traverse an instance (64-bytes alignment)
            uint64_t rayMask     : (T)Bits::rayMask;      // ray mask used for ray masking

            uint64_t pad2 : (T)Bits::pad2;
        };
    };
};
static_assert(sizeof(MemRay<Xe>) == 64, "MemRay has to be 64 bytes large");

template <>
struct PrimLeafDesc<Xe>
{
    enum Type : uint32_t
    {
        TYPE_NONE = 0,

        /* For a node type of NODE_TYPE_MESHLET, the referenced leaf may
         * still be a QuadLeaf or a Meshlet. We need this as we produce
         * two quads instead of one meshlet when meshlet compression does
         * not work well. */

         TYPE_QUAD    = 0,
         TYPE_MESHLET = 1,

         /* For a node type of NODE_TYPE_PROCEDURAL we support enabling
          * and disabling the opaque/non_opaque culling. */

          TYPE_OPACITY_CULLING_ENABLED  = 0,
          TYPE_OPACITY_CULLING_DISABLED = 1
    };

    enum class Bits : uint8_t
    {
        shaderIndex = 24,
        geomMask    = 8,
        geomIndex   = 29,
        type        = 1,
        geomFlags   = 2,
    };

    using T = uint32_t;

    uint32_t shaderIndex : (T)Bits::shaderIndex; // shader index used for shader record calculations
    uint32_t geomMask    : (T)Bits::geomMask;    // geometry mask used for ray masking

    uint32_t geomIndex : (T)Bits::geomIndex; // the geometry index specifies the n'th geometry of the scene
    uint32_t type      : (T)Bits::type;      // distinguish between QuadLeaves and Meshlets or enable/disable culling for procedurals and instances
    uint32_t geomFlags : (T)Bits::geomFlags; // geometry flags of this geometry
};
static_assert(sizeof(PrimLeafDesc<Xe>) == 8, "PrimLeafDesc must be 8 bytes large");

template <>
struct QuadLeaf<Xe>
{
    PrimLeafDesc<Xe> leafDesc;  // the leaf header

    uint32_t primIndex0;    // primitive index of first triangle (has to be at same offset as for CompressedMeshlet!)

    enum class Bits : uint8_t
    {
        primIndex1Delta = 16,
        j0              = 2,
        j1              = 2,
        j2              = 2,
        last            = 1,
        pad             = 9,
    };

    using T = uint32_t;

    uint32_t primIndex1Delta : (T)Bits::primIndex1Delta;  // delta encoded primitive index of second triangle
    uint32_t j0              : (T)Bits::j0;               // specifies first vertex of second triangle
    uint32_t j1              : (T)Bits::j1;               // specified second vertex of second triangle
    uint32_t j2              : (T)Bits::j2;               // specified third vertex of second triangle
    uint32_t last            : (T)Bits::last;             // true if the second triangle is the last triangle in a leaf list
    uint32_t pad             : (T)Bits::pad;              // unused bits

    Vec3f v0;  // first vertex of first triangle
    Vec3f v1;  // second vertex of first triangle
    Vec3f v2;  // third vertex of first triangle
    Vec3f v3;  // additional vertex only used for second triangle
};
static_assert(sizeof(QuadLeaf<Xe>) == 64, "QuadLeaf must be 64 bytes large");

template <>
struct InstanceLeaf<Xe>
{
    /* first 64 bytes accessed during traversal by hardware */
    struct Part0
    {
        using T = uint32_t;
        enum class Bits : uint8_t
        {
            shaderIndex           = 24,
            geomMask              = 8,
            instContToHitGrpIndex = 24,
            pad0                  = 5,
            type                  = 1,
            geomFlags             = 2,
            startNodePtr          = 48,
            instFlags             = 8,
            ComparisonMode        = 1,
            ComparisonValue       = 7,
        };

        uint32_t shaderIndex : (T)Bits::shaderIndex;  // shader index used to calculate instancing shader in case of software instancing
        uint32_t geomMask    : (T)Bits::geomMask;     // geometry mask used for ray masking

        uint32_t instanceContributionToHitGroupIndex : (T)Bits::instContToHitGrpIndex;  // TODO: add description
        uint32_t pad0                                : (T)Bits::pad0;                   // explicit padding bits
        /* PrimLeafDesc */
        uint32_t type                                : (T)Bits::type;                   // enables/disables opaque culling
        uint32_t geomFlags                           : (T)Bits::geomFlags;              // geometry flags are not used for instances

        uint64_t startNodePtr : (T)Bits::startNodePtr;  // start node where to continue traversal of the instanced object
        uint64_t instFlags    : (T)Bits::instFlags;     // flags for the instance (see InstanceFlags)

        // Xe2+
        uint64_t ComparisonMode  : (T)Bits::ComparisonMode;  // 0 for less than or equal, 1 for greater
        uint64_t ComparisonValue : (T)Bits::ComparisonValue; // to be compared with ray.ComparisonValue


        // Note that the hardware swaps the translation components of the
        // world2obj and obj2world matrices, and uses column-major instead of row-major.

        // DXR and Vulkan specify transform matrices in row-major order.
        // A 3x4 row-major matrix from the API maps to HWInstanceLeaf layout as shown:
        //     | vx[0] vy[0] vz[0] p[0] |
        // M = | vx[1] vy[1] vz[1] p[1] |
        //     | vx[2] vy[2] vz[2] p[2] |

        Vec3f world2obj_vx;   // 1st col of Worl2Obj transform
        Vec3f world2obj_vy;   // 2nd col of Worl2Obj transform
        Vec3f world2obj_vz;   // 3rd col of Worl2Obj transform
        Vec3f obj2world_p;    // translation of Obj2World transform (on purpose in first 64 bytes)
    } part0;

    /* second 64 bytes accessed during shading */
    struct Part1
    {
        uint64_t bvhPtr : 48;   // pointer to BVH where start node belongs too
        uint64_t pad    : 16;   // unused bits (explicit padding)

        uint32_t instanceID;    // user defined value per DXR spec
        uint32_t instanceIndex; // geometry index of the instance (n'th geometry in scene)

        Vec3f obj2world_vx;     // 1st col of Obj2World transform
        Vec3f obj2world_vy;     // 2nd col of Obj2World transform
        Vec3f obj2world_vz;     // 3rd col of Obj2World transform
        Vec3f world2obj_p;      // translation of World2Obj transform
    } part1;
};

template <>
struct MemHit<Xe3>
{
    enum class Bits : uint8_t
    {
        u = 24,
        v = 24,
        hitGroupIndex0 = 8,
        hitGroupIndex1 = 8,
        hitGroupIndex2 = 6,
        hitGroupIndex3 = 6,
        primIndexDelta = 5,
        pad1 = 7,
        leafNodeSubType = 4,
        valid = 1,
        leafType = 3,
        primLeafIndex = 4,
        bvhLevel = 3,
        frontFace = 1,
        done = 1,
        needSWSTOC = 1,
        reserved = 2,

        primLeafPtr = 58,
        instLeafPtr = 58,
    };

    enum class Offset : uint8_t
    {
        done = 28,
    };

    using T = uint32_t;

    float    t;                                        // hit distance of current hit (or initial traversal distance)

    uint32_t u              : (T)Bits::u;              // barycentric u hit coordinate stored as 24 bit unorm
    uint32_t hitGroupIndex0 : (T)Bits::hitGroupIndex0; // 1st bits of hitGroupIndex

    uint32_t v              : (T)Bits::v;              // barycentric v hit coordinate stored as 24 bit unorm
    uint32_t hitGroupIndex1 : (T)Bits::hitGroupIndex1; // 2nd bits of hitGroupIndex

    uint32_t primIndexDelta  : (T)Bits::primIndexDelta;  // prim index delta for compressed meshlets and quads
    uint32_t pad1            : (T)Bits::pad1;            // MBZ
    uint32_t leafNodeSubType : (T)Bits::leafNodeSubType; // sub-type of leaf node
    uint32_t valid           : (T)Bits::valid;           // set if there is a hit
    uint32_t leafType        : (T)Bits::leafType;        // type of node primLeafPtr is pointing to
    uint32_t primLeafIndex   : (T)Bits::primLeafIndex;   // index of the hit primitive inside the leaf
    uint32_t bvhLevel        : (T)Bits::bvhLevel;        // the instancing level at which the hit occured
    uint32_t frontFace       : (T)Bits::frontFace;       // whether we hit the front-facing side of a triangle (also used to pass opaque flag when calling intersection shaders)
    uint32_t done            : (T)Bits::done;            // used in sync mode to indicate that traversal is done
    uint32_t needSWSTOC      : (T)Bits::needSWSTOC;      // If set, any-hit shader must perform a SW fallback STOC test
    uint32_t reserved        : (T)Bits::reserved;        // unused bit

    uint64_t hitGroupIndex2 : (T)Bits::hitGroupIndex2; // 3rd bits of hitGroupIndex
    uint64_t primLeafPtr    : (T)Bits::primLeafPtr;    // pointer to BVH leaf node (MSBs of 64b pointer aligned to 64B)

    uint64_t hitGroupIndex3 : (T)Bits::hitGroupIndex3; // 4th bits of hit group index
    uint64_t instLeafPtr    : (T)Bits::instLeafPtr;    // pointer to BVH instance leaf node (MSBs of 64b pointer aligned to 64B)
};
static_assert(sizeof(MemHit<Xe3>) == 32, "MemHit has to be 32 bytes large");

template <>
struct MemRay<Xe3>
{
    // 32 B
    Vec3f org;         // the origin of the ray
    Vec3f dir;         // the direction of the ray
    float tnear;       // the start of the ray
    float tfar;        // the end of the ray

    using T = uint32_t;
    enum class Bits : uint8_t
    {
        rootNodePtr           = 64,
        instLeafPtr           = 64,
        rayFlags              = 16,
        rayMask               = 8,
        ComparisonValue       = 7,
        pad1                  = 1,
        missShaderIndex       = 16,
        shaderIndexMultiplier = 4,
        pad2                  = 4,
        internalRayFlags      = 8,
    };

    uint64_t rootNodePtr : (T)Bits::rootNodePtr;  // root node to start traversal at (64-byte alignment)
    union
    {
        // This just an alias for 16-bit type.
        // As we must avoid structure members alignment, to properly generate
        // offsets in MemRayStructure, rayFlags has 64bit type. To simplify the
        // access, a 16bit alias is created (Ray0.flagsFromTraceRay16BitTypeAlias.rayFlags).
        // It points to same part of MemHit structure as just Ray0.flagsFromTraceRay.rayFlags.
        struct
        {
            uint16_t rayFlags;
            uint16_t uw1;
            uint16_t uw2;
            uint16_t uw3;
        } flagsFromTraceRay16BitTypeAlias;

        struct
        {
           // This is just an alias for instLeafPtr used to access
           // RayFlags written by the application in the shader, before the TraceRay.
           // This structure is used to access Ray0.flagsFromTraceRay.rayFlags
           // from the below RayFlags description.
            uint64_t rayFlags;
        } flagsFromTraceRay;

        uint64_t instLeafPtr : (T)Bits::instLeafPtr;  // the pointer to instance leaf in case we traverse an instance (64-bytes alignment)
    };

    // RayFlags:
    //
    // There are 3 sets of flags, from the application point of view,
    // available during the ray traversal:
    // 1. RayFlags set in the shader before the TraceRay functions is called.
    // 2. Flags applied to the entire Pipeline.
    // 3. Flags applied per instances and geometry in the BVH
    //
    // All the Ray data is stored in the MemRayStructures which have two instances:
    // 1. TopLevel (Ray0)
    // 2. BottomLevel (Ray1)
    //
    // TopLevel is written by SW when the Ray is cast (before calling TraceRay).
    // When the traversal reaches the instance and enters related BottomLevel BVH,
    // TopLevel MemRay is copied by HW. Flags from entered instance are applied to
    // the RayFlags.
    // The BottomLevel MemRay is written only by HW, when AnyHit or Intersection
    // Shaders are called.
    //
    // As HW can only apply flags from TracRay (1) and the BVH (3), flags from
    // the Pipeline (2) has to applied by SW. It is done by applying them to
    // the flags set before the TraceRay (1). But the application can read
    // both the Pipeline flags (2) and the RayFlags (1) independently.
    // So the original RayFlags (1) are additionally stored in the MemRay memory.
    // As there is no dedicated space for them, the instLeafPtr from TopLevel MemRay
    // is used, as it is irrelevant to the traversal at the TopLevel.
    //
    // When the AnyHit or Intersection shader are called, there are 3 sets of flags
    // available:
    // 1. RayFlags combined with Pipeline Flags - Ray0.rayFlags
    // 2. RayFlags - Ray0.instLeafPtr aka Ray0.flagsFromTraceRay.rayFlags
    // 3. RayFlags + Pipeline Flags + Instance/GeometryFlags - Ray1.rayFlags
    //
    // rayFlags defined here is used to access:
    // 1. Ray0.rayFlags
    // 2. Ray1.rayFlags
    //
    // Additionally an alias for 16-bit access is added.
    // As we must avoid structure members alignment, to properly generate
    // offsets in MemRayStructure, rayFlags has 64bit type. To simplify the
    // access, a 16bit alias is created (rayFlags16BitAccessAlias.rayFlags).
    // It points to same part of MemHit structure as just rayFlags.
    union
    {
        struct
        {
            uint16_t rayFlags;
            uint16_t uw1;
        } rayFlags16BitTypeAlias;

        struct
        {
            uint32_t rayFlags : (T)Bits::rayFlags;        // ray flags (see RayFlag structure)
            uint32_t rayMask : (T)Bits::rayMask;         // ray mask used for ray masking
            uint32_t ComparisonValue : (T)Bits::ComparisonValue; // to be compared with Instance.ComparisonMask
            uint32_t pad1 : (T)Bits::pad1;
        };
    };

    uint32_t hitGroupIndex;                              // hit group shader index

    uint32_t missShaderIndex       : (T)Bits::missShaderIndex;       // index of miss shader to invoke on a miss
    uint32_t shaderIndexMultiplier : (T)Bits::shaderIndexMultiplier; // shader index multiplier
    uint32_t pad2                  : (T)Bits::pad2;
    uint32_t internalRayFlags      : (T)Bits::internalRayFlags;      // Xe3: internal ray flags (see InternalRayFlags enum)

    float time;            // ray time in range [0,1] (force to 0 for now)
};
static_assert(sizeof(MemRay<Xe3>) == 64, "MemRay has to be 64 bytes large");

enum class InternalRayFlags : uint16_t
{
    NONE                            = 0x0,
    TRIANGLE_FRONT_COUNTERCLOCKWISE = 0x1, // Xe3: switch front and back-facing triangles
    LEVEL_ASCEND_DISABLED           = 0x2, // Xe3: disables the automatic level ascend for this level
    SKIP_MISS_SHADER                = 0x4, // Xe3: skip execution of miss shader
    DISABLE_STOC                    = 0x8, // Xe3: disables sub triangle opacity culling
};

template <>
struct PrimLeafDesc<Xe3>
{
    enum Type : uint32_t
    {
        TYPE_NONE = 0,

        /* For a node type of NODE_TYPE_MESHLET, the referenced leaf may
         * still be a QuadLeaf or a Meshlet. We need this as we produce
         * two quads instead of one meshlet when meshlet compression does
         * not work well. */

         TYPE_QUAD    = 0,
         TYPE_MESHLET = 1,

         /* For a node type of NODE_TYPE_PROCEDURAL we support enabling
          * and disabling the opaque/non_opaque culling. */

          TYPE_OPACITY_CULLING_ENABLED  = 0,
          TYPE_OPACITY_CULLING_DISABLED = 1
    };

    enum class Bits : uint8_t
    {
        shaderIndex         = 24,
        geomMask            = 8,

        geomIndex           = 24,
        subType             = 4,
        reserved0           = 1,
        DisableOpacityCull  = 1,
        OpaqueGeometry      = 1,
        IgnoreRayMultiplier = 1,
    };

    using T = uint32_t;

    uint32_t shaderIndex         : (T)Bits::shaderIndex;         // shader index used for shader record calculations
    uint32_t geomMask            : (T)Bits::geomMask;            // geometry mask used for ray masking

    uint32_t geomIndex           : (T)Bits::geomIndex;           // Xe1+: the geometry index specifies the n'th geometry of the scene
    uint32_t subType             : (T)Bits::subType;             // Xe3: geometry sub-type
    uint32_t reserved0           : (T)Bits::reserved0;           // Xe1+: reserved bit (MBZ)
    uint32_t DisableOpacityCull  : (T)Bits::DisableOpacityCull;  // Xe1+: disables opacity culling
    uint32_t OpaqueGeometry      : (T)Bits::OpaqueGeometry;      // Xe1+: determines if geometry is opaque
    uint32_t IgnoreRayMultiplier : (T)Bits::IgnoreRayMultiplier; // Xe3: ignores ray geometry multiplier
};
static_assert(sizeof(PrimLeafDesc<Xe3>) == 8, "PrimLeafDesc must be 8 bytes large");

template <>
struct QuadLeaf<Xe3>
{
    PrimLeafDesc<Xe3> leafDesc;  // the leaf header

    uint32_t primIndex0;         // primitive index of first triangle (has to be at same offset as for CompressedMeshlet!)

    enum class Bits : uint8_t
    {
        primIndex1Delta   = 5,
        stoc1_tri0_swstoc = 1,
        stoc1_tri1_swstoc = 1,
        pad               = 1,
        stoc1_tri0_opaque = 4,
        stoc1_tri0_transp = 4,
        j0                = 2,
        j1                = 2,
        j2                = 2,
        last              = 1,
        lastInMeshlet     = 1,
        stoc1_tri1_opaque = 4,
        stoc1_tri1_transp = 4,
    };

    using T = uint32_t;

    uint32_t primIndex1Delta   : (T)Bits::primIndex1Delta;   // offset of primID of second triangle
    uint32_t stoc1_tri0_swstoc : (T)Bits::stoc1_tri0_swstoc; // indicates that software STOC emulation for triangle 0 is required in AHS
    uint32_t stoc1_tri1_swstoc : (T)Bits::stoc1_tri1_swstoc; // indicates that software STOC emulation for triangle 1 is required in AHS
    uint32_t pad               : (T)Bits::pad;               // reserved (MBZ)
    uint32_t stoc1_tri0_opaque : (T)Bits::stoc1_tri0_opaque; // STOC level 1 sub-triangle opaque      bits for triangle 0
    uint32_t stoc1_tri0_transp : (T)Bits::stoc1_tri0_transp; // STOC level 1 sub-triangle transparent bits for triangle 0
    uint32_t j0                : (T)Bits::j0;
    uint32_t j1                : (T)Bits::j1;
    uint32_t j2                : (T)Bits::j2;
    uint32_t last              : (T)Bits::last;              // last quad in BVH leaf
    uint32_t lastInMeshlet     : (T)Bits::lastInMeshlet;     // last quad in meshlet
    uint32_t stoc1_tri1_opaque : (T)Bits::stoc1_tri1_opaque; // STOC level 1 sub-triangle opaque      bit for triangle 1
    uint32_t stoc1_tri1_transp : (T)Bits::stoc1_tri1_transp; // STOC level 1 sub-triangle transparent bit for triangle 1

    Vec3f v0;  // first vertex of first triangle
    Vec3f v1;  // second vertex of first triangle
    Vec3f v2;  // third vertex of first triangle
    Vec3f v3;  // additional vertex only used for second triangle
};
static_assert(sizeof(QuadLeaf<Xe3>) == 64, "QuadLeaf must be 64 bytes large");

struct QuadLeaf_STOC3
{
    QuadLeaf<Xe3> quad;
    uint64_t fullyOpaqueBits0;       // fully opaque sub-triangle bits for triangle 0
    uint64_t fullyTransparentBits0;  // fully transparent sub-triangle bits for triangle 0
    uint64_t fullyOpaqueBits1;       // fully opaque sub-triangle bits for triangle 1
    uint64_t fullyTransparentBits1;  // fully transparent sub-triangle bits for triangle 1

    uint64_t stoc3_tri0_addr;      // Xe3: address of full STOC bits for triangle 0
    uint8_t  stoc3_tri0_level;     // Xe3: the recursion level for STOC bits for triangle 0 (4-12)

    uint8_t  stoc3_tri0_swstoc : 1;// Xe3: indicates that software STOC emulation for triangle 0 is required in AHS
    uint8_t  _reserved0        : 7;

    uint8_t  reserved0[6];

    uint64_t stoc3_tri1_addr;      // Xe3: address of full STOC bits for triangle 1
    uint8_t  stoc3_tri1_level;     // Xe3: the recursion level for STOC bits for triangle 1 (4-12)

    uint8_t  stoc3_tri1_swstoc : 1;// Xe3: indicates that software STOC emulation for triangle 1 is required in AHS
    uint8_t  _reserved1        : 7;

    uint8_t  reserved1[6];
};
static_assert(sizeof(QuadLeaf_STOC3) == 128, "QuadLeaf_STOC3 must be 128 bytes large");

template <>
struct InstanceLeaf<Xe3>
{
    /* first 64 bytes accessed during traversal by hardware */
    struct Part0
    {
        using T = uint32_t;
        enum class Bits : uint8_t
        {
            instContToHitGrpIndex = 24,
            geomMask              = 8,
            instFlags             = 8,
            ComparisonMode        = 1,
            ComparisonValue       = 7,
            pad0                  = 8,
            subType               = 3,
            pad1                  = 2,
            DisableOpacityCull    = 1,
            OpaqueGeometry        = 1,
            IgnoreRayMultiplier   = 1,
        };

        uint32_t instanceContributionToHitGroupIndex : (T)Bits::instContToHitGrpIndex; // Xe3: instance contribution to hit group index
        uint32_t geomMask                            : (T)Bits::geomMask;             // Xe1+: geometry mask used for ray masking

        uint32_t instFlags            : (T)Bits::instFlags;           // Xe3: flags for the instance (see InstanceFlags)
        uint32_t ComparisonMode       : (T)Bits::ComparisonMode;      // Xe3: 0 for <=, 1 for > comparison
        uint32_t ComparisonValue      : (T)Bits::ComparisonValue;     // Xe3: to be compared with ray.ComparionMask
        uint32_t pad0                 : (T)Bits::pad0;                // reserved (MBZ)
        uint32_t subType              : (T)Bits::subType;             // Xe3: geometry sub-type
        uint32_t pad1                 : (T)Bits::pad1;                // reserved (MBZ)
        uint32_t DisableOpacityCull   : (T)Bits::DisableOpacityCull;  // Xe1+: disables opacity culling
        uint32_t OpaqueGeometry       : (T)Bits::OpaqueGeometry;      // Xe1+: determines if geometry is opaque
        uint32_t IgnoreRayMultiplier  : (T)Bits::IgnoreRayMultiplier; // Xe3: ignores ray geometry multiplier

        uint64_t startNodePtr;                                        // Xe3: 64 bit start node of instanced object


        // Note that the hardware swaps the translation components of the
        // world2obj and obj2world matrices, and uses column-major instead of row-major.

        // DXR and Vulkan specify transform matrices in row-major order.
        // A 3x4 row-major matrix from the API maps to HWInstanceLeaf layout as shown:
        //     | vx[0] vy[0] vz[0] p[0] |
        // M = | vx[1] vy[1] vz[1] p[1] |
        //     | vx[2] vy[2] vz[2] p[2] |

        Vec3f world2obj_vx;   // 1st col of Worl2Obj transform
        Vec3f world2obj_vy;   // 2nd col of Worl2Obj transform
        Vec3f world2obj_vz;   // 3rd col of Worl2Obj transform
        Vec3f obj2world_p;    // translation of Obj2World transform (on purpose in first 64 bytes)
    } part0;

    /* second 64 bytes accessed during shading */
    struct Part1
    {
        uint64_t bvhPtr : 48;   // pointer to BVH where start node belongs too
        uint64_t pad    : 16;   // unused bits (explicit padding)

        uint32_t instanceID;    // user defined value per DXR spec
        uint32_t instanceIndex; // geometry index of the instance (n'th geometry in scene)

        Vec3f obj2world_vx;     // 1st col of Obj2World transform
        Vec3f obj2world_vy;     // 2nd col of Obj2World transform
        Vec3f obj2world_vz;     // 3rd col of Obj2World transform
        Vec3f world2obj_p;      // translation of World2Obj transform
    } part1;
};
static_assert(sizeof(InstanceLeaf<Xe3>) == 128, "InstanceLeaf must be 128 bytes large");


// HW stack
template <typename GenT, uint32_t MaxBVHLevels>
struct RTStack
{
    MemHit<GenT> committedHit;    // stores committed hit
    MemHit<GenT> potentialHit;    // stores potential hit that is passed to any hit shader

    MemRay<GenT> ray0;
    MemRay<GenT> ray1;
    MemRay<GenT> ray[MaxBVHLevels - 2];       // stores a ray for each instancing level
    MemTravStack travStack[MaxBVHLevels]; // spill location for the internal stack state per instancing level

    static_assert(MaxBVHLevels > 2);
};

template <typename GenT>
struct RTStack<GenT, MAX_BVH_LEVELS>
{
    MemHit<GenT> committedHit;    // stores committed hit
    MemHit<GenT> potentialHit;    // stores potential hit that is passed to any hit shader

    MemRay<GenT> ray0;
    MemRay<GenT> ray1;
    MemTravStack travStack[MAX_BVH_LEVELS]; // spill location for the internal stack state per instancing level
};


// This is the ShadowMemory that we maintain if we DONOT need spill/fill it to/from HW RTStack.
// We keep this data structure as small as possible to reduce the size.
template <typename GenT, uint32_t MaxBVHLevels>
struct SMStack
{
    MemHit<GenT> committedHit;    // stores committed hit
    MemHit<GenT> potentialHit;    // stores potential hit that is passed to any hit shader

    MemRay<GenT> ray0;
    MemRay<GenT> ray1;
    MemRay<GenT> ray[MaxBVHLevels - 2];       // stores a ray for each instancing level
    //MemTravStack travStack[MaxBVHLevels]; // spill location for the internal stack state per instancing level

    static_assert(MaxBVHLevels > 2);
};

template <typename GenT>
struct SMStack<GenT, MAX_BVH_LEVELS>
{
    MemHit<GenT> committedHit;    // stores committed hit
    MemHit<GenT> potentialHit;    // stores potential hit that is passed to any hit shader

    MemRay<GenT> ray0;
    MemRay<GenT> ray1;
    //MemTravStack travStack[MaxBVHLevels]; // spill location for the internal stack state per instancing level
};

// For now, we're just defaulting to using an RTStack assuming
// MAX_BVH_LEVELS == 2.
template <typename GenT>
using RTStack2 = RTStack<GenT, MAX_BVH_LEVELS>;
template <typename GenT>
using SMStack2 = SMStack<GenT, MAX_BVH_LEVELS>;

template <typename RTStack2T>
constexpr size_t sizeofRTStack2() { return sizeof(RTStack2T); }

#ifdef HAS_INCLUDE_TYPE_TRAITS
static_assert(std::is_standard_layout_v<RTStack2<Xe>>);
static_assert(std::is_standard_layout_v<SMStack2<Xe>>);
#endif // HAS_INCLUDE_TYPE_TRAITS

// Makes sure that important fields are at their proper offset from the start
// of the structure. Update this if the structure changes.  This is just for
// documentation purposes.
static_assert(offsetof(RTStack2<Xe>, committedHit) == 0,   "unexpected offset!");
static_assert(offsetof(RTStack2<Xe>, potentialHit) == 32,  "unexpected offset!");
static_assert(offsetof(RTStack2<Xe>, ray0)         == 64,  "unexpected offset!");
static_assert(offsetof(RTStack2<Xe>, ray1)         == 64 + 64, "unexpected offset!");
static_assert(offsetof(RTStack2<Xe>, travStack)    == 192, "unexpected offset!");


/////////////// BVH structures ///////////////

enum NodeType : uint8_t
{
    NODE_TYPE_MIXED = 0x0,        // identifies a mixed internal node where each child can have a different type
    NODE_TYPE_INTERNAL = 0x0,     // internal BVH node with 6 children
    NODE_TYPE_INSTANCE = 0x1,     // instance leaf
    NODE_TYPE_PROCEDURAL = 0x3,   // procedural leaf
    NODE_TYPE_QUAD = 0x4,         // quad leaf
    NODE_TYPE_MESHLET = 0x6,      // meshlet leaf
    NODE_TYPE_INVALID = 0x7       // indicates invalid node
};

struct InternalNode
{
    static constexpr const uint32_t NUM_CHILDREN = 6;

    Vec3f lower;          // world space origin of quantization grid
    int32_t childOffset;  // offset to all children in 64B multiples

    NodeType nodeType;    // the type of the node
    uint8_t pad;          // unused byte

    int8_t exp_x;         // 2^exp_x is the size of the grid in x dimension
    int8_t exp_y;         // 2^exp_y is the size of the grid in y dimension
    int8_t exp_z;         // 2^exp_z is the size of the grid in z dimension
    uint8_t nodeMask;     // mask used for ray filtering

    struct ChildData
    {
        uint8_t blockIncr : 2;  // size of child in 64 byte blocks
        uint8_t startPrim : 4;  // start primitive in fat leaf mode or child type in mixed mode
        uint8_t pad       : 2;  // unused bits
    } childData[NUM_CHILDREN];

    uint8_t lower_x[NUM_CHILDREN];  // the quantized lower bounds in x-dimension
    uint8_t upper_x[NUM_CHILDREN];  // the quantized upper bounds in x-dimension
    uint8_t lower_y[NUM_CHILDREN];  // the quantized lower bounds in y-dimension
    uint8_t upper_y[NUM_CHILDREN];  // the quantized upper bounds in y-dimension
    uint8_t lower_z[NUM_CHILDREN];  // the quantized lower bounds in z-dimension
    uint8_t upper_z[NUM_CHILDREN];  // the quantized upper bounds in z-dimension
};

enum class GeometryFlags : uint32_t
{
    NONE       = 0x0,
    RTX_OPAQUE = 0x1
};

template <typename GenT>
struct ProceduralLeaf
{
    static const uint32_t N = 13;

    enum class Bits : uint8_t
    {
        numPrimitives = 4,
        pad           = 32 - numPrimitives - N,
        last          = N,
    };

    using T = uint32_t;

    PrimLeafDesc<GenT> leafDesc;             // leaf header identifying the geometry
    uint32_t numPrimitives : (T)Bits::numPrimitives;  // number of stored primitives
    uint32_t pad           : (T)Bits::pad;            // explicit padding bits
    uint32_t last          : (T)Bits::last;           // bit vector with a last bit per primitive
    uint32_t _primIndex[N];                           // primitive indices of all primitives stored inside the leaf
};

static_assert(sizeof(ProceduralLeaf<Xe>) == 64, "ProceduralLeaf must be 64 bytes large");

static_assert(sizeof(QuadLeaf<Xe>) == sizeof(ProceduralLeaf<Xe>),
    "Leaves must be same size");
static_assert(sizeof(QuadLeaf<Xe3>) == sizeof(ProceduralLeaf<Xe3>),
    "Leaves must be same size");
static_assert(sizeof(QuadLeaf<Xe>) == sizeof(QuadLeaf<Xe3>));

constexpr uint32_t LeafSize = sizeof(QuadLeaf<Xe>);

/*
  The CompressedMeshlet structure stores triangles compressed
  losslessly. Between 1 to 16 triangles can get stored that index
  into a vertex array with up to 16 vertices.

  The structure contains a header, a list of delta compressed
  indices (allocated front to back), and a list of compressed
  vertices (allocated back to front).

 */
template <typename GenT>
struct CompressedMeshlet
{
    PrimLeafDesc<GenT> leafDesc;
    uint32_t first_triangle_primID;      // has to be at same offset as for QuadLeaf!

    uint32_t num_triangles         : 4;  // number of stored triangles (0 -> 1 triangle, 1 -> 2 triangles, etc..)
    uint32_t num_position_bits_x   : 4;  // number of bit pairs for x coordinate (0 -> 2 bits, 1 -> 4 bits, etc.)
    uint32_t num_position_bits_y   : 4;  // number of bit pairs for y coordinate (0 -> 2 bits, 1 -> 4 bits, etc.)
    uint32_t num_position_bits_z   : 4;  // number of bit pairs for z coordinate (0 -> 2 bits, 1 -> 4 bits, etc.)
    uint32_t num_primID_delta_bits : 4;  // number of bits for primIndex delta encoding (0 -> 1 bit, 1 -> 2 bits, etc.)
    uint32_t first_triangle_islast : 1;  // last bit for first triangle
    uint32_t pad                   : 8;  // explicit padding bits

    /* in this array triangles are stored front to back and vertex
     * positions back to front. */
    char data[100];

    Vec3f first_position;
};

static_assert(sizeof(CompressedMeshlet<Xe>) == 128,
    "CompressedMeshlet has to be 128 bytes large");

enum class InstanceFlags : uint8_t
{
    NONE = 0x0,
    TRIANGLE_CULL_DISABLE = 0x1,
    TRIANGLE_FRONT_COUNTERCLOCKWISE = 0x2,
    FORCE_OPAQUE = 0x4,
    FORCE_NON_OPAQUE = 0x8,
    FORCE_2STATE_STOC = 0x10,
    DISABLE_STOC = 0x20,
};

template <typename GenT>
struct alignas(256) BVH
{
    uint64_t rootNodeOffset;        // root node offset

    Vec3f   bounds_min;             // bounds of the BVH
    Vec3f   bounds_max;

    uint32_t nodeDataStart;         // first 64 byte block of node data
    uint32_t nodeDataCur;           // next free 64 byte block for node allocations
    uint32_t leafDataStart;         // first 64 byte block of leaf data
    uint32_t leafDataCur;           // next free 64 byte block for leaf allocations
    uint32_t proceduralDataStart;   // first 64 byte block for procedural leaf data
    uint32_t proceduralDataCur;     // next free 64 byte block for procedural leaf allocations
    uint32_t backPointerDataStart;  // first 64 byte block for back pointers
    uint32_t backPointerDataEnd;    // end of back pointer array

    // miscellaneous header information
    // ...

    // node data

    // These are really variable length arrays in memory but are here for
    // documentation purposes.
    InternalNode         innerNode[1];
    QuadLeaf<GenT>       geomLeaf[1];
    ProceduralLeaf<GenT> proceduralLeaf[1];
    InstanceLeaf<GenT>   instLeaf[1];
    uint32_t             backPointers[1];

    // array size = 256 - (sizeOfStructMembers % 256)
    // sizeOfStructMembers = 8          //uint64_t rootNodeOffset
    //                     + 2*12       // Vec3f   bounds_min; Vec3f   bounds_max;
    //                     + 9 * 4      // 9 * uint32_t
    //                     + 64         // InternalNode
    //                     + 64         // QuadLeaf
    //                     + 64         // ProceduralLeaf
    //                     + 128        // InstanceLeaf
    char __Padding[124];
};

constexpr uint32_t getRayFlagMask(uint32_t Val)
{
    return (Val << 1) - 1;
}

enum class RAYTRACING_PIPELINE_FLAGS : uint16_t
{
    NONE = 0x0,
    SKIP_TRIANGLES = 0x100,
    SKIP_PROCEDURAL_PRIMITIVES = 0x200,
};

enum class RayFlags : uint16_t
{
    NONE = 0x00,
    FORCE_OPAQUE = 0x01,                        // forces geometry to be opaque (no anyhit shader invokation)
    FORCE_NON_OPAQUE = 0x02,                    // forces geometry to be non-opqaue (invoke anyhit shader)
    ACCEPT_FIRST_HIT_AND_END_SEARCH = 0x04,     // terminates traversal on the first hit found (shadow rays)
    SKIP_CLOSEST_HIT_SHADER = 0x08,             // skip execution of the closest hit shader
    CULL_BACK_FACING_TRIANGLES = 0x10,          // back facing triangles to not produce a hit
    CULL_FRONT_FACING_TRIANGLES = 0x20,         // front facing triangles do not produce a hit
    CULL_OPAQUE = 0x40,                         // opaque geometry does not produce a hit
    CULL_NON_OPAQUE = 0x80,                     // non-opaque geometry does not produce a hit

    SKIP_TRIANGLES = 0x100,                     // Skip all triangle intersections and consider them as misses
    SKIP_PROCEDURAL_PRIMITIVES = 0x200,         // Skip execution of intersection shaders for procedural primitives

    // Anything below this point should not be propagated to RayFlags()

    //TRIANGLE_FRONT_COUNTERCLOCKWISE = 0x4000, // This value MUST not be programmed by SW but used internally by HW only.

    LEVEL_ASCEND_DISABLED = 0x8000,             // disables the automatic level ascend for this level,
                                                // thus traversal will terminate when BVH at this level is done
};

// DXR spec:
// "Only defined ray flags are propagated by the system, e.g. visible to the RayFlags() shader intrinsic."
constexpr uint32_t RayFlagsMask = getRayFlagMask((uint32_t)RayFlags::SKIP_PROCEDURAL_PRIMITIVES);

enum class RayFlags_Xe3 : uint16_t
{
    NONE = 0x00,
    FORCE_OPAQUE = 0x01,                        // forces geometry to be opaque (no anyhit shader invokation)
    FORCE_NON_OPAQUE = 0x02,                    // forces geometry to be non-opqaue (invoke anyhit shader)
    ACCEPT_FIRST_HIT_AND_END_SEARCH = 0x04,     // terminates traversal on the first hit found (shadow rays)
    SKIP_CLOSEST_HIT_SHADER = 0x08,             // skip execution of the closest hit shader
    CULL_BACK_FACING_TRIANGLES = 0x10,          // back facing triangles to not produce a hit
    CULL_FRONT_FACING_TRIANGLES = 0x20,         // front facing triangles do not produce a hit
    CULL_OPAQUE = 0x40,                         // opaque geometry does not produce a hit
    CULL_NON_OPAQUE = 0x80,                     // non-opaque geometry does not produce a hit

    SKIP_TRIANGLES = 0x100,                     // Skip all triangle intersections and consider them as misses
    SKIP_PROCEDURAL_PRIMITIVES = 0x200,         // Skip execution of intersection shaders for procedural primitives

    FORCE_2STATE_STOC = 0x400,                  // Force 2 state sub triangle opacity mode.
};

// DXR spec:
// "Only defined ray flags are propagated by the system, e.g. visible to the RayFlags() shader intrinsic."
constexpr uint32_t RayFlagsMask_Xe3 = getRayFlagMask((uint32_t)RayFlags_Xe3::FORCE_2STATE_STOC);
static_assert(sizeof(MemTravStack) == 32, "MemTravStack has to be 32 bytes large");

enum class RayQueryFlags : uint16_t
{
    NONE = 0x00,
};


// On DG2, writes will not go to the L1$ unless they are 16-byte aligned
// and at least 16 bytes in size.
constexpr uint32_t LSC_WRITE_GRANULARITY = 16;

struct StackPtrAndBudges
{
    enum class Bits : uint8_t
    {
        DimBits = 5,
    };

    using T = uint32_t;

    // This tracks the offset from the base of the HW portion of the stack
    // to wherever we currently point in the SW portion of the stack.
    uint16_t StackOffset;

    union {
        uint16_t BudgeBits;

        struct {
            uint16_t XSize : (T)Bits::DimBits;  // TODO: add description
            uint16_t YSize : (T)Bits::DimBits;  // TODO: add description
            uint16_t ZSize : (T)Bits::DimBits;  // TODO: add description
            uint16_t Pad   : 1;                 // explicit padding bit to get 16 bits in the union
        };
    };
};

static_assert(sizeof(StackPtrAndBudges) == 4, "wrong size?");

// The goal is to minimize the amount of data we put into the L1$.  Rather than
// storing a full a64 pointer and 3 dwords for the x,y,z DispatchRayIndex()
// values (2 + 3 = 5 dwords per ray), we note that the DXR spec says:
//
// "There are 3 dimensions passed in to set the grid size: width/height/depth.
//  These dimensions are constrained such that width*height*depth <= 2^30.
//  Exceeding this produces undefined behavior."
//
// This means we can compress the three dims into 30 bits and use the upper
// bits of the stack pointer dword to track how many bits are needed by
// each dimension
struct StackPtrDRIEncoding
{
    StackPtrAndBudges PtrAndBudges;
    uint32_t CompressedDispatchRayIndices;
};

static_assert(sizeof(StackPtrDRIEncoding) == 8, "wrong size?");

struct alignas(LSC_WRITE_GRANULARITY) SWHotZone_v1
{
    StackPtrDRIEncoding Encoding;

    // Compiler may elect to enlarge the hot-zone.
    //uint8_t additional_bytes[];

    // pad to LSC write granularity (16B on Gen12)
    char __Padding[LSC_WRITE_GRANULARITY - sizeof(StackPtrDRIEncoding)];
};

// We currently default to this encoding.  Set the 'EnableCompressedRayIndices'
// regkey to switch to the "v1" encoding above.
struct alignas(LSC_WRITE_GRANULARITY) SWHotZone_v2
{
    uint32_t StackOffset;
    uint32_t DispatchRaysIndex[3];

    // Compiler may elect to enlarge the hot-zone.
    //uint8_t additional_bytes[];

    // pad to LSC write granularity (16B on Gen12)
};

struct alignas(LSC_WRITE_GRANULARITY) SWHotZone_v3
{
    uint32_t StackOffset;
    uint32_t DispatchRaysIndex[3];

    uint32_t Complete;

    // pad to LSC write granularity (16B on Gen12)
    uint32_t Pad[3];
};

constexpr uint32_t StackFrameAlign = 16;
static_assert(IGC::RTStackAlign % LSC_WRITE_GRANULARITY == 0, "not aligned to write granularity?");

// This is the structure of memory that will be allocated by the UMD:
template <typename HotZoneTy, typename SyncStackTy, typename AsyncStackTy, typename SWStackTy,
          uint32_t RTStackAlign, uint64_t DSS_COUNT, uint64_t NumDSSRTStacks, uint64_t SIMD_LANES_PER_DSS>
struct RTMemory
{
    // Packed SW hot-zones
    alignas(IGC::RTStackAlign) HotZoneTy HotZones[DSS_COUNT * NumDSSRTStacks];

    // sync stacks for synchronous ray tracing
    alignas(RTStackAlign) SyncStackTy SyncStacks[DSS_COUNT * SIMD_LANES_PER_DSS];

    // RTMemBasePointer points here <----
    alignas(RTStackAlign) AsyncStackTy AsyncStacks[DSS_COUNT * NumDSSRTStacks];

    // Align to L3 sector size, or LSC sector size if stack is LSC-cached
    alignas(IGC::RTStackAlign) SWStackTy SWStacks[DSS_COUNT * NumDSSRTStacks];
};

// This will be invoked by the UMD to allocate stack space.
constexpr uint64_t calcRTMemoryAllocSize(
    uint64_t SWHotZoneSize,
    uint64_t SyncStackSize,
    uint64_t AsyncStackSize,
    uint64_t SWStackSize,
    uint64_t DSS_COUNT,
    uint64_t NumDSSRTStacks,
    uint64_t SIMD_LANES_PER_DSS)
{
    // SIMD_LANES_PER_DSS = EUCount * ThreadCount * SIMD16
    return IGC::Align(SWHotZoneSize * DSS_COUNT * NumDSSRTStacks, IGC::RTStackAlign) +
        IGC::Align(SyncStackSize * DSS_COUNT * SIMD_LANES_PER_DSS, IGC::RTStackAlign) +
        IGC::Align(AsyncStackSize * DSS_COUNT * NumDSSRTStacks, IGC::RTStackAlign) +
        IGC::Align(SWStackSize * DSS_COUNT * NumDSSRTStacks, IGC::RTStackAlign);
}


constexpr uint32_t getSyncStackSize() {
#define STYLE(X) static_assert(sizeofRTStack2<RTStack2<Xe>>() == sizeofRTStack2<RTStack2<X>>());
#include "RayTracingMemoryStyle.h"
#undef STYLE
    return sizeof(RTStack2<Xe>);
}

// As per this:
// The rtMemBasePtr points to the top of the async stacks.  After having computed
// the full allocation with 'calcRTMemoryAllocSize()', this returns the offset
// of the async stacks, hot zone, and sw stack from the base of the allocation.
constexpr uint64_t calcRTMemoryOffsets(
    uint64_t SWHotZoneSize,
    uint64_t SyncStackSize,
    uint64_t AsyncStackSize,
    uint64_t &SWHotZoneOffset,
    uint64_t &SWStackOffset,
    uint64_t DSS_COUNT,
    uint64_t NumDSSRTStacks,
    uint64_t SIMD_LANES_PER_DSS)
{
    uint64_t AsyncOffset =
        IGC::Align(SWHotZoneSize * DSS_COUNT * NumDSSRTStacks, IGC::RTStackAlign) +
        IGC::Align(SyncStackSize * DSS_COUNT * SIMD_LANES_PER_DSS, IGC::RTStackAlign);

    // The hot zone is the base of the allocation
    SWHotZoneOffset = 0;
    SWStackOffset =
        AsyncOffset +
        IGC::Align(AsyncStackSize * DSS_COUNT * NumDSSRTStacks, IGC::RTStackAlign);

    return AsyncOffset;
}


// unit tests:
static_assert(
    calcRTMemoryAllocSize(
        sizeof(SWHotZone_v1), sizeof(RTStack2<Xe>), sizeof(RTStack2<Xe>), 128,
        32, 2048, 16 * 8 * 16) ==
    sizeof(RTMemory<SWHotZone_v1, RTStack2<Xe>, RTStack2<Xe>, uint8_t[128], IGC::RTStackAlign,
        32, 2048, 16 * 8 * 16>), "mismatch?");

static_assert(
    calcRTMemoryAllocSize(
        8, sizeof(RTStack2<Xe>), sizeof(RTStack2<Xe>), 136,
        32, 2048, 16 * 8 * 16) ==
    sizeof(RTMemory<uint8_t[8], RTStack2<Xe>, RTStack2<Xe>, uint8_t[136], IGC::RTStackAlign,
        32, 2048, 16 * 8 * 16>), "mismatch?");


/* a list of commands for the ray tracing hardware */
enum class TraceRayCtrl : uint8_t
{
    TRACE_RAY_INITIAL = 0,              // Initializes hit and initializes traversal state
    TRACE_RAY_INSTANCE = 1,             // Loads committed hit and initializes traversal state
    TRACE_RAY_COMMIT = 2,               // Loads potential hit and loads traversal state
    TRACE_RAY_CONTINUE = 3,             // Loads committed hit and loads traversal state
    TRACE_RAY_NONE = 5                  // Illegal!
};

struct TraceRayPayload
{
    enum class PayloadBits : uint8_t
    {
        bvhLevel     = 3,
        reserved     = 5,
        traceRayCtrl = 3,
        reserved_1   = 5,
        stackID      = 12,
        reserved_2   = 4,
    };

    // This is the offset within the bitfield
    enum class PayloadOffsets : uint8_t
    {
        bvhLevel     = 0,
        reserved     = (uint8_t)PayloadBits::bvhLevel,
        traceRayCtrl = PayloadOffsets::reserved + (uint8_t)PayloadBits::reserved,
        reserved_1   = PayloadOffsets::traceRayCtrl + (uint8_t)PayloadBits::traceRayCtrl,
        stackID      = PayloadOffsets::reserved_1 + (uint8_t)PayloadBits::reserved_1,
    };
    // Trace Ray Payload (per SIMD lane)
    struct TraceRayPayloadData
    {
        uint32_t        bvhLevel     : (uint32_t)PayloadBits::bvhLevel;     // bits[  2:0]: tells the hardware which ray to process
        uint32_t        reserved     : (uint32_t)PayloadBits::reserved;     // bits[  7:3]: reserved MBZ
        uint32_t        traceRayCtrl : (uint32_t)PayloadBits::traceRayCtrl; // bits[ 10:8]: trace ray operation to perform
        uint32_t        reserved_1   : (uint32_t)PayloadBits::reserved_1;   // bits[15:11]: reserved MBZ
        uint32_t        stackID      : (uint32_t)PayloadBits::stackID;      // bits[27:16]: the ID of the stack of this thread
        uint32_t        reserved_2   : (uint32_t)PayloadBits::reserved_2;   // bits[31:28]: reserved MBZ
    } payload;
};

struct TraceRayMessage
{
    // This data is sent per message and is uniform across all rays in a dispatch.
    struct Header
    {
        // bits [63:0]
        uint64_t rtDispatchGlobalsPtr : 48;  // pointer to dispatch globals
        uint64_t padding;
        union {
            uint64_t rayQueryLocation;
            // bit 128
            uint64_t rayQuery         : 1;   // indicates a ray query message
            uint64_t rayQueryCheck    : 1;   // indicates a ray query check message
            uint64_t rayQueryRelease  : 1;   // indicates a ray query release message
        };
    } header;

    TraceRayPayload payload;

    static_assert(sizeof(TraceRayPayload::payload) == 4, "Payload must be 4 bytes");
};

// RayQuery enums

enum COMMITTED_STATUS : uint32_t
{
    COMMITTED_NOTHING,
    COMMITTED_TRIANGLE_HIT,
    COMMITTED_PROCEDURAL_PRIMITIVE_HIT
};

enum CANDIDATE_TYPE : uint32_t
{
    CANDIDATE_NON_OPAQUE_TRIANGLE,
    CANDIDATE_PROCEDURAL_PRIMITIVE
};

struct RayQueryReturnData
{
    enum class Bits : uint8_t
    {
        proceed_further = 1,
        committedStatus = 2,
        candidateType = 1,
        reserved = 28,
    };

    // This union is defined for futures use.
    // It will replace writing the RayQuery Return Value
    // to the MemHit in ShadowMemory.
    union
    {
        uint32_t rayQueryReturnDWord;

        struct
        {
            uint32_t PROCEED_FURTHER : (uint32_t)Bits::proceed_further; // This bit when set indicates that traversal is not complete i.e. all candidates have not been searched yet.
            COMMITTED_STATUS committedStatus : (uint32_t)Bits::committedStatus; // This bit field provides the Committed Hit's Status / Type.
            CANDIDATE_TYPE candidateType : (uint32_t)Bits::candidateType;   // This bit indicates the candidate type for a potential hit.
            uint32_t reserved : (uint32_t)Bits::reserved;        // Reserved
        };
    };
};

} // namespace RTStackFormat


DISABLE_WARNING_POP  // restore the previously saved pragma state, restore former compiler settings
