/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This test checks if has_sample is properly emitted for kernels that explicitly use samplers.

// REQUIRES: regkeys

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1'" -device dg2 | FileCheck %s

// CHECK:     has_sample

__kernel void uses_sampler_explicitly(__read_only image3d_t input,
                                       __global uchar *dst,
                                       sampler_t sampler,
                                       int4 srcOffset,
                                       uint dstOffset,
                                       uint2 Pitch) {
    uint x = get_global_id(0);
    uint y = get_global_id(1);
    uint z = get_global_id(2);

    const int4 srcCoord = (int4)(x, y, z, 0) + srcOffset;
    uint DstOffset = dstOffset + (y * Pitch.x) + (z * Pitch.y);

    uint4 c = read_imageui(input, sampler, srcCoord);
    *(dst + DstOffset + x) = convert_uchar_sat(c.x);
}
