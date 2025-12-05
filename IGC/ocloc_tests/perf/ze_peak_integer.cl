/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys
// RUN: ocloc compile -file %s -options "-igc_opts 'VISAOptions=-asmToConsole'" -device dg2 | FileCheck %s --check-prefix=CHECK
// RUN: ocloc compile -file %s -options "-igc_opts 'VISAOptions=-asmToConsole'" -device mtl | FileCheck %s --check-prefix=CHECK
// RUN: ocloc compile -file %s -options "-igc_opts 'VISAOptions=-asmToConsole'" -device arl-h | FileCheck %s --check-prefix=CHECK

// ze_peak, compute_integer_vN kernel
// This test checks that we generate the beneficial SIMD width for the kernels

// CHECK: .kernel compute_int_v4
// CHECK: add (16|M16)
// CHECK: .kernel compute_int_v8
// CHECK: add (16|M0)
// CHECK: .kernel compute_int_v16
// CHECK: add (16|M0)

#define MAD_4(x, y)     x = (y*x) + y;      y = (x*y) + x;      x = (y*x) + y;      y = (x*y) + x;
#define MAD_16(x, y)    MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);        MAD_4(x, y);
#define MAD_64(x, y)    MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);       MAD_16(x, y);

__kernel void compute_int_v4(__global int *input_value, __global int *output)
{
    int4 x = (int4)(input_value[0], (input_value[0]+1), (input_value[0]+2), (input_value[0]+3));
    int4 y = (int4)get_local_id(0);

    MAD_64(x, y);
    MAD_64(x, y);
    MAD_64(x, y);
    MAD_64(x, y);

    output[get_global_id(0)] = (y.S0) + (y.S1) + (y.S2) + (y.S3);
}


__kernel void compute_int_v8(__global int *input_value, __global int *output)
{
    int8 x = (int8)(input_value[0], (input_value[0]+1), (input_value[0]+2), (input_value[0]+3), (input_value[0]+4), (input_value[0]+5), (input_value[0]+6), (input_value[0]+7));
    int8 y = (int8)get_local_id(0);

    MAD_64(x, y);
    MAD_64(x, y);

    output[get_global_id(0)] = (y.S0) + (y.S1) + (y.S2) + (y.S3) + (y.S4) + (y.S5) + (y.S6) + (y.S7);
}

__kernel void compute_int_v16(__global int *input_value, __global int *output)
{
    int16 x = (int16)(input_value[0], (input_value[0]+1), (input_value[0]+2), (input_value[0]+3), (input_value[0]+4), (input_value[0]+5), (input_value[0]+6), (input_value[0]+7),
                    (input_value[0]+8), (input_value[0]+9), (input_value[0]+10), (input_value[0]+11), (input_value[0]+12), (input_value[0]+13), (input_value[0]+14), (input_value[0]+15));
    int16 y = (int16)get_local_id(0);

    MAD_64(x, y);

    int2 t = (y.S01) + (y.S23) + (y.S45) + (y.S67) + (y.S89) + (y.SAB) + (y.SCD) + (y.SEF);
    output[get_global_id(0)] = t.S0 + t.S1;
}
