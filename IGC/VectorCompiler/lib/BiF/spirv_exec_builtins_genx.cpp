/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/atomic.h>
#include <cm-cl/exec.h>

using namespace cm;

CM_NODEBUG CM_INLINE uint __spirv_BuiltInWorkDim() { return 3; }

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInGlobalSize(int dim) {
  return static_cast<ulong>(cm::exec::get_local_size(dim)) *
         cm::exec::get_group_count(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInGlobalInvocationId(int dim) {
  return cm::exec::get_local_id(dim) +
         static_cast<ulong>(cm::exec::get_group_id(dim)) *
             cm::exec::get_local_size(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInWorkgroupSize(int dim) {
  return cm::exec::get_local_size(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInEnqueuedWorkgroupSize(int dim) {
  return cm::exec::get_local_size(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInLocalInvocationId(int dim) {
  return cm::exec::get_local_id(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInNumWorkgroups(int dim) {
  return cm::exec::get_group_count(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInWorkgroupId(int dim) {
  return cm::exec::get_group_id(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInGlobalOffset(int dim) {
  // VC does not support global offset yet
  return 0;
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInLocalInvocationIndex() {
  using namespace cm::exec;
  return get_local_size(0) * get_local_size(1) * get_local_id(2) +
         get_local_size(0) * get_local_id(1) + get_local_id(0);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInGlobalLinearId() {
  using namespace cm::exec;
  uint group_id = get_group_count(0) * get_group_count(1) * get_group_id(2) +
                  get_group_count(0) * get_group_id(1) + get_group_id(0);
  uint local_size = get_local_size(0) * get_local_size(1) * get_local_size(2);
  uint local_id = __spirv_BuiltInLocalInvocationIndex();

  return group_id * local_size + local_id;
}

CM_NODEBUG CM_INLINE uint __spirv_BuiltInSubgroupSize() {
  // VC code always uses SubgroupSize == 1
  return 1;
}

CM_NODEBUG CM_INLINE uint __spirv_BuiltInSubgroupMaxSize() {
  // VC code always uses SubgroupSize == 1
  return 1;
}

CM_NODEBUG CM_INLINE uint __spirv_BuiltInNumSubgroups() {
  return __spirv_BuiltInWorkgroupSize(0) * __spirv_BuiltInWorkgroupSize(1) *
         __spirv_BuiltInWorkgroupSize(2);
}

CM_NODEBUG CM_INLINE uint __spirv_BuiltInNumEnqueuedSubgroups() {
  return __spirv_BuiltInNumSubgroups();
}

CM_NODEBUG CM_INLINE uint __spirv_BuiltInSubgroupId() {
  return __spirv_BuiltInLocalInvocationIndex();
}

CM_NODEBUG CM_INLINE uint __spirv_BuiltInSubgroupLocalInvocationId() {
  // VC code always uses SubgroupSize == 1
  return 0;
}

CM_NODEBUG CM_INLINE void __spirv_ControlBarrier(int scope, int memory_scope,
                                                 int memory_semantics) {
  cm::exec::fence(memory_scope, memory_semantics);
  cm::exec::barrier(scope);
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

// When __SubDeviceID is declared as an extern int, it is lowered to LLVM-IR
// like:
//
// @__SubDeviceID = external addrspace(1) global i32, align 4
//
// This global address is being then patched by the runtime and can be set to
// null when implicit scaling is disabled. One may wonder why `__SubDeviceID` is
// not declared as an `extern int*` In this case this would end up as a pointer
// to pointer in LLVM-IR. It would generate two loads and wouldn't be consistent
// with runtime behavior.
__global extern int __SubDeviceID;

// This variable has to be patched by GenXInitBiFConstant pass which will
// convert it into a constant with a target specific initialization value
int __cm_cl_MaxHWThreadIDPerSubDevice;

int __spirv_BuiltInSubDeviceIDINTEL() {
  __global volatile int *P = &__SubDeviceID;
  if (!P)
    return 0;
  return *P;
}

int __spirv_BuiltInGlobalHWThreadIDINTEL() {
  int SubDeviceId = __spirv_BuiltInSubDeviceIDINTEL();
  return cm::detail::__cm_cl_hw_thread_id() +
         SubDeviceId * __cm_cl_MaxHWThreadIDPerSubDevice;
}

int __vc_assert_print(__constant const char *fmt, ...);

extern "C" _Noreturn CM_NODEBUG CM_INLINE void
__devicelib_assert_fail(__generic const char *expr, __generic const char *file,
                        int32_t line, __generic const char *func, uint64_t gid0,
                        uint64_t gid1, uint64_t gid2, uint64_t lid0,
                        uint64_t lid1, uint64_t lid2) {
  __vc_assert_print("Assert called: %s.\n"
                    "File %s, Line %u, Function %s, "
                    "gid(%lu, %lu, %lu), lid(%lu, %lu, %lu).\n",
                    expr, file, line, func, gid0, gid1, gid2, lid0, lid1, lid2);
  atomic::execute<atomic::operation::store, memory_order_release,
                  memory_scope_all_devices>(cm::detail::assert_flags(), 1u);
  cm::detail::__cm_cl_debugtrap();
  cm::detail::__cm_cl_trap();
}
