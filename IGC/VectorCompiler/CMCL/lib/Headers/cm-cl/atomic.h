/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_ATOMIC_H
#define CM_CL_ATOMIC_H

#include "define.h"
#include "detail/builtins.h"
#include "vector.h"

#include <opencl_def.h>
#include <opencl_type_traits.h>

namespace cm {
namespace atomic {

// Load is implemented via atomicrmw or, that's why integer types are expected.
template <memory_order semantics, memory_scope scope, typename PtrT>
auto load(PtrT *ptr) {
  using element_type = typename cl::pointer_traits<PtrT *>::element_type;
  static_assert(cl::is_integral<element_type>::value, "integral type expected");
  element_type zero_init = 0;
  return detail::atomicrmw<operation::orl, semantics, scope>(ptr, zero_init);
}

template <memory_order semantics, memory_scope scope, typename PtrT,
          typename OpT>
void store(PtrT *ptr, OpT operand) {
  detail::atomicrmw<operation::xchg, semantics, scope>(ptr, operand);
}

// Infer semantics on failure regarding C++ rules.
constexpr memory_order
get_cmpxch_memory_semantics_on_failure(memory_order semantics_on_success) {
  switch (semantics_on_success) {
  default:
    return semantics_on_success;
  case memory_order_acq_rel:
    return memory_order_acquire;
  case memory_order_release:
    return memory_order_relaxed;
  }
}

template <atomic::operation op, memory_order semantics, memory_scope scope,
          typename PtrT, typename... OpT>
auto execute(PtrT *ptr, OpT... operands) {
  constexpr unsigned NumArgs = sizeof...(operands);
  if constexpr (op == operation::cmpxchg) {
    static_assert(NumArgs == 2, "illegal number of arguments for cmpxchg");
    return detail::cmpxchg<
        semantics, get_cmpxch_memory_semantics_on_failure(semantics), scope>(
        ptr, operands...);
  } else if constexpr (op == operation::load) {
    static_assert(NumArgs == 0, "illegal number of arguments for load");
    return load<semantics, scope, PtrT>(ptr);
  } else if constexpr (op == operation::store) {
    static_assert(NumArgs == 1, "illegal number of arguments for store");
    store<semantics, scope, PtrT>(ptr, operands...);
    return;
  } else {
    static_assert(NumArgs == 1, "illegal number of arguments for atomicrmw");
    return detail::atomicrmw<op, semantics, scope>(ptr, operands...);
  }
}

} // namespace atomic

} // namespace cm

#endif // CM_CL_ATOMIC_H
