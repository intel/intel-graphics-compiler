/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file contains a definition of the structure of the ray tracing
/// stack as seen by a ray.  Each ray will have its own private copy of the
/// stack. Through a combination of offsetof() and sizeof(), passes will find
/// offsets to fields within the structs and update themselves automatically
/// when this file changes.
///
///
//===----------------------------------------------------------------------===//
#pragma once

#include "RayTracingRayDispatchGlobalData.h"
#include "RayTracingConstantsEnums.h"

#include <stdint.h>
#include <stddef.h>
#if !defined(__clang__) || (__clang_major__ >= 10)
#include <type_traits>
#endif


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
#if defined(__clang__)
    #define DO_PRAGMA(X) _Pragma(#X)                                                     // internal utility
    #define DISABLE_WARNING_PUSH         DO_PRAGMA(GCC diagnostic push)
    #define DISABLE_WARNING_POP          DO_PRAGMA(GCC diagnostic pop)
    #define DISABLE_WARNING(warningName) DO_PRAGMA(GCC diagnostic ignored #warningName)  // internal utility
    // Add warnings that you want to disable here:
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION   \
        DISABLE_WARNING(-Wgnu-anonymous-struct)  // anonymous structs are a GNU extension
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION \
        DISABLE_WARNING(-Wnested-anon-types)     // anonymous types declared in an anonymous union are an extension
#elif defined(__GNUC__)
    #define DO_PRAGMA(X) _Pragma(#X)                                                     // internal utility
    #define DISABLE_WARNING_PUSH         DO_PRAGMA(GCC diagnostic push)
    #define DISABLE_WARNING_POP          DO_PRAGMA(GCC diagnostic pop)
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


DISABLE_WARNING_PUSH       // save the current pragma state, save the current compiler settings
// select the warnings that we want to disable, for this file only
DISABLE_WARNING_ANONYMOUS_STRUCT_UNION
DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION


namespace IGC {

// will be patched with the global root signature at compile time
struct alignas(8) TypeHoleGlobalRootSig {};

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
constexpr static uint8_t TOP_LEVEL_BVH = 0;
constexpr static uint8_t BOTTOM_LEVEL_BVH = 1;

// Auxiliary types to make update of the structs easier.
using uint3 = uint32_t[3];
using uint4 = uint32_t[4];
using Vec3f = float[3];
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
    uint64_t _shader               : 48;  // shader function pointer:
                                          // In hardware this are only 26 bits and
                                          // functions are assumed to be 64 bit aligned.
                                          // The functional model cannot assume 64 bit alignment
                                          // and we have to use more bits to store function pointers.

    uint64_t _reserved1            : 10;  // unused padding bytes
};

static_assert(sizeof(KSP) == 8, "changed?");

// This is the structure of a shader identifier as of DG2 at least.  The compiler
// output structure (i.e., RayTracingPipelineOutput) must obey this format as
// this is where HW expects the shaders to be.
struct ShaderIdentifier
{
    KSP ClosestHit;
    union
    {
        KSP Intersection;
        KSP AnyHit;
    };
    KSP Unused1;
    KSP Unused2;

    static constexpr uint32_t NumSlots = SHADER_IDENTIFIER_SIZE_IN_BYTES / sizeof(KSP);
    static constexpr uint32_t RaygenFirstOpenSlot = 1;
    static constexpr uint32_t NumRaygenOpenSlots  = NumSlots - RaygenFirstOpenSlot;
};

static_assert(ShaderIdentifier::NumSlots == 4);
static_assert(sizeof(ShaderIdentifier) == SHADER_IDENTIFIER_SIZE_IN_BYTES, "changed?");

// will be patched with the local root signature at compile time
struct alignas(32) TypeHoleLocalRootSig {};

// A shader record is composed of two parts:
// +-------------------+-------------------+
// | Shader Identifier |     Local Args    |
// +-------------------+-------------------+
//  <----32 bytes-----> <--(4K - 32) max-->
//
// That is, the entire record can be at most 4096 bytes so at
// most 4096 - 32 = 4064 bytes can be referenced by a local
// root signature.
struct alignas(RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT) ShaderRecord
{
    ShaderIdentifier ID;
    TypeHoleLocalRootSig LocalRootSig;
};

struct MemHit {
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
        valid         = 16,
        leafType      = 17,
        primLeafIndex = 20,
        bvhLevel      = 24,
        frontFace     = 27,
        done          = 28,
    };

    using T = uint32_t;

    union
    {
        uint32_t topOfPrimIndexDelta;
        uint32_t frontFaceDword;
        uint32_t hitInfoDWord;

        struct {
            uint32_t primIndexDelta : (T)Bits::primIndexDelta; // prim index delta for compressed meshlets and quads
            uint32_t valid          : (T)Bits::valid;          // set if there is a hit
            uint32_t leafType       : (T)Bits::leafType;       // type of node primLeafPtr is pointing to
            uint32_t primLeafIndex  : (T)Bits::primLeafIndex;  // index of the hit primitive inside the leaf
            uint32_t bvhLevel       : (T)Bits::bvhLevel;       // the instancing level at which the hit occured
            uint32_t frontFace      : (T)Bits::frontFace;      // whether we hit the front-facing side of a triangle (also used to pass opaque flag when calling intersection shaders)
            uint32_t done           : (T)Bits::done;           // used in sync mode to indicate that traversal is done (HW will only set this to 0)
            uint32_t pad0           : (T)Bits::pad0;           // unused bits (explicit padding)
        };
    };

    union
    {
        uint64_t topOfPrimLeafPtr;

        struct {
            uint64_t primLeafPtr     : (T)Bits::primLeafPtr;     // pointer to BVH leaf node (multiple of 64 bytes)
            uint64_t hitGroupRecPtr0 : (T)Bits::hitGroupRecPtr0; // LSB of hit group record of the hit triangle (multiple of 16 bytes)
        };
    };

    union
    {
        uint64_t topOfInstLeafPtr;

        struct {
            uint64_t instLeafPtr     : (T)Bits::instLeafPtr;     // pointer to BVH instance leaf node (in multiple of 64 bytes)
            uint64_t hitGroupRecPtr1 : (T)Bits::hitGroupRecPtr1; // MSB of hit group record of the hit triangle (multiple of 16 bytes)
        };
    };

    static_assert((uint32_t)Bits::primLeafPtr == (uint32_t)Bits::instLeafPtr,
        "Size changed?");
};

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

static constexpr const uint32_t NUM_CHILDREN = 6;

struct InternalNode
{
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

enum GeometryFlags : uint32_t
{
    NONE = 0x0,
    RTX_OPAQUE = 0x1
};

struct PrimLeafDesc
{
    enum Type : uint32_t
    {
        TYPE_NONE = 0,

        /* For a node type of NODE_TYPE_MESHLET, the referenced leaf may
         * still be a QuadLeaf or a Meshlet. We need this as we produce
         * two quads instead of one meshlet when meshlet compression does
         * not work well. */

        TYPE_QUAD = 0,
        TYPE_MESHLET = 1,

        /* For a node type of NODE_TYPE_PROCEDURAL we support enabling
         * and disabling the opaque/non_opaque culling. */

        TYPE_OPACITY_CULLING_ENABLED = 0,
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

    union
    {
        uint32_t topOfGeomIndex;

        struct {
            uint32_t geomIndex : (T)Bits::geomIndex; // the geometry index specifies the n'th geometry of the scene
            /* Type */
            uint32_t type      : (T)Bits::type;      // distinguish between QuadLeaves and Meshlets or enable/disable culling for procedurals and instances
            /* GeometryFlags */
            uint32_t geomFlags : (T)Bits::geomFlags; // geometry flags of this geometry
        };
    };
};

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

    PrimLeafDesc leafDesc;                            // leaf header identifying the geometry
    uint32_t numPrimitives : (T)Bits::numPrimitives;  // number of stored primitives
    uint32_t pad           : (T)Bits::pad;            // explicit padding bits
    uint32_t last          : (T)Bits::last;           // bit vector with a last bit per primitive
    uint32_t _primIndex[N];                           // primitive indices of all primitives stored inside the leaf
};

static_assert(sizeof(ProceduralLeaf) == 64, "ProceduralLeaf must be 64 bytes large");

struct QuadLeaf
{
    PrimLeafDesc leafDesc;  // the leaf header

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

    union {
        uint32_t topOfPriIndex1Delta;

        struct {
            uint32_t primIndex1Delta : (T)Bits::primIndex1Delta;  // delta encoded primitive index of second triangle
            uint32_t j0              : (T)Bits::j0;               // specifies first vertex of second triangle
            uint32_t j1              : (T)Bits::j1;               // specified second vertex of second triangle
            uint32_t j2              : (T)Bits::j2;               // specified third vertex of second triangle
            uint32_t last            : (T)Bits::last;             // true if the second triangle is the last triangle in a leaf list
            uint32_t pad             : (T)Bits::pad;              // unused bits
        };
    };

    Vec3f v0;  // first vertex of first triangle
    Vec3f v1;  // second vertex of first triangle
    Vec3f v2;  // third vertex of first triangle
    Vec3f v3;  // additional vertex only used for second triangle
};

static_assert(sizeof(QuadLeaf) == 64, "QuadLeaf must be 64 bytes large");

static_assert(sizeof(QuadLeaf) == sizeof(ProceduralLeaf),
    "Leaves must be same size");

constexpr uint32_t LeafSize = sizeof(QuadLeaf);

/*
  The CompressedMeshlet structure stores triangles compressed
  losslessly. Between 1 to 16 triangles can get stored that index
  into a vertex array with up to 16 vertices.

  The structure contains a header, a list of delta compressed
  indices (allocated front to back), and a list of compressed
  vertices (allocated back to front).

 */
struct CompressedMeshlet
{
    PrimLeafDesc leafDesc;
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

static_assert(sizeof(CompressedMeshlet) == 128, "CompressedMeshlet has to be 128 bytes large");

enum class InstanceFlags : uint8_t
{
    NONE = 0x0,
    TRIANGLE_CULL_DISABLE = 0x1,
    TRIANGLE_FRONT_COUNTERCLOCKWISE = 0x2,
    FORCE_OPAQUE = 0x4,
    FORCE_NON_OPAQUE = 0x8
};

struct InstanceLeaf
{
    /* first 64 bytes accessed during traversal by hardware */
    struct Part0
    {
    public:
        enum class Bits : uint8_t
        {
            shaderIndex = 24,
            geomMask = 8,

            instContToHitGrpIndex = 24,
            pad0 = 5,
            type = 1,
            geomFlags = 2,

            startNodePtr = 48,
            instFlags = 8,
            reserved1 = 1,
            reserved2 = 7,
        };

        using T = uint32_t;

        uint32_t shaderIndex : (T)Bits::shaderIndex;  // shader index used to calculate instancing shader in case of software instancing
        uint32_t geomMask    : (T)Bits::geomMask;     // geometry mask used for ray masking

        union {
            uint32_t instContToHitGroupIndex;

            struct {
                uint32_t instanceContributionToHitGroupIndex : (T)Bits::instContToHitGrpIndex;  // TODO: add description
                uint32_t pad0                                : (T)Bits::pad0;                   // explicit padding bits
                /* PrimLeafDesc::Type */
                uint32_t type                                : (T)Bits::type;                   // enables/disables opaque culling
                /* GeometryFlags */
                uint32_t geomFlags                           : (T)Bits::geomFlags;              // geometry flags are not used for instances
            };
        };

        uint64_t startNodePtr : (T)Bits::startNodePtr;  // start node where to continue traversal of the instanced object
        uint64_t instFlags    : (T)Bits::instFlags;     // flags for the instance (see InstanceFlags)

        // DG2
        //uint64_t pad1         : 8;                    // unused bits (explicit padding)
        uint64_t reserved1 : (T)Bits::reserved1;  // 0 for less than or equal, 1 for greater
        uint64_t reserved2: (T)Bits::reserved2; // to be compared with ray.ComparisonValue


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

static_assert(sizeof(InstanceLeaf) == 128, "InstanceLeaf must be 128 bytes large");

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
    InternalNode   innerNode[1];
    QuadLeaf       geomLeaf[1];
    ProceduralLeaf proceduralLeaf[1];
    InstanceLeaf   instLeaf[1];
    uint32_t       backPointers[1];
};

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

struct MemRay
{
    // 32 B
    Vec3f org;         // the origin of the ray
    Vec3f dir;         // the direction of the ray
    float tnear;       // the start of the ray
    float tfar;        // the end of the ray

    enum class Bits : uint8_t
    {
        rootNodePtr = 48,
        rayFlags    = 16,

        rayFlagsCopy = rayFlags,

        hitGroupSRBasePtr = 48,
        hitGroupSRStride  = 16,

        missSRPtr             = 48,
        pad                   = 1,
        shaderIndexMultiplier = 8,

        instLeafPtr = 48,
        rayMask     = 8,

        ComparisonValue = 7,
    };

    using T = uint32_t;

    // This is the offset within the bitfield
    enum class Offset : uint8_t
    {
        // Add as needed
        ComparisonValue = (T)Bits::missSRPtr + (T)Bits::pad,
        shaderIndexMultiplier = (T)Offset::ComparisonValue + (T)Bits::ComparisonValue,
        rayFlagsCopy = 0,
        rayMask = (uint32_t)Bits::instLeafPtr,
    };

    // 32 B
    union {
        uint64_t topOfNodePtrAndFlags;

        struct {
            uint64_t rootNodePtr : (T)Bits::rootNodePtr; // root node to start traversal at
            uint64_t rayFlags    : (T)Bits::rayFlags;    // ray flags (see RayFlags structure)
        };
    };

    union {
        uint64_t hitGroupShaderRecordInfo;

        struct {
            uint64_t hitGroupSRBasePtr : (T)Bits::hitGroupSRBasePtr; // base of hit group shader record array (16-bytes alignment)
            uint64_t hitGroupSRStride  : (T)Bits::hitGroupSRStride;  // stride of hit group shader record array (16-bytes alignment)
        };
    };

    union {
        uint64_t missShaderRecordInfo;

        struct {
            uint64_t missSRPtr             : (T)Bits::missSRPtr;             // pointer to miss shader record to invoke on a miss (8-bytes alignment)

            // DG2
            //uint64_t pad                 : 8;                              // explicit padding bits
            uint64_t pad                   : (T)Bits::pad;                   // explicit padding bits
            uint64_t ComparisonValue       : (T)Bits::ComparisonValue;       // to be compared with Instance.ComparisonValue


            uint64_t shaderIndexMultiplier : (T)Bits::shaderIndexMultiplier; // shader index multiplier
        };
    };

    union {
        uint64_t topOfInstanceLeafPtr;

        struct {
            // the 'instLeafPtr' is not actually used by HW in the TOP_LEVEL_BVH.
            // We insert the user set rayflags here (i.e., the flags set without
            // the pipeline flags from the RTPSO). Other IHVs thought pipeline
            // flags would not modify the results of RayFlags(). So we will read
            // that value from here and write the flags|pipline in the rayFlags
            // above as usual. DXR spec will be modified to this behavior.

            uint64_t instLeafPtr : (T)Bits::instLeafPtr;  // the pointer to instance leaf in case we traverse an instance (64-bytes alignment)
            uint64_t rayMask     : (T)Bits::rayMask;      // ray mask used for ray masking
        };
    };
};

static_assert(sizeof(MemHit) == 32,       "MemHit has to be 32 bytes large");
static_assert(sizeof(MemRay) == 64,       "MemRay has to be 64 bytes large");
static_assert(sizeof(MemTravStack) == 32, "MemTravStack has to be 32 bytes large");

//org, dir, tnear and tfar
constexpr uint32_t RayInfoSize = 8;


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

constexpr uint32_t StackFrameAlign = 16;
static_assert(IGC::RTStackAlign % LSC_WRITE_GRANULARITY == 0, "not aligned to write granularity?");

// This is the portion of the RTStack that we read and write from to
// communicate with the RTUnit.
template <uint32_t MaxBVHLevels>
struct HWRayData
{
    MemHit committedHit;    // stores committed hit
    MemHit potentialHit;    // stores potential hit that is passed to any hit shader

    MemRay       ray[MaxBVHLevels];       // stores a ray for each instancing level
    MemTravStack travStack[MaxBVHLevels]; // spill location for the internal stack state per instancing level
};

// This is the ShadowMemory that we maintain if we DONOT need spill/fill it to/from HW RTStack.
// We keep this data structure as small as possible to reduce the size.
template <uint32_t MaxBVHLevels>
struct SMRayData
{
    MemHit committedHit;    // stores committed hit
    MemHit potentialHit;    // stores potential hit that is passed to any hit shader

    MemRay       ray[MaxBVHLevels];       // stores a ray for each instancing level
    //MemTravStack travStack[MaxBVHLevels]; // spill location for the internal stack state per instancing level
};

// This is the raytracing software stack.  Every allocated stack will have
// this header at the very beginning.  The stack convention described at the
// top of this file will reside right after this data (i.e., we offset past
// this header to get to the compiler managed stack).
template <uint32_t MaxBVHLevels>
struct RTStack
{
    // HW accessible region
    HWRayData<MaxBVHLevels> hwRayData;
};

// This is the ShadowMemory's RTStack.
template <uint32_t MaxBVHLevels>
struct SMStack
{
    // SM accessible region
    SMRayData<MaxBVHLevels> smRayData;
};

// For now, we're just defaulting to using an RTStack assuming
// MAX_BVH_LEVELS == 2.
using RTStack2   = RTStack<MAX_BVH_LEVELS>;
using HWRayData2 = HWRayData<MAX_BVH_LEVELS>;
using SMStack2   = SMStack<MAX_BVH_LEVELS>;
using SMRayData2 = SMRayData<MAX_BVH_LEVELS>;

#if !defined(__clang__) || (__clang_major__ >= 10)
static_assert(std::is_standard_layout_v<RTStack2>);
static_assert(std::is_standard_layout_v<SMStack2>);
#endif

// Makes sure that important fields are at their proper offset from the starty of the structure.
// Update this if the structure changes.  This is just for documentation purposes.
static_assert(offsetof(HWRayData2, committedHit) == 0,  "unexpected offset!");
static_assert(offsetof(HWRayData2, potentialHit) == 32, "unexpected offset!");
static_assert(offsetof(HWRayData2, ray) == 64,          "unexpected offset!");
static_assert(offsetof(HWRayData2, travStack) == 192,   "unexpected offset!");
static_assert(offsetof(RTStack2, hwRayData) == 0,       "unexpected offset!");

// This is the structure of memory that will be allocated by the UMD:
template <typename HotZoneTy, typename HWRayDataTy, typename RTStackTy, typename SWStackTy,
          uint64_t DSS_COUNT, uint64_t NumDSSRTStacks, uint64_t SIMD_LANES_PER_DSS>
struct RTMemory
{
    // Packed SW hot-zones
    alignas(IGC::RTStackAlign) HotZoneTy HotZones[DSS_COUNT * NumDSSRTStacks];

    // HWRayData for synchronous ray tracing
    alignas(IGC::RTStackAlign) HWRayDataTy SyncStacks[DSS_COUNT * SIMD_LANES_PER_DSS];

    // RTMemBasePointer points here <----
    alignas(IGC::RTStackAlign) RTStackTy AsyncStacks[DSS_COUNT * NumDSSRTStacks];

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
    return IGC::Align(SWHotZoneSize * DSS_COUNT * NumDSSRTStacks, IGC::RTStackAlign)     +
           IGC::Align(SyncStackSize * DSS_COUNT * SIMD_LANES_PER_DSS, IGC::RTStackAlign) +
           IGC::Align(AsyncStackSize * DSS_COUNT * NumDSSRTStacks, IGC::RTStackAlign)    +
           IGC::Align(SWStackSize * DSS_COUNT * NumDSSRTStacks, IGC::RTStackAlign);
}

constexpr uint32_t getSyncStackSize(){ return sizeof(HWRayData2); }

// As per this:
// The rtMemBasePtr points to the top of the async stacks.  After having computed
// the full allocation with 'calcRTMemoryAllocSize()', This returns the offset
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
        sizeof(SWHotZone_v1), sizeof(HWRayData2), sizeof(HWRayData2), 128,
        32, 2048, 16 * 8 * 16) ==
    sizeof(RTMemory<SWHotZone_v1, HWRayData2, HWRayData2, uint8_t[128],
        32, 2048, 16 * 8 * 16>), "mismatch?");

static_assert(
    calcRTMemoryAllocSize(
        8, sizeof(HWRayData2), sizeof(HWRayData2), 136,
        32, 2048, 16 * 8 * 16) ==
    sizeof(RTMemory<uint8_t[8], HWRayData2, HWRayData2, uint8_t[136],
        32, 2048, 16 * 8 * 16>), "mismatch?");

struct TraceRayMessage
{
    /* a list of commands for the ray tracing hardware */
    enum TraceRayCtrl : uint8_t
    {
        TRACE_RAY_INITIAL = 0,              // Initializes hit and initializes traversal state
        TRACE_RAY_INSTANCE = 1,             // Loads committed hit and initializes traversal state
        TRACE_RAY_COMMIT = 2,               // Loads potential hit and loads traversal state
        TRACE_RAY_CONTINUE = 3,             // Loads committed hit and loads traversal state
        TRACE_RAY_NONE = 5                  // Illegal!
    };

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
        };
    } header;

    // this data is sent per ray (SIMD LANE)
    struct Payload
    {
        uint8_t      bvhLevel;     // the level tells the hardware which ray to process
        TraceRayCtrl traceRayCtrl; // the command the hardware should perform
        uint16_t     stackID;      // the ID of the stack of this thread
    } payload;

    static_assert(sizeof(Payload) == 4, "Payload must be 4 bytes");
};

// Extra rayquery information which are not part of RTStack
struct RayQueryStateInfo
{
    TraceRayMessage::TraceRayCtrl traceRayCtrl;
    //add more here if needed
};

// ShadowMemory: memory allocated by SW to spill/fill RTStackFormat::RayQueryObject
// ShadowMemory holds an array of RayQueryObject
// Implementation wise, ShadowMemory is used blindly by RayTracing FrontEnd no matter how many RayQueryObjects there are.
// And we rely on later optimization to boil away unnecessary ShadowMemory accesses.
// Theoretically (if optimization works perfectly well), there should be NO ShadowMemory access in GEN kernel
// unless there are multiple RayQuery objects whose liveness overlap with each other.
// NOTE: Not all information in RayQueryObject should be spill/fill, for example, those ShaderRecord information is only for async traceray.
// And we might want to avoid spill/fill the whole RayQueryObject. On the other hand, we want to reuse existing data structure like RTStack
// instead of recreating wheels. So, temporarily, we might still spill/fill the whole RayQueryObject.
template <uint32_t MaxBVHLevels>
struct RayQueryObject
{
    RTStack<MaxBVHLevels> rtStack;
    RayQueryStateInfo   stateInfo;
};

using RayQueryObject2 = RayQueryObject<MAX_BVH_LEVELS>;
//static_assert(std::is_standard_layout_v<RayQueryObject2>);

// TraceRayInline enums

enum COMMITTED_STATUS : uint32_t
{
    COMMITTED_NOTHING,
    COMMITTED_TRIANGLE_HIT,
    COMMITTED_PROCEDURAL_PRIMITIVE_HIT
};

} // namespace RTStackFormat


DISABLE_WARNING_POP  // restore the previously saved pragma state, restore former compiler settings
