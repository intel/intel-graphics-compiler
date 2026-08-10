/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Verify the memory scope of the fences emitted for work-group synchronization.
//
// REQUIRES: regkeys, dg2-supported

#define TILE_SIZE 64

#if defined(TEST_WAIT_GROUP_EVENTS)

//==============================================================================
// wait_group_events must produce a work-group-scope, non-invalidating fence pair:
//==============================================================================
// RUN: ocloc compile -file %s -device dg2 -options "-cl-std=CL3.0 -DTEST_WAIT_GROUP_EVENTS \
// RUN: -igc_opts 'DumpVISAASMToConsole=1'" \
// RUN: | FileCheck %s --check-prefix=CHECK-WGE --implicit-check-not=lsc_fence.ugm.invalidate \
// RUN: --implicit-check-not=lsc_fence.ugm.evict

// CHECK-WGE-DAG: lsc_fence.ugm.none.group
// CHECK-WGE-DAG: lsc_fence.slm.none.group

__kernel void test_wait_group_events(const __global float *in, __global float *out) {
  __local float tile[TILE_SIZE];
  event_t ev = async_work_group_copy(tile, in + get_group_id(0) * TILE_SIZE, TILE_SIZE, 0);
  wait_group_events(1, &ev);
  out[get_global_id(0)] = tile[TILE_SIZE - 1 - get_local_id(0)];
}

#elif defined(TEST_DEVICE_FENCE)

//==============================================================================
// Not breaking actual device scope fences test:
//==============================================================================
// RUN: ocloc compile -file %s -device dg2 -options "-cl-std=CL3.0 -DTEST_DEVICE_FENCE \
// RUN: -igc_opts 'DumpVISAASMToConsole=1'" \
// RUN: | FileCheck %s --check-prefix=CHECK-DEV

// CHECK-DEV: lsc_fence.ugm.invalidate

__kernel void test_device_fence(const __global float *in, __global float *out) {
  out[get_global_id(0)] = in[get_global_id(0)];
  atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_acquire, memory_scope_device);
  out[get_global_id(0) + 1] = in[get_global_id(0) + 1];
}

#endif
