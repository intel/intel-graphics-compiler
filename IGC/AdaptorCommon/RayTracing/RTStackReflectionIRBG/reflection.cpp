/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This file is processed by the IRBuilderGenerator tool.
///
//===----------------------------------------------------------------------===//

// Set this so that __builtin_offset() is used so the result of offsetof() is
// a constexpr on windows.
#ifdef _WIN32
#define _CRT_USE_BUILTIN_OFFSETOF 1
#endif // _WIN32

#include "ConstantsEnums.h"
#include "RTStackFormat.h"
#include "../../common/RaytracingShaderTypes.h"
#include "BuilderUtils.h"
#include "AutoGenDesc.h"

using namespace RTStackFormat;
using namespace IGC;

namespace hook {
namespace bi {
PUREBUILTIN bool supportStochasticLod();
PUREBUILTIN RTGAS RayDispatchGlobalData *getGlobalBufferPtr();
PUREBUILTIN uint64_t canonizePointer(uint64_t);
BUILTIN void createReadSyncTraceRay(uint32_t);
PUREBUILTIN bool isRayQueryReturnOptimizationEnabled();
PUREBUILTIN uint32_t ctlz(uint32_t);
PUREBUILTIN uint32_t cttz(uint32_t);
PUREBUILTIN uint32_t get32BitLaneIDReplicate();
PUREBUILTIN uint32_t get32BitLaneID();
PUREBUILTIN uint32_t getSr0_0();
PUREBUILTIN uint32_t getSimdSize();
PUREBUILTIN uint32_t getMaxSimdSize();
PUREBUILTIN uint32_t getMaxThreadsPerEU();
PUREBUILTIN uint16_t createSubgroupId();
PUREBUILTIN uint32_t createGroupId(uint32_t Dim);
BUILTIN SWHotZoneAS SWHotZone_v3 *createDummyInstID(SWHotZoneAS SWHotZone_v3 *Ptr);


} // namespace bi
namespace fn {
BUILTIN uint32_t getPrimitiveIDBIFFunction();
BUILTIN uint32_t getGeometryIndexBIFFunction();
} // namespace fn
} // namespace hook

#define EXTERNAL_HOOK(DECL)                                                                                            \
  namespace hook {                                                                                                     \
  namespace fn {                                                                                                       \
  DECL;                                                                                                                \
  }                                                                                                                    \
  }

#define STYLE(X)                                                                                                       \
  TYPEOF RTStack2<X> _gettype_RTStack2_##X() { return {}; }
#include "RayTracingMemoryStyle.h"
#undef STYLE

#define STYLE(X)                                                                                                       \
  TYPEOF SMStack2<X> _gettype_SMStack2_##X() { return {}; }
#include "RayTracingMemoryStyle.h"
#undef STYLE

TYPEOF SWHotZone_v1 _gettype_SWHotZone_v1() { return {}; }
TYPEOF SWHotZone_v2 _gettype_SWHotZone_v2() { return {}; }
TYPEOF SWHotZone_v3 _gettype_SWHotZone_v3() { return {}; }

TYPEOF RayDispatchGlobalData _gettype_RayDispatchGlobalData() { return {}; }

ALIGNOF TypeHoleGlobalRootSig _alignof_TypeHoleGlobalRootSig() { return {}; }

TYPEOF RTGlobalsAndRootSig _gettype_RTGlobalsAndRootSig() { return {}; }

//////////// end type-of ////////////

// RayDispatchGlobalData accesses
#define RTGlobals(field, Gen, entry)                                                                                   \
  IMPL auto IMPL_get_##field##_fromGlobals_##Gen(RTGAS IGC::RayDispatchGlobalData *globalBufferPtr) {                  \
    return globalBufferPtr->rt.entry.field;                                                                            \
  }                                                                                                                    \
                                                                                                                       \
  CREATE_PRIVATE auto _get_##field##_fromGlobals_##Gen(RTGAS IGC::RayDispatchGlobalData *globalBufferPtr) {            \
    return IMPL_get_##field##_fromGlobals_##Gen(globalBufferPtr);                                                      \
  }                                                                                                                    \
                                                                                                                       \
  CREATE_PRIVATE auto _get_##field##_##Gen() {                                                                         \
    return IMPL_get_##field##_fromGlobals_##Gen(hook::bi::getGlobalBufferPtr());                                       \
  }

#define RTGlobalsCommon(field)                                                                                         \
  IMPL auto IMPL_get_##field##_fromGlobals(RTGAS IGC::RayDispatchGlobalData *globalBufferPtr) {                        \
    return globalBufferPtr->rt.xe.common.field;                                                                        \
  }                                                                                                                    \
                                                                                                                       \
  CREATE_PRIVATE auto _get_##field##_fromGlobals(RTGAS IGC::RayDispatchGlobalData *globalBufferPtr) {                  \
    return IMPL_get_##field##_fromGlobals(globalBufferPtr);                                                            \
  }                                                                                                                    \
                                                                                                                       \
  CREATE_PRIVATE auto _get_##field() { return IMPL_get_##field##_fromGlobals(hook::bi::getGlobalBufferPtr()); }

//////////// Start Xe specific fields ////////////
RTGlobals(rtMemBasePtr, Xe, xe);
RTGlobals(maxBVHLevels, Xe, xe.rt_data_info);
RTGlobals(stackSizePerRay, Xe, xe.stack_size_info);
RTGlobals(numDSSRTStacks, Xe, xe.num_stacks_info);
RTGlobals(maxBVHLevels, Xe3, xe3.rt_data_info);
//////////// End Xe specific fields ////////////

//////////// Start Common Fields ////////////
RTGlobalsCommon(statelessScratchPtr);


CREATE_PRIVATE auto
_getBaseSurfaceStatePointerFromPointerToGlobals(RTGAS RayDispatchGlobalData *__restrict__ GlobalsPtr) {
  return GlobalsPtr->rt.xe3.common.baseSurfaceStatePointer;
}

//////////// End Common Fields ////////////

#undef RTGlobals
#undef RTGlobalsCommon

//////////// anchors ////////////
//
// These methods are only here to ensure that the associated type is used
// so it appears in the generated IR. Note that we don't annotate these with
// "create" since they aren't actually used for codegen.
//
#define STYLE(X)                                                                                                       \
  ATTR auto __anchor_SMStack2_##X(SMStack2<X> *p) { return &p->ray0; }
#include "RayTracingMemoryStyle.h"
#undef STYLE

//////////// create ////////////
CREATE_PRIVATE auto _gepof_StackOffset_v1(SWHotZone_v1 *p) { return &p->Encoding.PtrAndBudges.StackOffset; }

CREATE_PRIVATE auto _gepof_StackOffset_v2(SWHotZone_v2 *p) { return &p->StackOffset; }

CREATE_PRIVATE auto _getDispatchRaysIndex_HotZone_v1(SWHotZoneAS SWHotZone_v1 *__restrict__ HotZonePtr, uint32_t dim) {
  auto &Budges = HotZonePtr->Encoding.PtrAndBudges;
  uint32_t X = Budges.XSize;
  uint32_t Y = Budges.YSize;
  uint32_t Z = Budges.ZSize;
  uint32_t PriorBits = ((dim > 0) ? X : 0) + ((dim > 1) ? Y : 0);
  uint32_t SelfBits = 0;
  if (dim == 0)
    SelfBits = X;
  if (dim == 1)
    SelfBits = Y;
  if (dim == 2)
    SelfBits = Z;
  SelfBits = (1 << SelfBits) - 1;
  return (HotZonePtr->Encoding.CompressedDispatchRayIndices >> PriorBits) & SelfBits;
}

CREATE_PRIVATE auto _getDispatchRaysIndex_HotZone_v2(SWHotZoneAS SWHotZone_v2 *__restrict__ HotZonePtr, uint32_t dim) {
  return HotZonePtr->DispatchRaysIndex[dim];
}

CREATE_PRIVATE void _setDispatchRaysIndex_HotZone_v1(SWHotZoneAS SWHotZone_v1 *__restrict__ HotZonePtr, uint32_t X,
                                                     uint32_t Y, uint32_t Z) {
  uint32_t Indices[] = {X, Y, Z};
  uint32_t CompressedVal = 0;
  uint32_t CurLoc = 0;
  uint16_t Budges = 0;
#pragma unroll
  for (uint32_t i = 0; i < 3; i++) {
    uint32_t UsedBits = 32 - hook::bi::ctlz(Indices[i]);
    CompressedVal |= Indices[i] << CurLoc;
    Budges |= (uint16_t)UsedBits << (i * (uint32_t)StackPtrAndBudges::Bits::DimBits);
    CurLoc += UsedBits;
  }

  HotZonePtr->Encoding.PtrAndBudges.BudgeBits = Budges;
  HotZonePtr->Encoding.CompressedDispatchRayIndices = CompressedVal;
}

CREATE_PRIVATE void _setDispatchRaysIndex_HotZone_v2(SWHotZoneAS SWHotZone_v2 *__restrict__ HotZonePtr, uint32_t dim,
                                                     uint32_t Val) {
  HotZonePtr->DispatchRaysIndex[dim] = Val;
}

CREATE_PRIVATE uint64_t _getBVHPtr(uint64_t BVHI, uint64_t Offset, bool FixedOffset) {
  // The DXR spec says:
  // "Specifying a NULL acceleration structure forces a miss."

  if (FixedOffset) {
    return BVHI ? BVHI + Offset : 0;
  }

  // Previously, we loaded the offset to the bvh out of the acceleration
  // structure and added it to the base to get the rootNodePtr.
  //
  // This path is here for legacy purposes, but we don't use it currently.

  auto *BVHPtr = (CAS BVH<Xe> *)BVHI;
  return BVHI ? BVHI + BVHPtr->rootNodeOffset : 0;
}


template <typename GenT> IMPL auto _getWorldRayOrig(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t Dim) {
  return StackPtr->ray0.org[Dim];
}
IMPL_ALL_2ARG(_getWorldRayOrig, StackPtr, Dim)

template <typename GenT>
IMPL auto _getMemRayOrig(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t Dim, uint32_t BvhLevel) {
  return (BvhLevel == 0 ? StackPtr->ray0 : StackPtr->ray1).org[Dim];
}
IMPL_ALL_3ARG(_getMemRayOrig, StackPtr, Dim, BvhLevel)

template <typename GenT>
IMPL auto _getMemRayDir(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t Dim, uint32_t BvhLevel) {
  return (BvhLevel == 0 ? StackPtr->ray0 : StackPtr->ray1).dir[Dim];
}
IMPL_ALL_3ARG(_getMemRayDir, StackPtr, Dim, BvhLevel)

template <typename GenT> IMPL auto _getWorldRayDir(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t Dim) {
  return StackPtr->ray0.dir[Dim];
}
IMPL_ALL_2ARG(_getWorldRayDir, StackPtr, Dim)

template <typename GenT> IMPL auto _getRayTMin(RTSAS RTStack2<GenT> *__restrict__ StackPtr) {
  return StackPtr->ray0.tnear;
}
IMPL_ALL_1ARG(_getRayTMin, StackPtr)


template <typename GenT>
IMPL auto _getRayInfo(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t Idx, uint32_t BvhLevel) {
  auto *Ptr = (RTSAS float *)&(BvhLevel == 0 ? StackPtr->ray0 : StackPtr->ray1);
  return Ptr[Idx];
}
IMPL_ALL_3ARG(_getRayInfo, StackPtr, Idx, BvhLevel)

template <typename GenT> IMPL uint16_t _getRayFlagsSync(RTSAS RTStack2<GenT> *__restrict__ StackPtr) {
  return StackPtr->ray0.rayFlags16BitTypeAlias.rayFlags;
}
IMPL_ALL_1ARG(_getRayFlagsSync, StackPtr)

template <typename GenT> IMPL void _setRayFlagsSync(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint16_t Flag) {
  StackPtr->ray0.rayFlags16BitTypeAlias.rayFlags = Flag;
}
IMPL_ALL_2ARG(_setRayFlagsSync, StackPtr, Flag)


template <typename GenT, typename Fn>
IMPL uint64_t fetchPrimLeaf(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed, Fn HandlePtr) {
  uint64_t LeafPtr = Committed ? StackPtr->committedHit.primLeafPtr : StackPtr->potentialHit.primLeafPtr;

  return HandlePtr(LeafPtr);
}

IMPL uint64_t fetchPrimLeaf(RTSAS RTStack2<Xe> *__restrict__ StackPtr, bool Committed) {
  return fetchPrimLeaf(StackPtr, Committed,
                       [](uint64_t LeafPtr) { return hook::bi::canonizePointer(LeafPtr * LeafSize); });
}

template <typename GenT> IMPL uint64_t fetchPrimLeaf(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  return fetchPrimLeaf(StackPtr, Committed, [](uint64_t LeafPtr) { return LeafPtr * LeafSize; });
}

template <typename GenT, typename Fn>
IMPL auto fetchInstanceLeaf(RTSAS RTStack2<GenT> *__restrict__ StackPtr, CallableShaderTypeMD ShaderTy, Fn HandlePtr) {
  uint64_t LeafPtr = StackPtr->potentialHit.instLeafPtr;
  if (ShaderTy == ClosestHit)
    LeafPtr = StackPtr->committedHit.instLeafPtr;
  return (GAS InstanceLeaf<GenT> *)HandlePtr(LeafPtr);
}

template <typename GenT>
IMPL auto fetchInstanceLeaf(RTSAS RTStack2<GenT> *__restrict__ StackPtr, CallableShaderTypeMD ShaderTy) {
  static_assert((uint32_t)MemHit<GenT>::Bits::instLeafPtr == 58);
  return fetchInstanceLeaf(StackPtr, ShaderTy, [](uint64_t LeafPtr) { return LeafPtr * LeafSize; });
}

IMPL auto fetchInstanceLeaf(RTSAS RTStack2<Xe> *__restrict__ StackPtr, CallableShaderTypeMD ShaderTy) {
  return fetchInstanceLeaf(StackPtr, ShaderTy,
                           [](uint64_t LeafPtr) { return hook::bi::canonizePointer(LeafPtr * LeafSize); });
}

template <typename GenT> IMPL uint64_t _getPrimLeaf(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  return fetchPrimLeaf(StackPtr, Committed);
}
IMPL_ALL_2ARG(_getPrimLeaf, StackPtr, Committed)

template <typename GenT>
IMPL auto _getInstanceLeaf(RTSAS RTStack2<GenT> *__restrict__ StackPtr, CallableShaderTypeMD ShaderTy) {
  return fetchInstanceLeaf(StackPtr, ShaderTy);
}
IMPL_ALL_2ARG(_getInstanceLeaf, StackPtr, ShaderTy)

template <typename GenT>
IMPL uint32_t _getInstanceContributionToHitGroupIndex(RTSAS RTStack2<GenT> *__restrict__ StackPtr,
                                                      CallableShaderTypeMD ShaderTy) {
  return fetchInstanceLeaf(StackPtr, ShaderTy)->part0.instanceContributionToHitGroupIndex;
}
IMPL_ALL_2ARG(_getInstanceContributionToHitGroupIndex, StackPtr, ShaderTy)
template <typename GenT> IMPL uint32_t _getRayMask(RTSAS RTStack2<GenT> *__restrict__ StackPtr) {
  return StackPtr->ray0.rayMask;
}
IMPL_ALL_1ARG(_getRayMask, StackPtr)


template <typename RTStackT>
IMPL auto _getLeafNodeSubType(RTSAS RTStack2<RTStackT> *__restrict__ StackPtr, bool Committed) {
  // memhit->leafNodeSubType
  return Committed ? StackPtr->committedHit.leafNodeSubType : StackPtr->potentialHit.leafNodeSubType;
}
IMPL_ALL_2ARG_XE3PLUS(_getLeafNodeSubType, StackPtr, Committed)

template <typename GenT> IMPL bool _isValid(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  auto *Hit = Committed ? &StackPtr->committedHit : &StackPtr->potentialHit;

  return Hit->valid != 0;
}
IMPL_ALL_2ARG(_isValid, StackPtr, Committed)

template <typename GenT> IMPL uint32_t _createLeafType(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  auto *Hit = Committed ? &StackPtr->committedHit : &StackPtr->potentialHit;

  return Hit->leafType;
}
IMPL_ALL_2ARG(_createLeafType, StackPtr, Committed)

template <typename GenT> IMPL bool _getIsFrontFace(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  return Committed ? StackPtr->committedHit.frontFace : StackPtr->potentialHit.frontFace;
}
IMPL_ALL_2ARG(_getIsFrontFace, StackPtr, Committed)

template <typename GenT> IMPL uint32_t _getBvhLevel(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  return Committed ? StackPtr->committedHit.bvhLevel : StackPtr->potentialHit.bvhLevel;
}
IMPL_ALL_2ARG(_getBvhLevel, StackPtr, Committed)

template <typename GenT> IMPL bool _isDoneBitNotSet(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  uint32_t Val = Committed ? StackPtr->committedHit.done : StackPtr->potentialHit.done;
  return Val == 0;
}
IMPL_ALL_2ARG(_isDoneBitNotSet, StackPtr, Committed)

template <typename GenT> IMPL void _setDoneBit(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  if (Committed)
    StackPtr->committedHit.done = 1;
  else
    StackPtr->potentialHit.done = 1;
}
IMPL_ALL_2ARG(_setDoneBit, StackPtr, Committed)

template <typename GenT> IMPL void _setHitValid(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  if (Committed)
    StackPtr->committedHit.valid = 1;
  else
    StackPtr->potentialHit.valid = 1;
}
IMPL_ALL_2ARG(_setHitValid, StackPtr, Committed)

template <typename RTStackT, typename Fn>
IMPL float fetchBaryCentric(RTSAS RTStackT *__restrict__ StackPtr, uint32_t Idx, bool Committed, Fn ConvFn) {
  auto &Hit = Committed ? StackPtr->committedHit : StackPtr->potentialHit;

  return (Idx == 0) ? ConvFn(Hit.u) : ConvFn(Hit.v);
}

template <typename RTStackT>
IMPL float fetchBaryCentric(RTSAS RTStackT *__restrict__ StackPtr, uint32_t Idx, bool Committed) {
  return fetchBaryCentric(StackPtr, Idx, Committed,
                          // Barycentric coordinate in unorm24 (0 is 0.0, 0x00ffffff is 1.0)
                          [](uint32_t bary) { return bary * (1.0f / 0x00ffffff); });
}

IMPL float fetchBaryCentric(RTSAS RTStack2<Xe> *__restrict__ StackPtr, uint32_t Idx, bool Committed) {
  return fetchBaryCentric(StackPtr, Idx, Committed, [](float bary) { return bary; });
}

template <typename GenT>
IMPL float _getHitBaryCentric(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t Idx, bool Committed) {
  return fetchBaryCentric(StackPtr, Idx, Committed);
}
IMPL_ALL_3ARG(_getHitBaryCentric, StackPtr, Idx, Committed)

CREATE_PRIVATE void _writeBaryCentricToStorage_Xe(RTSAS RTStack2<Xe> *__restrict__ StackPtr, SWStackAS float *Storage,
                                                  bool Committed) {
  // Retaining this specialization for now to generate the exact
  // code we want.
  auto *Hit = Committed ? &StackPtr->committedHit.u : &StackPtr->potentialHit.u;

#pragma unroll
  for (uint32_t i = 0; i < 2; i++)
    Storage[i] = Hit[i];
}

template <typename RTStackT>
IMPL void _writeBaryCentricToStorageImpl(RTSAS RTStackT *__restrict__ StackPtr, SWStackAS float *Storage,
                                         bool Committed) {
#pragma unroll
  for (uint32_t i = 0; i < 2; i++)
    Storage[i] = fetchBaryCentric(StackPtr, i, Committed);
}

template <typename RTStackT>
IMPL void _writeBaryCentricToStorage(RTSAS RTStack2<RTStackT> *__restrict__ StackPtr, SWStackAS float *Storage,
                                     bool Committed) {
  return _writeBaryCentricToStorageImpl(StackPtr, Storage, Committed);
}
IMPL_ALL_3ARG_XE3PLUS(_writeBaryCentricToStorage, StackPtr, Storage, Committed)

template <typename GenT>
IMPL float _TransformWorldToObject(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t dim, bool isOrigin,
                                   CallableShaderTypeMD ShaderTy) {
  auto *InstanceLeafPtr = fetchInstanceLeaf(StackPtr, ShaderTy);

  static_assert(offsetof(InstanceLeaf<GenT>, part0.world2obj_vy) ==
                    offsetof(InstanceLeaf<GenT>, part0.world2obj_vx) + sizeof(Vec3f),
                "layout change?");
  static_assert(offsetof(InstanceLeaf<GenT>, part0.world2obj_vz) ==
                    offsetof(InstanceLeaf<GenT>, part0.world2obj_vy) + sizeof(Vec3f),
                "layout change?");
  static_assert(offsetof(InstanceLeaf<GenT>, part0.obj2world_p) ==
                    offsetof(InstanceLeaf<GenT>, part0.world2obj_vz) + sizeof(Vec3f),
                "layout change?");

  auto *MatPtr = &InstanceLeafPtr->part0.world2obj_vx[0];

  float acc = isOrigin ? InstanceLeafPtr->part1.world2obj_p[dim] : 0.f;

  auto *rayInfo = isOrigin ? &StackPtr->ray0.org[0] : &StackPtr->ray0.dir[0];

#pragma unroll
  for (uint32_t i = 0; i < 3; i++)
    acc += MatPtr[dim + i * 3] * rayInfo[i];

  return acc;
}
IMPL_ALL_4ARG(_TransformWorldToObject, StackPtr, dim, isOrigin, ShaderTy)

IMPL bool isHitShader(CallableShaderTypeMD ShaderTy) {
  switch (ShaderTy) {
  case ClosestHit:
  case AnyHit:
  case Intersection:
    return true;
  default:
    return false;
  }
}

template <typename GenT>
IMPL float _getObjWorldAndWorldObj(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t dim, bool ObjToWorld,
                                   CallableShaderTypeMD ShaderTy) {
  // this function can be called when the stack does not encode a valid hit
  // in that case we should avoid dereferencing the instance leaf ptr
  // and return an "identity" matrix
  if (!isHitShader(ShaderTy))
    return dim % 4 == 0 ? 1.0f : 0.0f;

  auto *InstanceLeafPtr = fetchInstanceLeaf(StackPtr, ShaderTy);

  auto *MatPtr = ObjToWorld ? &InstanceLeafPtr->part1.obj2world_vx[dim] : &InstanceLeafPtr->part0.world2obj_vx[dim];
  if (dim >= 9) {
    MatPtr = ObjToWorld ? &InstanceLeafPtr->part0.obj2world_p[dim - 9] : &InstanceLeafPtr->part1.world2obj_p[dim - 9];
  }
  return *MatPtr;
}
IMPL_ALL_4ARG(_getObjWorldAndWorldObj, StackPtr, dim, ObjToWorld, ShaderTy)

template <typename GenT>
IMPL float _getRayTCurrent(RTSAS RTStack2<GenT> *__restrict__ StackPtr, CallableShaderTypeMD ShaderTy) {
  auto *Ptr = &StackPtr->potentialHit.t;
  if (ShaderTy == Miss)
    Ptr = &StackPtr->ray0.tfar;
  if (ShaderTy == ClosestHit)
    Ptr = &StackPtr->committedHit.t;

  return *Ptr;
}
IMPL_ALL_2ARG(_getRayTCurrent, StackPtr, ShaderTy)

template <typename GenT> IMPL float _getHitT(RTSAS RTStack2<GenT> *__restrict__ StackPtr, bool Committed) {
  return Committed ? StackPtr->committedHit.t : StackPtr->potentialHit.t;
}
IMPL_ALL_2ARG(_getHitT, StackPtr, Committed)

template <typename GenT> IMPL void _setHitT(RTSAS RTStack2<GenT> *__restrict__ StackPtr, float t, bool Committed) {
  if (Committed)
    StackPtr->committedHit.t = t;
  else
    StackPtr->potentialHit.t = t;
}
IMPL_ALL_3ARG(_setHitT, StackPtr, t, Committed)

template <typename GenT>
IMPL uint32_t _getInstanceIndex(RTSAS RTStack2<GenT> *__restrict__ StackPtr, CallableShaderTypeMD ShaderTy) {
  // this function can be called when the stack does not encode a valid hit
  // in that case we should avoid dereferencing the instance leaf ptr
  // and return 0
  if (!isHitShader(ShaderTy))
    return 0;

  return fetchInstanceLeaf(StackPtr, ShaderTy)->part1.instanceIndex;
}
IMPL_ALL_2ARG(_getInstanceIndex, StackPtr, ShaderTy)

template <typename GenT>
IMPL uint32_t _getInstanceID(RTSAS RTStack2<GenT> *__restrict__ StackPtr, CallableShaderTypeMD ShaderTy) {
  // this function can be called when the stack does not encode a valid hit
  // in that case we should avoid dereferencing the instance leaf ptr
  // and return 0
  if (!isHitShader(ShaderTy))
    return 0;

  return fetchInstanceLeaf(StackPtr, ShaderTy)->part1.instanceID;
}
IMPL_ALL_2ARG(_getInstanceID, StackPtr, ShaderTy)

template <typename GenT>
IMPL uint32_t _getPrimitiveIndex(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t leafType,
                                 CallableShaderTypeMD ShaderTy) {
  // this function can be called when the stack does not encode a valid hit
  // in that case we should avoid dereferencing the instance leaf ptr
  // and return 0
  if (!isHitShader(ShaderTy))
    return 0;

  bool Committed = ShaderTy == ClosestHit;

  uint64_t LeafPtr = fetchPrimLeaf(StackPtr, Committed);

  // We are interested in only the LSB of leafType
  // because we only check if type is procedural.
  // This is required for RayQueryReturnOptimization, where only 1
  // bit describes the type of committed geometry.

  static_assert(((NODE_TYPE_PROCEDURAL & 1) == 1) && ((NODE_TYPE_QUAD & 1) == 0) && ((NODE_TYPE_MESHLET & 1) == 0),
                "optimized CommittedStatus broken");

  uint32_t primLeafIndex = Committed ? StackPtr->committedHit.primLeafIndex : StackPtr->potentialHit.primLeafIndex;

  uint32_t primIndexDelta = Committed ? StackPtr->committedHit.primIndexDelta : StackPtr->potentialHit.primIndexDelta;

  if ((leafType & 1) == (NODE_TYPE_PROCEDURAL & 1)) {
    auto *ProcLeafPtr = (GAS ProceduralLeaf<GenT> *)LeafPtr;
    return ProcLeafPtr->_primIndex[primLeafIndex];
  } else {
    auto *QuadLeafPtr = (GAS QuadLeaf<GenT> *)LeafPtr;
    return QuadLeafPtr->primIndex0 + primIndexDelta;
  }
}
IMPL_ALL_3ARG(_getPrimitiveIndex, StackPtr, leafType, ShaderTy)


template <typename GenT>
IMPL uint32_t _getGeometryIndex(RTSAS RTStack2<GenT> *__restrict__ StackPtr, uint32_t leafType,
                                CallableShaderTypeMD ShaderTy) {
  // this function can be called when the stack does not encode a valid hit
  // in that case we should avoid dereferencing the instance leaf ptr
  // and return 0
  if (!isHitShader(ShaderTy))
    return 0;

  uint64_t LeafPtr = fetchPrimLeaf(StackPtr, ShaderTy == ClosestHit);

  // We are interested in only the LSB of leafType
  // because we only check if type is procedural.
  // This is required for RayQueryReturnOptimization, where only 1
  // bit describes the type of committed geometry.

  static_assert(((NODE_TYPE_PROCEDURAL & 1) == 1) && ((NODE_TYPE_QUAD & 1) == 0) && ((NODE_TYPE_MESHLET & 1) == 0),
                "optimized CommittedStatus broken");

  if ((leafType & 1) == (NODE_TYPE_PROCEDURAL & 1)) {
    auto *ProcLeafPtr = (GAS ProceduralLeaf<GenT> *)LeafPtr;
    return ProcLeafPtr->leafDesc.geomIndex;
  } else {
    auto *QuadLeafPtr = (GAS QuadLeaf<GenT> *)LeafPtr;
    return QuadLeafPtr->leafDesc.geomIndex;
  }
}
IMPL_ALL_3ARG(_getGeometryIndex, StackPtr, leafType, ShaderTy)
CREATE_PRIVATE void _createPotentialHit2CommittedHit_Xe(RTSAS RTStack2<Xe> *__restrict__ StackPtr) {
  auto &CH = StackPtr->committedHit;
  auto &PH = StackPtr->potentialHit;

  CH.t = PH.t;
  CH.u = PH.u;
  CH.v = PH.v;

  CH.primIndexDelta = PH.primIndexDelta;
  CH.valid = PH.valid;
  CH.leafType = PH.leafType;
  CH.primLeafIndex = PH.primLeafIndex;
  CH.bvhLevel = PH.bvhLevel;
  CH.frontFace = PH.frontFace;
  CH.done = PH.done;
  CH.pad0 = PH.pad0;

  CH.primLeafPtr = PH.primLeafPtr;
  CH.hitGroupRecPtr0 = PH.hitGroupRecPtr0;

  CH.instLeafPtr = PH.instLeafPtr;
  CH.hitGroupRecPtr1 = PH.hitGroupRecPtr1;
}

template <typename RTStackT> IMPL void _createPotentialHit2CommittedHitImpl(RTSAS RTStackT *__restrict__ StackPtr) {
  // TODO: show that we can move the Xe specialization to this without any
  // losses.
  auto *CH = (RTSAS uint32_t *)&StackPtr->committedHit;
  auto *PH = (RTSAS uint32_t *)&StackPtr->potentialHit;

  for (uint32_t i = 0; i < sizeof(MemHit<Xe3>) / 4; i++)
    CH[i] = PH[i];
}

template <typename RTStackT>
IMPL void _createPotentialHit2CommittedHit(RTSAS RTStack2<RTStackT> *__restrict__ StackPtr) {
  return _createPotentialHit2CommittedHitImpl(StackPtr);
}
IMPL_ALL_1ARG_XE3PLUS(_createPotentialHit2CommittedHit, StackPtr)

CREATE_PRIVATE void _createTraceRayInlinePrologue_Xe(RTSAS RTStack2<Xe> *__restrict__ StackPtr, _float8 RayInfo,
                                                     uint64_t RootNodePtr, uint32_t RayFlags,
                                                     uint32_t InstanceInclusionMask, uint32_t ComparisonValue,
                                                     float TMax, bool updateFlags, bool initialDoneBitValue) {
  *((RTSAS _float8 *)&StackPtr->ray0.org) = RayInfo;

  auto &ray0 = StackPtr->ray0;

  if (updateFlags)
    RayFlags |= ray0.rayFlags16BitTypeAlias.rayFlags;

  ray0.rootNodePtr = RootNodePtr;
  ray0.rayFlags = RayFlags;

  if (hook::bi::supportStochasticLod()) {
    ray0.hitGroupSRBasePtr = 0;
    ray0.hitGroupSRStride = 0;
    ray0.missSRPtr = 0;
    ray0.pad = 0;
    ray0.ComparisonValue = ComparisonValue;
    ray0.shaderIndexMultiplier = 0;
  }

  ray0.instLeafPtr = 0;
  ray0.rayMask = InstanceInclusionMask;
  ray0.pad2 = 0;

  auto &CH = StackPtr->committedHit;
  auto &PH = StackPtr->potentialHit;

  CH.t = TMax;
  CH.u = 0.0f;
  CH.v = 0.0f;

  CH.primIndexDelta = 0;
  CH.valid = 0;
  CH.leafType = 0;
  CH.primLeafIndex = 0;
  CH.bvhLevel = 0;
  CH.frontFace = 0;
  CH.done = 0;
  CH.pad0 = 0;

  PH.primIndexDelta = 0;
  PH.valid = 0;
  PH.leafType = 0;
  PH.primLeafIndex = 0;
  PH.bvhLevel = 0;
  PH.frontFace = 0;
  PH.done = initialDoneBitValue;
  PH.pad0 = 0;
}

template <typename RTStackT>
IMPL void _createTraceRayInlinePrologue(RTSAS RTStackT *__restrict__ StackPtr, _float8 RayInfo, uint64_t RootNodePtr,
                                        uint32_t RayFlags, uint32_t InstanceInclusionMask, uint32_t ComparisonValue,
                                        float TMax, bool updateFlags, bool initialDoneBitValue) {
  *((RTSAS _float8 *)&StackPtr->ray0.org) = RayInfo;

  auto &ray0 = StackPtr->ray0;

  if (updateFlags)
    RayFlags |= ray0.rayFlags16BitTypeAlias.rayFlags;

  ray0.rootNodePtr = RootNodePtr;
  ray0.instLeafPtr = 0;

  ray0.rayFlags = RayFlags;
  ray0.rayMask = InstanceInclusionMask;
  ray0.ComparisonValue = ComparisonValue;
  ray0.pad1 = 0;

  ray0.hitGroupIndex = 0;

  ray0.missShaderIndex = 0;
  ray0.shaderIndexMultiplier = 0;
  ray0.pad2 = 0;
  ray0.internalRayFlags = 0;

  ray0.time = 0.0f;

  auto &CH = StackPtr->committedHit;
  auto &PH = StackPtr->potentialHit;

  CH.t = TMax;

  CH.primIndexDelta = 0;
  CH.pad1 = 0;
  CH.leafNodeSubType = 0;
  CH.valid = 0;
  CH.leafType = 0;
  CH.primLeafIndex = 0;
  CH.bvhLevel = 0;
  CH.frontFace = 0;
  CH.done = 0;
  CH.needSWSTOC = 0;
  CH.reserved = 0;

  PH.primIndexDelta = 0;
  PH.pad1 = 0;
  PH.leafNodeSubType = 0;
  PH.valid = 0;
  PH.leafType = 0;
  PH.primLeafIndex = 0;
  PH.bvhLevel = 0;
  PH.frontFace = 0;
  PH.done = initialDoneBitValue;
  PH.needSWSTOC = 0;
  PH.reserved = 0;
}

CREATE_PRIVATE void _createTraceRayInlinePrologue_Xe3(RTSAS RTStack2<Xe3> *__restrict__ StackPtr, _float8 RayInfo,
                                                      uint64_t RootNodePtr, uint32_t RayFlags,
                                                      uint32_t InstanceInclusionMask, uint32_t ComparisonValue,
                                                      float TMax, bool updateFlags, bool initialDoneBitValue) {
  _createTraceRayInlinePrologue(StackPtr, RayInfo, RootNodePtr, RayFlags, InstanceInclusionMask, ComparisonValue, TMax,
                                updateFlags, initialDoneBitValue);
}

CREATE_PRIVATE void _createTraceRayInlinePrologue_Xe3PEff64(RTSAS RTStack2<Xe3PEff64> *__restrict__ StackPtr,
                                                            _float8 RayInfo, uint64_t RootNodePtr, uint32_t RayFlags,
                                                            uint32_t InstanceInclusionMask, uint32_t ComparisonValue,
                                                            float TMax, bool updateFlags, bool initialDoneBitValue) {
  _createTraceRayInlinePrologue(StackPtr, RayInfo, RootNodePtr, RayFlags, InstanceInclusionMask, ComparisonValue, TMax,
                                updateFlags, initialDoneBitValue);
}


CREATE_PRIVATE void _emitSingleRQMemRayWrite_Xe(RTSAS RTStack2<Xe> *__restrict__ HWStackPtr,
                                                RTShadowAS RTStack2<Xe> *__restrict__ SMStackPtr,
                                                bool singleRQProceed) {
  // copy ray info
  auto *Dst = &HWStackPtr->ray0.org[0];
  auto *Src = &SMStackPtr->ray0.org[0];

#pragma unroll
  for (uint32_t i = 0; i < 8; i++)
    Dst[i] = Src[i];

  auto *SMRay = (RTShadowAS uint32_t *)&Src[8];

  _uint8 result;
  result.s0 = SMRay[0];
  result.s1 = SMRay[1];
  if (singleRQProceed) {
    result.s2 = 0;
    result.s3 = 0;
    result.s4 = 0;
    result.s5 = 0;
  } else {
    result.s2 = SMRay[2];
    result.s3 = SMRay[3];
    result.s4 = SMRay[4];
    result.s5 = SMRay[5];
  }
  result.s6 = SMRay[6];
  result.s7 = SMRay[7];

  *((RTSAS _uint8 *)&Dst[8]) = result;
}

template <typename RTStackT>
IMPL void _emitSingleRQMemRayWriteImpl(RTSAS RTStackT *__restrict__ HWStackPtr,
                                       RTShadowAS RTStackT *__restrict__ SMStackPtr) {
  // copy ray info
  auto *Dst = &HWStackPtr->ray0.org[0];
  auto *Src = &SMStackPtr->ray0.org[0];

#pragma unroll
  for (uint32_t i = 0; i < 8; i++)
    Dst[i] = Src[i];

  auto *SMRay = (RTShadowAS uint32_t *)&Src[8];

  _uint8 result;
  result.s0 = SMRay[0];
  result.s1 = SMRay[1];
  result.s2 = 0;
  result.s3 = 0;
  result.s4 = SMRay[4];
  result.s5 = 0;
  result.s6 = 0;
  result.s7 = 0;

  *((RTSAS _uint8 *)&Dst[8]) = result;
}

template <typename RTStackT>
IMPL void _emitSingleRQMemRayWrite(RTSAS RTStack2<RTStackT> *__restrict__ HWStackPtr,
                                   RTShadowAS RTStack2<RTStackT> *__restrict__ SMStackPtr) {
  _emitSingleRQMemRayWriteImpl(HWStackPtr, SMStackPtr);
}
IMPL_ALL_2ARG_XE3PLUS(_emitSingleRQMemRayWrite, HWStackPtr, SMStackPtr)

template <typename RTStackT>
IMPL void _copyMemHitInProceedImpl(RTSAS RTStack2<RTStackT> *__restrict__ HWStackPtr,
                                   RTShadowAS RTStack2<RTStackT> *__restrict__ SMStackPtr, bool singleRQProceed) {
  // copy first 16 bytes
  auto *SMCH = (RTShadowAS uint32_t *)&SMStackPtr->committedHit;
  _uint8 CHResult;
  if (singleRQProceed) {
    CHResult.s0 = 0;
    CHResult.s1 = 0;
    CHResult.s2 = 0;
    CHResult.s4 = 0;
    CHResult.s5 = 0;
    CHResult.s6 = 0;
    CHResult.s7 = 0;
  } else {
    CHResult.s0 = SMCH[0];
    CHResult.s1 = SMCH[1];
    CHResult.s2 = SMCH[2];
    CHResult.s4 = SMCH[4];
    CHResult.s5 = SMCH[5];
    CHResult.s6 = SMCH[6];
    CHResult.s7 = SMCH[7];
  }
  CHResult.s3 = SMCH[3];
  *((RTSAS _uint8 *)&HWStackPtr->committedHit) = CHResult;

  auto *SMPH = (RTShadowAS uint32_t *)&SMStackPtr->potentialHit;
  _uint8 PHResult;
  PHResult.s0 = SMPH[0];
  if (singleRQProceed) {
    PHResult.s1 = 0;
    PHResult.s2 = 0;
    PHResult.s4 = 0;
    PHResult.s5 = 0;
    PHResult.s6 = 0;
    PHResult.s7 = 0;
  } else {
    PHResult.s1 = SMPH[1];
    PHResult.s2 = SMPH[2];
    PHResult.s4 = SMPH[4];
    PHResult.s5 = SMPH[5];
    PHResult.s6 = SMPH[6];
    PHResult.s7 = SMPH[7];
  }
  // HW will only reset the done bit to 0.  Prior to the sync trace ray,
  // we set the bit and HW will set it to 0 if there is more to do.
  PHResult.s3 = SMPH[3] | (1 << (uint32_t)MemHit<RTStackT>::Offset::done);

  *((RTSAS _uint8 *)&HWStackPtr->potentialHit) = PHResult;
}

template <typename RTStackT>
IMPL void _copyMemHitInProceed(RTSAS RTStack2<RTStackT> *__restrict__ HWStackPtr,
                               RTShadowAS RTStack2<RTStackT> *__restrict__ SMStackPtr, bool singleRQProceed) {
  _copyMemHitInProceedImpl(HWStackPtr, SMStackPtr, singleRQProceed);
}
IMPL_ALL_3ARG(_copyMemHitInProceed, HWStackPtr, SMStackPtr, singleRQProceed)

template <typename GenT>
IMPL bool _syncStackToShadowMemory(RTSAS RTStack2<GenT> *__restrict__ HWStackPtr,
                                   RTShadowAS RTStack2<GenT> *__restrict__ SMStackPtr, uint32_t ProceedReturnVal,
                                   uint32_t *ShadowMemRTCtrlPtr) {
  if (SMStackPtr->potentialHit.done == 0) {
    hook::bi::createReadSyncTraceRay(ProceedReturnVal);
    *ShadowMemRTCtrlPtr = (uint32_t)TraceRayCtrl::TRACE_RAY_CONTINUE;

    auto *HWPH = (RTSAS uint32_t *)&HWStackPtr->potentialHit;
    auto *SMPH = (RTShadowAS uint32_t *)&SMStackPtr->potentialHit;

// SMstack->potentialHit = HWstack->potentialHit
#pragma unroll
    for (uint32_t i = 0; i < sizeof(MemHit<GenT>) / 4; i++)
      SMPH[i] = HWPH[i];

    auto *HWRay = (RTSAS float *)&HWStackPtr->ray1;
    auto *SMRay = (RTShadowAS float *)&SMStackPtr->ray1;

// SMstack->ray[BOTTOM_LEVEL_BVH] = HWstack->ray[BOTTOM_LEVEL_BVH]
// copy ray origin and direction
#pragma unroll
    for (uint32_t i = 0; i < 6; i++)
      SMRay[i] = HWRay[i];

    bool isValid = false;
    if (hook::bi::isRayQueryReturnOptimizationEnabled()) {
      RayQueryReturnData Data{ProceedReturnVal};

      auto &PH = SMStackPtr->potentialHit;
      // Done bit must be inverted first, as it is set differently
      // depending on whether is is read from the the stack or from
      // return value.
      // If it is read from the stack, 1 means the traversal is completed.
      // But if read from return value 1 means the traversal is not completed.
      // Inverting it allows simpler handling with no divergent paths
      // for optimization on and off.
      PH.done = Data.PROCEED_FURTHER == 0;
      PH.leafType =
          Data.candidateType == CANDIDATE_TYPE::CANDIDATE_NON_OPAQUE_TRIANGLE ? NODE_TYPE_QUAD : NODE_TYPE_PROCEDURAL;
      isValid = Data.committedStatus != COMMITTED_STATUS::COMMITTED_NOTHING;
    } else {
      isValid = HWStackPtr->committedHit.valid;
    }

    bool NotDone = (SMStackPtr->potentialHit.done == 0);

    if (isValid) {
      auto *HWPH = (RTSAS uint32_t *)&HWStackPtr->committedHit;
      auto *SMPH = (RTShadowAS uint32_t *)&SMStackPtr->committedHit;

// SMstack->CommittedHit = HWstack->CommittedHit
#pragma unroll
      for (uint32_t i = 0; i < sizeof(MemHit<GenT>) / 4; i++)
        SMPH[i] = HWPH[i];

      if (hook::bi::isRayQueryReturnOptimizationEnabled()) {
        auto &CH = SMStackPtr->committedHit;
        RayQueryReturnData Data{ProceedReturnVal};

        // Committed Status is placed inside leafType bitfield.
        // It must be properly encoded, so it could be latter correctly read.
        // If there was a hit, the data returned by RayQuery contains two values:
        // 1 - COMMITTED_TRIANGLE_HIT
        // 2 - COMMITTED_PROCEDURAL_PRIMITIVE_HIT
        // At this point one is subtracted from it because LowerCommittedStatus adds it
        // back to get Committed type from NodeType.
        // See (_getCommittedStatus*).
        CH.valid = isValid;
        CH.leafType =
            Data.committedStatus == COMMITTED_STATUS::COMMITTED_TRIANGLE_HIT ? NODE_TYPE_QUAD : NODE_TYPE_PROCEDURAL;
      }
    }

    return NotDone;
  }

  return false;
}
IMPL_ALL_4ARG(_syncStackToShadowMemory, HWStackPtr, SMStackPtr, ProceedReturnVal, ShadowMemRTCtrlPtr)

template <typename GenT> IMPL COMMITTED_STATUS _getCommittedStatus(RTSAS RTStack2<GenT> *__restrict__ SMStackPtr) {
  // we are interested in only the LSB of leafType
  static_assert(((NODE_TYPE_PROCEDURAL & 1) == 1) && ((NODE_TYPE_QUAD & 1) == 0) && ((NODE_TYPE_MESHLET & 1) == 0),
                "optimized CommittedStatus broken");
  static_assert((COMMITTED_NOTHING == 0) && (COMMITTED_TRIANGLE_HIT == 1) && (COMMITTED_PROCEDURAL_PRIMITIVE_HIT == 2),
                "enum changed?");

  auto &CH = SMStackPtr->committedHit;
  uint32_t valid = 1 + uint32_t(CH.leafType & 1);
  return CH.valid ? COMMITTED_STATUS(valid) : COMMITTED_NOTHING;
}
IMPL_ALL_1ARG(_getCommittedStatus, SMStackPtr)

template <typename GenT> IMPL CANDIDATE_TYPE _getCandidateType(RTSAS RTStack2<GenT> *__restrict__ SMStackPtr) {
  // we are interested in only the LSB of leafType
  static_assert(((NODE_TYPE_PROCEDURAL & 1) == CANDIDATE_PROCEDURAL_PRIMITIVE) &&
                    ((NODE_TYPE_QUAD & 1) == CANDIDATE_NON_OPAQUE_TRIANGLE) &&
                    ((NODE_TYPE_MESHLET & 1) == CANDIDATE_NON_OPAQUE_TRIANGLE),
                "optimized CandidateType broken");

  auto &PH = SMStackPtr->potentialHit;
  return CANDIDATE_TYPE(PH.leafType & 1);
}
IMPL_ALL_1ARG(_getCandidateType, SMStackPtr)

CREATE_PRIVATE void _commitProceduralPrimitiveHit_Xe(RTSAS RTStack2<Xe> *__restrict__ SMStackPtr, float THit) {
  auto &CH = SMStackPtr->committedHit;
  auto &PH = SMStackPtr->potentialHit;

  CH.t = THit;
  PH.t = THit;

  CH.u = 0.f;
  CH.v = 0.f;

  PH.valid = true;
  CH.valid = true;

  CH.primLeafPtr = PH.primLeafPtr;
  CH.hitGroupRecPtr0 = PH.hitGroupRecPtr0;

  CH.instLeafPtr = PH.instLeafPtr;
  CH.hitGroupRecPtr1 = PH.hitGroupRecPtr1;
}

template <typename RTStackT>
IMPL void _commitProceduralPrimitiveHitImpl(RTSAS RTStackT *__restrict__ SMStackPtr, float THit) {
  auto &CH = SMStackPtr->committedHit;
  auto &PH = SMStackPtr->potentialHit;

  CH.t = THit;
  PH.t = THit;

  CH.u = 0;
  CH.hitGroupIndex0 = 0;
  CH.v = 0;
  CH.hitGroupIndex1 = 0;

  PH.valid = true;
  CH.valid = true;

  CH.primLeafPtr = PH.primLeafPtr;
  CH.hitGroupIndex2 = 0;

  CH.instLeafPtr = PH.instLeafPtr;
  CH.hitGroupIndex3 = 0;
}

template <typename RTStackT>
IMPL void _commitProceduralPrimitiveHit(RTSAS RTStack2<RTStackT> *__restrict__ SMStackPtr, float THit) {
  return _commitProceduralPrimitiveHitImpl(SMStackPtr, THit);
}
IMPL_ALL_2ARG_XE3PLUS(_commitProceduralPrimitiveHit, StackPtr, ShaderTy)

IMPL uint32_t emitStateRegID(uint32_t Start, uint32_t End) {
  return (hook::bi::getSr0_0() & BITMASK_RANGE(Start, End)) >> Start;
}

// compute the sync stack IDs for rayquery
CREATE_PRIVATE uint32_t _getSyncStackID_Xe() {
  // SyncStackID = (EUID[3:0] << 7) | (ThreadID[2:0] << 4) | SIMDLaneID[3:0]; // With fused EUs (e.g. DG2)

  // Note: bits sr0.0[7:4] in DG2, PVC are not the actual EUID within a DSS:

  // To calculate the true EUID you need to replace bit 6 with
  // the subslice ID (bit 8 of sr0.0).
  //
  // sr0.0 layout:
  //
  // DG2:
  // bits [5:4] = EUID within a row (EUID[1:0])
  // bit     6  = Must be zero
  // bit     7  = row ID            (EUID[3])
  // bit     8  = subslice ID       (EUID[2])
  //
  // EUID[3:0]     = sr0.0[7:8:5:4]
  // ThreadID[2:0] = sr0.0[2:0]
  auto sr0 = [](uint32_t Start, uint32_t End) { return emitStateRegID(Start, End); };

  uint32_t EUID = ((sr0(4, 7) & ~0b100) << 7) | sr0(8, 8) << 9;

  uint32_t ThreadID = sr0(0, 2);

  uint32_t SIMDLaneID = hook::bi::get32BitLaneIDReplicate();

  return (EUID << 0 | ThreadID << 4 | SIMDLaneID << 0);
}

CREATE_PRIVATE uint32_t _getSyncStackID_Xe_HPC() {
  // SyncStackID = (EUID[2:0] << 8) | (ThreadID[2:0] << 4) | SIMDLaneID[3:0]; // With natively wide EUs (e.g. PVC)
  //
  // PVC:
  // bits [5:4] = EUID within a row (EUID [1:0])
  // bit     6  = Must be zero
  // bit     7  = Must be zero (no row ID)
  // bit     8  = EUID[2]
  //
  // EUID[2:0]     = sr0.0[8:5:4]
  // ThreadID[2:0] = sr0.0[2:0]
  auto sr0 = [](uint32_t Start, uint32_t End) { return emitStateRegID(Start, End); };

  uint32_t EUID = (sr0(4, 5) << 8) | sr0(8, 8) << 10;

  uint32_t ThreadID = sr0(0, 2);

  uint32_t SIMDLaneID = hook::bi::get32BitLaneIDReplicate();

  return (EUID << 0 | ThreadID << 4 | SIMDLaneID << 0);
}

CREATE_PRIVATE uint32_t _getSyncStackID_Xe2() {
  // SyncStackID = (EUID[2:0] << 8) | (ThreadID[2:0] << 4) | SIMDLaneID[3:0]; // Xe2
  auto sr0 = [](uint32_t Start, uint32_t End) { return emitStateRegID(Start, End); };

  uint32_t EUID = sr0(4, 6);

  uint32_t ThreadID = sr0(0, 2);

  uint32_t SIMDLaneID = hook::bi::get32BitLaneIDReplicate();

  return (EUID << 8 | ThreadID << 4 | SIMDLaneID << 0);
}

CREATE_PRIVATE uint32_t _getSyncStackID_Xe3() {
  // SyncStackID = (EUID[2:0] << 8) | (ThreadID[3:0] << 4) | SIMDLaneID[3:0]; // Xe3
  auto sr0 = [](uint32_t Start, uint32_t End) { return emitStateRegID(Start, End); };

  uint32_t EUID = sr0(4, 6);

  uint32_t ThreadID = sr0(0, 3);

  uint32_t SIMDLaneID = hook::bi::get32BitLaneIDReplicate();

  return (EUID << 8 | ThreadID << 4 | SIMDLaneID << 0);
}

CREATE_PRIVATE uint32_t _getSyncStackID_Xe3p() {
  // SyncStackID = (EUID[2:0] << 8) | (ThreadID[3:0] << 4) | SIMDLaneID[3:0]; // Xe3p
  auto sr0 = [](uint32_t Start, uint32_t End) { return emitStateRegID(Start, End); };

  uint32_t EUID = sr0(4, 6);

  uint32_t ThreadID = sr0(0, 3);

  uint32_t SIMDLaneID = hook::bi::get32BitLaneID();

  return (EUID << 8 | ThreadID << 4 | SIMDLaneID << 0);
}

CREATE_PRIVATE uint32_t _getSyncStackID_Xe3pEff64() {
  // SyncStackID = (EUID[2:0] * maxThreadsPerEU * maxSIMDSize) +
  //               (ThreadID[3:0] *maxSIMDSize) +
  //                SIMDLaneID[3:0]; // Xe3pv2
  auto sr0 = [](uint32_t Start, uint32_t End) { return emitStateRegID(Start, End); };

  uint32_t EUID = sr0(4, 6);

  uint32_t ThreadID = sr0(0, 3);

  uint32_t SIMDLaneID = hook::bi::get32BitLaneID();

  return ((EUID * hook::bi::getMaxThreadsPerEU() * hook::bi::getMaxSimdSize()) +
          (ThreadID * hook::bi::getMaxSimdSize()) + SIMDLaneID);
}



template <typename GenT>
IMPL RTSAS void *_getHitAddress(RTSAS RTStack2<GenT> *__restrict__ HWStackPtr, bool Committed) {
  return Committed ? &HWStackPtr->committedHit : &HWStackPtr->potentialHit;
}
IMPL_ALL_2ARG(_getHitAddress, HWStackPtr, Committed)

