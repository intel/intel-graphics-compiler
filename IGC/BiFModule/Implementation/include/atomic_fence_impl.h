/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __ATOMIC_FENCE_IMPL_H__
#define __ATOMIC_FENCE_IMPL_H__

#define SEMANTICS_PRE_OP_NEED_FENCE (Release | AcquireRelease | SequentiallyConsistent)

#define SEMANTICS_POST_OP_NEEDS_FENCE (Acquire | AcquireRelease | SequentiallyConsistent)

#define FENCE_PRE_OP(Scope, Semantics, isGlobal)                                    \
    if (((Semantics) & (SEMANTICS_PRE_OP_NEED_FENCE)) > 0)                          \
    {                                                                               \
        bool flushL3 = (isGlobal) && ((Scope) == Device || (Scope) == CrossDevice); \
        __intel_memfence_handler(flushL3, isGlobal, false, isGlobal, Scope);        \
    }

#define FENCE_POST_OP(Scope, Semantics, isGlobal)                                   \
    if (((Semantics) & (SEMANTICS_POST_OP_NEEDS_FENCE)) > 0)                        \
    {                                                                               \
        bool flushL3 = (isGlobal) && ((Scope) == Device || (Scope) == CrossDevice); \
        __intel_memfence_handler(flushL3, isGlobal, isGlobal, false, Scope);        \
    }

void __intel_memfence_handler(
    bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1, Scope_t scope);

#endif // __ATOMIC_FENCE_IMPL_H__
