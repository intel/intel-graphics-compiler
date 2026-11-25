/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// EmitVISAPass support for __builtin_IB_memfence/__builtin_IB_typedmemfence requires some arguments to
// be constants as those are used to prepare a message descriptor, so must be known at compile time.
// To assure that all the arguments are constants for O0 path, there is a special function
// marked with __attribute__((optnone)) which implements seperate call instruction created for each
// arguments configuration.

// MEMFENCE IMPLEMENTATION

// Below choice of some HW scopes is explained.
// 1. TODO: CrossDevice should be mapped to LSC_FS_SYSTEM_RELEASE or LSC_FS_SYSTEM_ACQUIRE.
//     sycl::memory_scope::system and OpenCL's memory_scope_all_svm_devices are
//     lowered to SPIR-V CrossDevice. SYCL SPEC 2020 for system scope:
//     "applies to any work-item or host thread in the system that is currently permitted to access
//     the memory allocation containing the referenced object".
// 2. Subgroup and Invocation --> LSC_FS_THREAD_GROUP:
//     our HW spec doesn’t have corresponding scope for Invocation or Subgroup, hence mapping to
//     lowest possible scope
// 3. Region is a SW concept. There is no corresponding HW scope now. Mapping to GPU level.
#define CONVERT_SCOPE_SPIRV_TO_VISA(scope)                                                                             \
  ((scope) == CrossDevice ? LSC_FS_GPU                                                                                 \
   : (scope) == Device    ? LSC_FS_GPU                                                                                 \
   : (scope) == Region    ? LSC_FS_GPU                                                                                 \
                          : LSC_FS_THREAD_GROUP)

// Adding scope parameter to 'MEMFENCE_IF' macro causes huge compilation time increase
// due to number of calls to __builtin_IB_memfence inside __intel_memfence_optnone increases from
// 16 (2 * 2 * 2 * 2) to <number of possible scope values> * 16.
// So for optnone version keep old behavior: use GPU scope for global memory fence
// and GROUP scope for local memory fence.
void OPTNONE __intel_memfence_optnone(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1) {
#define MEMFENCE_IF(V1, V5, V6, V7)                                                                                    \
  if (flushRW == V1 && isGlobal == V5 && invalidateL1 == V6 && evictL1 == V7) {                                        \
    LSC_FS lsc_scope = (V5 ? LSC_FS_GPU : LSC_FS_THREAD_GROUP);                                                        \
    __builtin_IB_memfence(true, V1, false, false, false, V5, V6, V7, lsc_scope);                                       \
  } else

// Generate combinations for all MEMFENCE_IF cases, e.g.:
// true, true, true
// true, true, false etc.
#define MF_L3(...) MF_L2(__VA_ARGS__, false) MF_L2(__VA_ARGS__, true)
#define MF_L2(...) MF_L1(__VA_ARGS__, false) MF_L1(__VA_ARGS__, true)
#define MF_L1(...) MEMFENCE_IF(__VA_ARGS__, false) MEMFENCE_IF(__VA_ARGS__, true)
  MF_L3(false)
  MF_L3(true) {}

#undef MEMFENCE_IF
#undef MF_L3
#undef MF_L2
#undef MF_L1
}
void __intel_memfence(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1, Scope_t scope) {
  LSC_FS lsc_scope = CONVERT_SCOPE_SPIRV_TO_VISA(scope);
  __builtin_IB_memfence(true, flushRW, false, false, false, isGlobal, invalidateL1, evictL1, lsc_scope);
}

void __intel_memfence_handler(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1, Scope_t scope) {
  if (BIF_FLAG_CTRL_GET(OptDisable))
    __intel_memfence_optnone(flushRW, isGlobal, invalidateL1, evictL1);
  else
    __intel_memfence(flushRW, isGlobal, invalidateL1, evictL1, scope);
}

// TYPEDMEMFENCE IMPLEMENTATION

void OPTNONE __intel_typedmemfence_optnone(bool invalidateL1) {
  if (invalidateL1)
    __builtin_IB_typedmemfence(true);
  else
    __builtin_IB_typedmemfence(false);
}

void __intel_typedmemfence(bool invalidateL1) { __builtin_IB_typedmemfence(invalidateL1); }

void __intel_typedmemfence_handler(bool invalidateL1) {
  if (BIF_FLAG_CTRL_GET(OptDisable))
    __intel_typedmemfence_optnone(invalidateL1);
  else
    __intel_typedmemfence(invalidateL1);
}

#include "include/atomic_work_item_fence.h"

void __attribute__((overloadable)) __spirv_MemoryBarrier(int Memory, int Semantics) {
  __intel_atomic_work_item_fence(Memory, Semantics);
}
