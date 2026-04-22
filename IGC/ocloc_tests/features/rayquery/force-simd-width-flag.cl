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

// RUN: %if nvl-p-b0-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device nvl-p-b0 2>&1 | FileCheck %s --check-prefix CHECK-NVLP-DEFAULT %}
// CHECK-NVLP-DEFAULT: .kernel_attr SimdSize=16

// RUN: %if nvl-p-b0-supported %{ not ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'ForceOCLSIMDWidth=8'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device nvl-p-b0 2>&1 | FileCheck %s --check-prefix CHECK-NVLP-FORCE-8 %}
// CHECK-NVLP-FORCE-8: error: SIMD size of 8 has been forced when SIMD size of at least 16 is required on this platform

// RUN: %if nvl-p-b0-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1,ForceOCLSIMDWidth=16'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device nvl-p-b0 2>&1 | FileCheck %s --check-prefix CHECK-NVLP-FORCE-16 %}
// CHECK-NVLP-FORCE-16: .kernel_attr SimdSize=16

// RUN: %if nvl-p-b0-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1,ForceOCLSIMDWidth=32'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device nvl-p-b0 2>&1 | FileCheck %s --check-prefix CHECK-NVLP-FORCE-32 %}
// CHECK-NVLP-FORCE-32: .kernel_attr SimdSize=32



// RUN: %if ptl-h-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device ptl-h 2>&1 | FileCheck %s --check-prefix CHECK-PTLH-DEFAULT %}
// CHECK-PTLH-DEFAULT: .kernel_attr SimdSize=16

// RUN: %if ptl-h-supported %{ not ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'ForceOCLSIMDWidth=8'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device ptl-h 2>&1 | FileCheck %s --check-prefix CHECK-PTLH-FORCE-8 %}
// CHECK-PTLH-FORCE-8: error: SIMD size of 8 has been forced when SIMD size of at least 16 is required on this platform

// RUN: %if ptl-h-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1,ForceOCLSIMDWidth=16'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device ptl-h 2>&1 | FileCheck %s --check-prefix CHECK-PTLH-FORCE-16 %}
// CHECK-PTLH-FORCE-16: .kernel_attr SimdSize=16

// RUN: %if ptl-h-supported %{ not ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'ForceOCLSIMDWidth=32'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device ptl-h 2>&1 | FileCheck %s --check-prefix CHECK-PTLH-FORCE-32 %}
// CHECK-PTLH-FORCE-32: error: in kernel 'test': Failed to compile the forced SIMD size of 32



// RUN: %if dg2-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device dg2 2>&1 | FileCheck %s --check-prefix CHECK-DG2-DEFAULT %}
// CHECK-DG2-DEFAULT: .kernel_attr SimdSize=16

// RUN: %if dg2-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1,ForceOCLSIMDWidth=8'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device dg2 2>&1 | FileCheck %s --check-prefix CHECK-DG2-FORCE-8 %}
// CHECK-DG2-FORCE-8: .kernel_attr SimdSize=8

// RUN: %if dg2-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1,ForceOCLSIMDWidth=16'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device dg2 2>&1 | FileCheck %s --check-prefix CHECK-DG2-FORCE-16 %}
// CHECK-DG2-FORCE-16: .kernel_attr SimdSize=16

// RUN: %if dg2-supported %{ not ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'ForceOCLSIMDWidth=32'" -internal_options "-cl-ext=-all,+cl_intel_rt_production" -device dg2 2>&1 | FileCheck %s --check-prefix CHECK-DG2-FORCE-32 %}
// CHECK-DG2-FORCE-32: error: in kernel 'test': Failed to compile the forced SIMD size of 32

kernel void test()
{
    intel_ray_desc_t raydesc;
    intel_raytracing_acceleration_structure_t hwaccel_ptr;
    intel_ray_query_t query = intel_ray_query_init(raydesc, hwaccel_ptr);
    intel_ray_query_start_traversal(query);
}
