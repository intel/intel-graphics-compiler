/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options "-cl-fast-relaxed-math -igc_opts 'VISAOptions=-asmToConsole'" -device dg2 | FileCheck %s --check-prefix=CHECK-ASM

// With -cl-fast-relaxed-math enabled, IGC computes e^x using 2^x because
// 2^x is directly supported by the hardware. To convert e^x to 2^x, it
// calculates 2^(x * log2(e)), where log2(e) is M_LOG2E_F (0x3FB8AA3B ≈ 1.44269504).
//
// For exp(a * b), IGC transforms this to exp2((a * b) * M_LOG2E_F).
// The compiler must preserve the original multiplication order to avoid overflow.
//
// Critical case: When a is large (e.g., FLOAT_MAX) and b is 0:
// - Correct order: (a * b) * M_LOG2E_F = (FLOAT_MAX * 0) * M_LOG2E_F = 0
// - Wrong order: (a * M_LOG2E_F) * b = (FLOAT_MAX * M_LOG2E_F) * 0 = INF * 0 = NaN
//
// This test verifies that multiplication by M_LOG2E_F happens right before passing
// the value to the math.exp instruction, preserving the original (a * b) multiplication order.
// To preserve this order, IGC must ensure that fast math flags are not applied to the
// (x * M_LOG2E_F) multiplication, preventing reordering optimization (visitBinaryOperatorFmulFaddPropagation)
// in CustomUnsafeOptPass.cpp.

// CHECK-ASM: mul {{\(.*\)}} [[REG:(r[0-9]+|acc[0-9]+)]].{{.*}}:f {{.*}}:f 0x3FB8AA3B:f
// CHECK-ASM: math.exp {{\(.*\)}} {{.*}}:f [[REG]].{{.*}}:f

__attribute__((intel_reqd_sub_group_size(16)))
__kernel void test_exp(__global const float *A,__global const float2 *B, __global float2 *out)
{
    size_t gid = get_global_id(0);
    float a = A[gid];
    float2 b = B[gid];
    out[gid] = exp(a * -b);
}
