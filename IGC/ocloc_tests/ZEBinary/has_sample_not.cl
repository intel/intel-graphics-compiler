/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This test checks if has_sample is properly not emitted for kernels that do not use samplers.

// REQUIRES: regkeys

// RUN: ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpZEInfoToConsole=1'" -device dg2 | FileCheck %s

// CHECK-NOT:     has_sample

__kernel void uses_sampler_not(__read_write image3d_t input,
                                       __global uchar *dst,
                                       int4 srcOffset,
                                       uint dstOffset,
                                       uint2 Pitch) {
    uint x = get_global_id(0);
    uint y = get_global_id(1);
    uint z = get_global_id(2);

    const int4 srcCoord = (int4)(x, y, z, 0) + srcOffset;
    uint DstOffset = dstOffset + (y * Pitch.x) + (z * Pitch.y);

    uint4 c = read_imageui(input, srcCoord);
    *(dst + DstOffset + x) = convert_uchar_sat(c.x);
}
