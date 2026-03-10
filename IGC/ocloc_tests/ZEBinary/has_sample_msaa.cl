/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This test checks if has_sample is properly emitted for kernels that use samplers implicitly.

// REQUIRES: regkeys

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1'" -device dg2 \
// RUN: -internal_options "-cl-ext=+cl_khr_gl_msaa_sharing" | FileCheck %s

// CHECK:     has_sample

__kernel void has_sample_msaa(
    read_only  image2d_array_msaa_t src,
    write_only image2d_array_t      dst,
    int width,
    int height,
    int layers,
    int sampleIndex
)
{
    const int x = (int)get_global_id(0);
    const int y = (int)get_global_id(1);
    const int layer = (int)get_global_id(2);

    if (x >= width || y >= height || layer >= layers) return;

    const int4 coord = (int4)(x, y, layer, 0);

    const float4 c = read_imagef(src, coord, sampleIndex);
    write_imagef(dst, coord, c);
}
