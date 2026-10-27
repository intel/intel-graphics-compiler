/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: lib-igc-clang
// REQUIRES: regkeys, nvl-p-b0-supported

// With the ray query return value:
// - intel_ray_query_init does not initialize the potential hit, so the
//   committed hit holds the only t = INFINITY store,
// - an accept-first commit stores COMMITTED_STATUS = CANDIDATE_TYPE + 1,
// - intel_has_committed_hit tests COMMITTED_STATUS (bits 2:1) of the fence.

// RUN: ocloc compile -file %s -device nvl-p-b0 -options "-igc_opts 'PrintToConsole=1,PrintAfter=igc-resolve-ocl-raytracing-builtins'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" 2>&1 | FileCheck %s

// CHECK: store float {{0x7FF0000000000000|\+inf}}
// CHECK-NOT: store float {{0x7FF0000000000000|\+inf}}
// CHECK: add nuw nsw i64 %{{.*}}, 2
// CHECK: [[STATUS:%.*]] = and i64 %{{.*}}, 6
// CHECK-NEXT: icmp ne i64 [[STATUS]], 0

kernel void test(global int *out, intel_raytracing_acceleration_structure_t accel) {
  intel_ray_desc_t raydesc = {0};
  raydesc.flags = intel_ray_flags_accept_first_hit_and_end_search;
  intel_ray_query_t query = intel_ray_query_init(raydesc, accel);
  intel_ray_query_start_traversal(query);
  intel_ray_query_sync(query);
  intel_ray_query_commit_potential_hit(query);
  out[0] = intel_has_committed_hit(query);
}
