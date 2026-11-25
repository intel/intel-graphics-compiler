/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Languages/OpenCL/IBiF_SPIRV_Utils.h"

// Named Barrier

void __intel_getInitializedNamedBarrierArray(local uint *id) {
  *id = 0;
  __spirv_ControlBarrier(Workgroup, 0, SequentiallyConsistent | WorkgroupMemory);
}

bool __intel_is_first_work_group_item(void);

local __namedBarrier *
__builtin_spirv_OpNamedBarrierInitialize_i32_p3__namedBarrier_p3i32(int SubGroupCount, local __namedBarrier *nb_array,
                                                                    local uint *id) {
  local __namedBarrier *NB = &nb_array[*id];
  NB->count = SubGroupCount;
  NB->orig_count = SubGroupCount;
  NB->inc = 0;
  __spirv_ControlBarrier(Workgroup, 0, SequentiallyConsistent | WorkgroupMemory);
  if (__intel_is_first_work_group_item()) {
    (*id)++;
  }
  __spirv_ControlBarrier(Workgroup, 0, SequentiallyConsistent | WorkgroupMemory);
  return NB;
}

static INLINE OVERLOADABLE uint AtomicCompareExchange(local uint *Pointer, uint Scope, uint Equal, uint Unequal,
                                                      uint Value, uint Comparator) {
  return __spirv_AtomicCompareExchange((local int *)Pointer, Scope, Equal, Unequal, Value, Comparator);
}

static INLINE uint SubgroupLocalId() { return __spirv_BuiltInSubgroupLocalInvocationId(); }

static INLINE OVERLOADABLE uint AtomicLoad(local uint *Pointer, uint Scope, uint Semantics) {
  return __spirv_AtomicLoad((local int *)Pointer, Scope, Semantics);
}

static INLINE OVERLOADABLE void AtomicStore(local uint *Pointer, uint Scope, uint Semantics, uint Value) {
  __spirv_AtomicStore((local int *)Pointer, Scope, Semantics, Value);
}

static INLINE OVERLOADABLE uint AtomicInc(local uint *Pointer, uint Scope, uint Semantics) {
  return __spirv_AtomicIIncrement((local int *)Pointer, Scope, Semantics);
}

static INLINE uint Broadcast(uint Execution, uint Value, uint3 LocalId) {
  return __spirv_GroupBroadcast(Execution, as_int(Value), LocalId);
}

static INLINE OVERLOADABLE uint SubgroupAtomicCompareExchange(local uint *Pointer, uint Scope, uint Equal, uint Unequal,
                                                              uint Value, uint Comparator) {
  uint result = 0;
  if (SubgroupLocalId() == 0)
    result = AtomicCompareExchange((volatile local uint *)Pointer, Scope, Equal, Unequal, Value, Comparator);
  result = Broadcast(Subgroup, result, (uint3)0);
  return result;
}

static INLINE OVERLOADABLE uint SubgroupAtomicInc(local uint *Pointer, uint Scope, uint Semantics) {
  uint result = 0;
  if (SubgroupLocalId() == 0)
    result = AtomicInc((volatile local uint *)Pointer, Scope, Semantics);
  result = Broadcast(Subgroup, result, (uint3)0);
  return result;
}

static void MemoryBarrier(Scope_t Memory, uint Semantics) { __spirv_MemoryBarrier(Memory, Semantics); }

void __builtin_spirv_OpMemoryNamedBarrier_p3__namedBarrier_i32_i32(local __namedBarrier *NB, Scope_t Memory,
                                                                   uint Semantics) {
  const uint AtomSema = SequentiallyConsistent | WorkgroupMemory;
  while (1) {
    const uint cnt = AtomicLoad(&NB->count, Workgroup, AtomSema);
    if (cnt > 0) {
      uint before = SubgroupAtomicCompareExchange(&NB->count, Workgroup, AtomSema, AtomSema, cnt - 1, cnt);
      if (before == cnt) {
        break;
      }
    }
  }

  while (AtomicLoad(&NB->count, Workgroup, AtomSema) > 0)
    ;
  MemoryBarrier(Memory, Semantics);
  uint inc = SubgroupAtomicInc(&NB->inc, Workgroup, AtomSema);
  if (inc == ((NB->orig_count) - 1)) {
    AtomicStore(&NB->inc, Workgroup, AtomSema, 0);
    AtomicStore(&NB->count, Workgroup, AtomSema, NB->orig_count);
  }
}
void __builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32(local __namedBarrier *barrier,
                                                                         cl_mem_fence_flags flags) {
  __builtin_spirv_OpMemoryNamedBarrier_p3__namedBarrier_i32_i32(barrier, Workgroup,
                                                                AcquireRelease | get_spirv_mem_fence(flags));
}

void __builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32_i32(local __namedBarrier *barrier,
                                                                             cl_mem_fence_flags flags,
                                                                             memory_scope scope) {
  __builtin_spirv_OpMemoryNamedBarrier_p3__namedBarrier_i32_i32(barrier, get_spirv_mem_scope(scope),
                                                                AcquireRelease | get_spirv_mem_fence(flags));
}
