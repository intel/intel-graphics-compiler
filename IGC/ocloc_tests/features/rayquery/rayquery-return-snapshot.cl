/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// UNSUPPORTED: lib-igc-clang
// REQUIRES: regkeys, nvl-p-b0-supported, ptl-supported

// RT writeback ignores the execution mask, so in a traversal loop the raw
// TraceRaySync result must have no user but the masked snapshot. Otherwise
// DeSSA may coalesce it with the fence PHI, and the next dispatch overwrites
// the status of lanes that already left the loop.

// RUN: ocloc compile -file %s -device nvl-p-b0 -options "-igc_opts 'PrintToConsole=1,PrintBefore=EmitPass'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" 2>&1 | FileCheck %s
// RUN: ocloc compile -file %s -device ptl-h -options "-igc_opts 'PrintToConsole=1,PrintBefore=EmitPass'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" 2>&1 | FileCheck %s

// CHECK-LABEL: define spir_kernel void @traversal_loop(
// CHECK: [[RAW0:%[0-9]+]] = call i32 @llvm.genx.GenISA.TraceRaySync
// CHECK-NEXT: call i32 @llvm.genx.GenISA.ReadTraceRaySyncResult(i32 [[RAW0]])
// CHECK-NOT: [[RAW0]]{{[^0-9]}}
// CHECK: [[RAW1:%[0-9]+]] = call i32 @llvm.genx.GenISA.TraceRaySync
// CHECK-NEXT: call i32 @llvm.genx.GenISA.ReadTraceRaySyncResult(i32 [[RAW1]])
// CHECK-NOT: [[RAW0]]{{[^0-9]}}
// CHECK-NOT: [[RAW1]]{{[^0-9]}}
// CHECK: ret void

kernel void traversal_loop(global int *out, intel_raytracing_acceleration_structure_t accel) {
  intel_ray_desc_t raydesc = {0};
  intel_ray_query_t query = intel_ray_query_init(raydesc, accel);
  intel_ray_query_start_traversal(query);
  intel_ray_query_sync(query);
  while (!intel_is_traversal_done(query)) {
    if (intel_get_hit_candidate(query, intel_hit_type_potential_hit) == intel_candidate_type_triangle)
      intel_ray_query_commit_potential_hit(query);
    intel_ray_query_start_traversal(query);
    intel_ray_query_sync(query);
  }
  out[get_global_id(0)] = intel_has_committed_hit(query);
}
