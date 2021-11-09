/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/atomic.h>
#include <cm-cl/vector.h>

using namespace cm;

extern "C" {
#include "spirv_atomics_common.h"
}

namespace {

template <typename PtrT, atomic::operation op, memory_order OCLSemantics,
          memory_scope OCLScope, typename... OpT>
constexpr auto invokeConcreteAtomic(PtrT *ptr, OpT... operands) {
  return atomic::execute<op, OCLSemantics, OCLScope, PtrT>(ptr, operands...);
}

template <typename PtrT, atomic::operation operation, memory_scope OCLScope,
          typename... OpT>
static CM_NODEBUG CM_INLINE auto
spirvAtomicHelperWithKnownScope(PtrT *ptr, int Semantics, OpT... operands) {
  switch (Semantics) {
  default:
  case SequentiallyConsistent:
    return invokeConcreteAtomic<PtrT, operation, memory_order_seq_cst,
                                OCLScope>(ptr, operands...);
  case Relaxed:
    return invokeConcreteAtomic<PtrT, operation, memory_order_relaxed,
                                OCLScope>(ptr, operands...);
  case Acquire:
    return invokeConcreteAtomic<PtrT, operation, memory_order_acquire,
                                OCLScope>(ptr, operands...);
  case Release:
    return invokeConcreteAtomic<PtrT, operation, memory_order_release,
                                OCLScope>(ptr, operands...);
  case AcquireRelease:
    return invokeConcreteAtomic<PtrT, operation, memory_order_acq_rel,
                                OCLScope>(ptr, operands...);
  }
}

// Iterate through all possible values of non-constant semantics and scope.
// Use the strict possible values if unknown: sequential-consistency semantics
// and cross-device scope.
template <atomic::operation operation, typename PtrT, typename... OpT>
static CM_NODEBUG CM_INLINE auto spirvAtomicHelper(PtrT *ptr, int Semantics,
                                                   int Scope, OpT... operands) {
  switch (Scope) {
  default:
  case CrossDevice:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_all_devices>(
        ptr, Semantics, operands...);
  case Device:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_device>(ptr, Semantics,
                                                                operands...);
  case Workgroup:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_work_group>(
        ptr, Semantics, operands...);
  case Subgroup:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_sub_group>(
        ptr, Semantics, operands...);
  case Invocation:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_work_item>(
        ptr, Semantics, operands...);
  }
}

} // namespace

#define SPIRV_ATOMIC_BUILTIN_LOAD(ADDRESS_SPACE, TYPE)                         \
  CM_NODEBUG CM_INLINE TYPE __spirv_AtomicLoad(ADDRESS_SPACE TYPE *ptr,        \
                                               int Scope, int Semantics) {     \
    return spirvAtomicHelper<atomic::operation::load, ADDRESS_SPACE TYPE>(     \
        ptr, Semantics, Scope);                                                \
  }

// FIXME: strict aliasing violation.
#define SPIRV_ATOMIC_BUILTIN_FLOATS(ADDRESS_SPACE)                             \
  CM_NODEBUG CM_INLINE float __spirv_AtomicLoad(ADDRESS_SPACE float *ptr,      \
                                                int Scope, int Semantics) {    \
    ADDRESS_SPACE int *int_ptr = reinterpret_cast<ADDRESS_SPACE int *>(ptr);   \
    return as_float(__spirv_AtomicLoad(int_ptr, Scope, Semantics));            \
  }                                                                            \
  CM_NODEBUG CM_INLINE double __spirv_AtomicLoad(ADDRESS_SPACE double *ptr,    \
                                                 int Scope, int Semantics) {   \
    ADDRESS_SPACE long *long_ptr =                                             \
        reinterpret_cast<ADDRESS_SPACE long *>(ptr);                           \
    return as_double(__spirv_AtomicLoad(long_ptr, Scope, Semantics));          \
  }

SPIRV_ATOMIC_BUILTIN_LOAD(__global, int)
SPIRV_ATOMIC_BUILTIN_LOAD(__global, long)
SPIRV_ATOMIC_BUILTIN_FLOATS(__global)
SPIRV_ATOMIC_BUILTIN_LOAD(__local, int)
SPIRV_ATOMIC_BUILTIN_LOAD(__local, long)
SPIRV_ATOMIC_BUILTIN_FLOATS(__local)
SPIRV_ATOMIC_BUILTIN_LOAD(__generic, int)
SPIRV_ATOMIC_BUILTIN_LOAD(__generic, long)
SPIRV_ATOMIC_BUILTIN_FLOATS(__generic)

#define SPIRV_ATOMIC_BUILTIN_STORE(ADDRESS_SPACE, TYPE)                        \
  CM_NODEBUG CM_INLINE void __spirv_AtomicStore(                               \
      ADDRESS_SPACE TYPE *ptr, int Scope, int Semantics, TYPE Value) {         \
    spirvAtomicHelper<atomic::operation::store, ADDRESS_SPACE TYPE>(           \
        ptr, Semantics, Scope, Value);                                         \
  }

SPIRV_ATOMIC_BUILTIN_STORE(__global, int)
SPIRV_ATOMIC_BUILTIN_STORE(__global, long)
SPIRV_ATOMIC_BUILTIN_STORE(__global, float)
SPIRV_ATOMIC_BUILTIN_STORE(__global, double)
SPIRV_ATOMIC_BUILTIN_STORE(__local, int)
SPIRV_ATOMIC_BUILTIN_STORE(__local, long)
SPIRV_ATOMIC_BUILTIN_STORE(__local, float)
SPIRV_ATOMIC_BUILTIN_STORE(__local, double)
SPIRV_ATOMIC_BUILTIN_STORE(__generic, int)
SPIRV_ATOMIC_BUILTIN_STORE(__generic, long)
SPIRV_ATOMIC_BUILTIN_STORE(__generic, float)
SPIRV_ATOMIC_BUILTIN_STORE(__generic, double)

#define SPIRV_ATOMIC_BUILTIN_BINARY(SPIRV_ATOMIC_OP, GEN_ATOMIC_OP,            \
                                    ADDRESS_SPACE, TYPE)                       \
  CM_NODEBUG CM_INLINE TYPE __spirv_Atomic##SPIRV_ATOMIC_OP(                   \
      ADDRESS_SPACE TYPE *ptr, int Scope, int Semantics, TYPE Value) {         \
    return spirvAtomicHelper<GEN_ATOMIC_OP, ADDRESS_SPACE TYPE>(               \
        ptr, Semantics, Scope, Value);                                         \
  }

SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __global, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __global, ulong)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __local, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __local, ulong)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __generic, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __generic, ulong)

SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __global, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __global, ulong)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __local, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __local, ulong)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __generic, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __generic, ulong)

SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __global, float)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __global, double)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __local, float)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __local, double)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __generic, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __generic, float)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __generic,
                            double)

#define SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(                              \
    SPIRV_ATOMIC_OP, GEN_ATOMIC_OP, OPVALUE, ADDRESS_SPACE, TYPE)              \
  CM_NODEBUG CM_INLINE TYPE __spirv_Atomic##SPIRV_ATOMIC_OP(                   \
      ADDRESS_SPACE TYPE *ptr, int Scope, int Semantics) {                     \
    TYPE Value = OPVALUE;                                                      \
    return spirvAtomicHelper<GEN_ATOMIC_OP, ADDRESS_SPACE TYPE>(               \
        ptr, Semantics, Scope, Value);                                         \
  }

SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __generic, long)

// FIXME: Unequal semantics will be eventually merged with
// Equal semantics which will result in Equal semantics,
// so we're skipping here for now.
#define SPIRV_ATOMIC_BUILTIN_TERNARY(SPIRV_ATOMIC_OP, GEN_ATOMIC_OP,           \
                                     ADDRESS_SPACE, TYPE)                      \
  CM_NODEBUG CM_INLINE TYPE __spirv_Atomic##SPIRV_ATOMIC_OP(                   \
      ADDRESS_SPACE TYPE *ptr, int Scope, int Equal, int Unequal, TYPE Value1, \
      TYPE Value2) {                                                           \
    return spirvAtomicHelper<GEN_ATOMIC_OP, ADDRESS_SPACE TYPE>(               \
        ptr, Equal, Scope, Value1, Value2);                                    \
  }

SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __global, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __global, long)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __local, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __local, long)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __generic, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __generic, long)

SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __global, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __global, long)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __local, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __local, long)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __generic, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __generic, long)
