/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: system-windows
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1'" -internal_options "-cl-intel-use-bindless-mode" -device mtl | FileCheck %s

// Check that unused image args arent tagged with 'stateful' addrmode in bindless mode.

// CHECK-NOT: addrmode:        stateful
// CHECK-NOT: binding_table_indices:

const sampler_t sampler =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

kernel void test(const global float *a,
                 global float *c,
                 read_only image2d_t input,
                 read_only image2d_t unused,
                 sampler_t sampler
                 ) {
    const int gid = get_global_id(0);

    int2 coord = {get_global_id(0), get_global_id(1)};
    float4 data = read_imagef(input, coord);
    c[gid] = a[gid] + data.x;
}
