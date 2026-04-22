/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys

// This test ensures successful compilation on three platforms: DG2, NVL-P-B0, and PTL-H.
// Each platform follows different code paths to select the SIMD size.
// - Minimum SIMD size (for general kernels):
//     DG2: 8, NVL-P-B0: 16, PTL-H: 16
// - For kernels with ray query:
//     DG2:      maximum SIMD size supported is 16, preferred is 8
//     NVL-P-B0: maximum SIMD size supported is 32, preferred is 16
//     PTL-H:    maximum SIMD size supported is 16, preferred is 16

// RUN: %if nvl-p-b0-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device nvl-p-b0 2>&1 | FileCheck %s --check-prefix CHECK-NVLP %}
// CHECK-NVLP: .kernel_attr SimdSize=32

// RUN: %if ptl-h-supported %{ not ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'ForceOCLSIMDWidth=32'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device ptl-h 2>&1 | FileCheck %s --check-prefix CHECK-PTLH %}
// CHECK-PTLH: error: in kernel 'test': Failed to compile the forced SIMD size of 32

// RUN: %if dg2-supported %{ not ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'ForceOCLSIMDWidth=32'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device dg2 2>&1 | FileCheck %s --check-prefix CHECK-DG2 %}
// CHECK-DG2: error: in kernel 'test': Failed to compile the forced SIMD size of 32

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test()
{
    intel_ray_desc_t raydesc;
    intel_raytracing_acceleration_structure_t hwaccel_ptr;
    intel_ray_query_t query = intel_ray_query_init(raydesc, hwaccel_ptr);
    intel_ray_query_start_traversal(query);
}
