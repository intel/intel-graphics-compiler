/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: system-windows
// REQUIRES: regkeys, pvc-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'VISAOptions=-asmToConsole, LoopSinkRegpressureMargin=128, ForceLoopSink=1, CodeLoopSinkingMinSize=4, ForceSIMDRPELimit=3'" -device pvc | FileCheck %s --check-prefix=CHECK-ASM

__attribute__((intel_reqd_sub_group_size(32)))
__kernel void register_pressure_example_forced(__global float* input, __global float* output, int N) {

    for (int i = 0; i < N; ++i) {
             int gid = get_global_id(0) + i;
             float a = input[gid];
             float b = a * 2.0f;
             float c = a + b;
             float d = c * 4.0f;
             float e = d - b;
             float f = e / 6.0f;
             float g = f + d;
             float h = g * 8.0f;
             float i = h - f;
             float j = i / 10.0f;
             // Store the result in the output array
             output[gid] += j;
             }

// CHECK-ASM:             mul (32|M0)
// CHECK-ASM-NEXT:        add (32|M0)
// CHECK-ASM-NEXT:        mul (32|M0)
// CHECK-ASM-NEXT:        add (32|M0)
// CHECK-ASM-NEXT:        mul (32|M0)
// CHECK-ASM-NEXT:        add (32|M0)
// CHECK-ASM-NEXT:        mul (32|M0)
// CHECK-ASM-NEXT:        add (32|M0)
// CHECK-ASM-NEXT:        mul (32|M0)
// CHECK-ASM-NEXT:        add (32|M0)
// CHECK-ASM-NEXT:        store.ugm.d32.a64 (32|M0)

}

__kernel void register_pressure_example(__global float* input, __global float* output, int N) {

    for (int i = 0; i < N; ++i) {
             int gid = get_global_id(0) + i;
             float a = input[gid];
             float b = a * 2.0f;
             float c = a + b;
             float d = c * 4.0f;
             float e = d - b;
             float f = e / 6.0f;
             float g = f + d;
             float h = g * 8.0f;
             float i = h - f;
             float j = i / 10.0f;
             // Store the result in the output array
             output[gid] += j;
             }


// CHECK-ASM: .kernel register_pressure_example
// CHECK-ASM:             mul (16|M0)
// CHECK-ASM-NEXT:        add (16|M0)
// CHECK-ASM-NEXT:        mul (16|M0)
// CHECK-ASM-NEXT:        add (16|M0)
// CHECK-ASM-NEXT:        mul (16|M0)
// CHECK-ASM-NEXT:        add (16|M0)
// CHECK-ASM-NEXT:        mul (16|M0)
// CHECK-ASM-NEXT:        add (16|M0)
// CHECK-ASM-NEXT:        mul (16|M0)
// CHECK-ASM-NEXT:        add (16|M0)
// CHECK-ASM-NEXT:        store.ugm.d32.a64 (16|M0)

}
