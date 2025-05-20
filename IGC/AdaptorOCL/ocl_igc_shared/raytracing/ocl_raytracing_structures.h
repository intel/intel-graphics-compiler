/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// TODO: can we reuse RTStackFormat.h?

using Vec3f = float[3];

enum NodeType
{
  NODE_TYPE_MIXED = 0x0,        // identifies a mixed internal node where each child can have a different type
  NODE_TYPE_INTERNAL = 0x0,     // internal BVH node with 6 children
  NODE_TYPE_INSTANCE = 0x1,     // instance leaf
  NODE_TYPE_PROCEDURAL = 0x3,   // procedural leaf
  NODE_TYPE_QUAD = 0x4,         // quad leaf
  NODE_TYPE_MESHLET = 0x6,      // meshlet leaf
  NODE_TYPE_INVALID = 0x7       // indicates invalid node
};

struct MemRay
{
  // 32 B
  Vec3f org;         // the origin of the ray
  Vec3f dir;         // the direction of the ray
  float tnear;       // the start of the ray
  float tfar;        // the end of the ray

  // 32 B
  uint64_t rootNodePtr : 48;  // root node to start traversal at
  uint64_t rayFlags : 16;     // ray flags (see RayFlag structure)

  uint64_t hitGroupSRBasePtr : 48; // base of hit group shader record array (16-bytes alignment)
  uint64_t hitGroupSRStride : 16;  // stride of hit group shader record array (16-bytes alignment)

  uint64_t missSRPtr : 48;  // pointer to miss shader record to invoke on a miss (8-bytes alignment)
  uint64_t pad : 8;
  uint64_t shaderIndexMultiplier : 8; // shader index multiplier

  uint64_t instLeafPtr : 48;  // the pointer to instance leaf in case we traverse an instance (64-bytes alignment)
  uint64_t rayMask : 8;       // ray mask used for ray masking
};
static_assert(sizeof(MemRay) == 64, "Wrong MemRay size");

struct MemHit {
  float    t;                   // hit distance of current hit (or initial traversal distance)
  float    u,v;                 // barycentric hit coordinates

  uint32_t primIndexDelta  : 16; // prim index delta for compressed meshlets and quads
  uint32_t valid           : 1; // set if there is a hit
  uint32_t leafType        : 3; // type of node primLeafPtr is pointing to
  uint32_t primLeafIndex   : 4; // index of the hit primitive inside the leaf
  uint32_t bvhLevel        : 3; // the instancing level at which the hit occured
  uint32_t frontFace       : 1; // whether we hit the front-facing side of a triangle (also used to pass opaque flag when calling intersection shaders)
  uint32_t done            : 1; // used in sync mode to indicate that traversal is done
  uint32_t pad0            : 3; // unused bits

  uint64_t primLeafPtr     : 42; // pointer to BVH leaf node (multiple of 64 bytes)
  uint64_t hitGroupRecPtr0 : 22; // LSB of hit group record of the hit triangle (multiple of 16 bytes)
  uint64_t instLeafPtr     : 42; // pointer to BVH instance leaf node (in multiple of 64 bytes)
  uint64_t hitGroupRecPtr1 : 22; // MSB of hit group record of the hit triangle (multiple of 16 bytes)
};
static_assert(sizeof(MemHit) == 32, "Wrong MemHit size");

struct NodeInfo
{
  uint8_t type      : 3;
  uint8_t  parent   : 1;  // Indicates a culled stack entry where a single parent node
                          // is stored in place of multiple child nodes
  uint8_t  cur_prim : 4;
};
static_assert(sizeof(NodeInfo) == 1, "Wrong NodeInfo size");

static const uint32_t MEM_STACK_SIZE = 4;

struct MemTravStack
{
  uint64_t curDepth      : 5;        // current depth in the restart trail
  uint64_t restartTrail0 : 29;       // lower bits of restart trail
  uint64_t restartTrail1 : 29;       // higher bits of restart trail
  uint64_t lastChild0    : 1;        // last child bit for node 0
  uint32_t restartTrail2 : 29;       // highest bits of restart trail
  uint32_t lastChild123  : 3;        // last child bit for nodes 1/2/3

  int32_t offset[MEM_STACK_SIZE];    // Signed offset relative to BVH root in multiples of 64B.
  NodeInfo nodeInfo[MEM_STACK_SIZE];
};
static_assert(sizeof(MemTravStack) == 32, "Wrong MemTravStack size");

struct alignas(32) RTDispatchGlobals
{
  // Cached by HW
public:
  uint64_t rtMemBasePtr;               // base address of the allocated stack memory
  uint64_t callStackHandlerKSP;        // this is the KSP of the continuation handler that is invoked by BTD when the read KSP is 0
  uint32_t stackSizePerRay;            // maximal stack size of a ray in 64 byte blocks
  uint32_t numDSSRTStacks;             // number of stacks per DSS
  // TODO: update with:
  // uint32_t maxBVHLevels       : 3;      // the maximal number of supported instancing levels, 0->8, 1->1, 2->2, etc.
  // uint32_t hitGroupStride     : 13;    // stride of hit group shader records (16-bytes alignment)
  // uint32_t missShaderStride   : 13;    // stride of miss shader records (8-bytes alignment)
  // uint32_t _pad2_mbz          : 3;
  uint32_t maxBVHLevels;
  uint32_t flags              : 1;
  uint32_t pad_mbz            : 31;

  // Not cached by HW
public:
  uint64_t hitGroupBasePtr;            // base pointer of hit group shader record array (16-bytes alignment)
  uint64_t missShaderBasePtr;          // base pointer of miss shader record array (8-bytes alignment)
  uint32_t _align_mbz[4];              // pad hardware section to 64 bytes
};
