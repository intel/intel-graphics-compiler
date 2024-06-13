/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/barrier.h>

CM_NODEBUG CM_INLINE void __spirv_ControlBarrier(int scope, int memory_scope,
                                                 int memory_semantics) {
  cm::exec::fence(memory_scope, memory_semantics);
  if (scope == cm::exec::workgroup)
    cm::exec::local_barrier();
  else if (scope == cm::exec::device)
    cm::exec::global_barrier();
}

CM_NODEBUG CM_INLINE void __spirv_MemoryBarrier(int scope, int semantics) {
  cm::exec::fence(scope, semantics);
}

CM_NODEBUG CM_INLINE void
__spirv_ControlBarrierArriveINTEL(int scope, int memory_scope,
                                  int memory_semantics) {
  cm::exec::fence(memory_scope, memory_semantics);
  cm::exec::barrier_arrive(scope);
}

CM_NODEBUG CM_INLINE void
__spirv_ControlBarrierWaitINTEL(int scope, int memory_scope,
                                int memory_semantics) {
  cm::exec::fence(memory_scope, memory_semantics);
  cm::exec::barrier_wait(scope);
}
