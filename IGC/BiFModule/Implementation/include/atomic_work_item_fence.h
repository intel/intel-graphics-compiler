/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __ATOMIC_WORK_ITEM_FENCE_H__
#define __ATOMIC_WORK_ITEM_FENCE_H__

#include "atomic_fence_impl.h"

void __intel_typedmemfence_handler(bool invalidateL1);

static void __intel_atomic_work_item_fence(Scope_t Memory, uint Semantics) {
  bool fence = Semantics & (Acquire | Release | AcquireRelease | SequentiallyConsistent);

  bool invalidateL1 = Semantics & (Acquire | AcquireRelease | SequentiallyConsistent);
  bool evictL1 = Semantics & (Release | AcquireRelease | SequentiallyConsistent);

  // We always need to 'fence' image memory (aka, flush caches, drain pipelines)
  fence |= (Semantics & ImageMemory);

  if (fence) {
    if (Semantics & ImageMemory) {
      // An image fence requires a fence with R/W invalidate (L3 flush)
      // + for pre-DG2 platforms, sampler cache flush
      __intel_typedmemfence_handler(invalidateL1);
    }
    // A global/local memory fence requires a hardware fence in general,
    // although on some platforms they may be elided; platform-specific checks are performed in codegen
    if (Semantics & WorkgroupMemory) {
      __intel_memfence_handler(false, false, false, false, Memory);
    }
    if (Semantics & CrossWorkgroupMemory) {
      if (Memory == Device || Memory == CrossDevice) {
        __intel_memfence_handler(true, true, invalidateL1, evictL1, Memory);
      } else if (Memory == Region) {
        __intel_memfence_handler(false, true, invalidateL1, evictL1, Memory);
      } else {
        // Single workgroup executes on one DSS and shares the same L1 cache.
        // If scope doesn't reach outside of workgroup, L1 flush can be skipped.
        __intel_memfence_handler(false, true, false, false, Memory);
      }
    }
  }
}

#endif // __ATOMIC_WORK_ITEM_FENCE_H__
