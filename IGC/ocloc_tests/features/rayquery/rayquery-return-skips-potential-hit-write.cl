/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: lib-igc-clang
// REQUIRES: regkeys

// intel_ray_query_init and intel_ray_query_start_traversal marked the potential
// hit done and valid for the memory based traversal-done check. That check now
// reads the dispatch return value, so the bits are no longer written.
// 0x10010000 is the pair of bits.
//
// One CHECK-NOT covers both writes: the kernel reaches __basic_rtstack_init
// through intel_ray_query_init and then calls intel_ray_query_start_traversal.
// Only one store survives, the second sets bits the first one already set.

// RUN: %if nvl-p-b0-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device nvl-p-b0 2>&1 | FileCheck %s %}
// RUN: %if nvl-p-b0-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1,DisableRayQueryReturnOptimization=1'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device nvl-p-b0 2>&1 | FileCheck %s --check-prefix=DISABLED %}

// CHECK: .kernel "test"
// CHECK-NOT: 0x10010000

// DISABLED: 0x10010000

kernel void test() {
  intel_ray_desc_t raydesc;
  intel_raytracing_acceleration_structure_t hwaccel_ptr;
  intel_ray_query_t query = intel_ray_query_init(raydesc, hwaccel_ptr);
  intel_ray_query_start_traversal(query);
}
