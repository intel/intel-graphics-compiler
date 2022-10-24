/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifdef cl_intel_pvc_rt_validation

void* intel_get_rt_stack( rtglobals_t rt_dispatch_globals ) {
  return __builtin_IB_intel_get_rt_stack(rt_dispatch_globals);
}

void* intel_get_thread_btd_stack( rtglobals_t rt_dispatch_globals ) {
  return __builtin_IB_intel_get_thread_btd_stack(rt_dispatch_globals);
}

void* intel_get_global_btd_stack( rtglobals_t rt_dispatch_globals ) {
  return __builtin_IB_intel_get_global_btd_stack(rt_dispatch_globals);
}

rtfence_t intel_dispatch_trace_ray_query(
    rtglobals_t rt_dispatch_globals, uint bvh_level, uint traceRayCtrl) {
  return __builtin_IB_intel_dispatch_trace_ray_query(rt_dispatch_globals, bvh_level, traceRayCtrl);
}

void intel_rt_sync(rtfence_t fence) {
  return __builtin_IB_intel_rt_sync(fence);
}

global void* intel_get_implicit_dispatch_globals() {
    return __builtin_IB_intel_get_implicit_dispatch_globals();
}

#endif
