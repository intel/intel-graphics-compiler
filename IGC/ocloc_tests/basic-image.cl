// UNSUPPORTED: system-windows
// REQUIRES: dg2-supported
// RUN: ocloc compile -file %s -options "-g" -internal_options "-cl-intel-use-bindless-mode" -device dg2 2>&1 | FileCheck %s

// Check that kernel build is successful in "-g" mode.

// CHECK: Build succeeded

const sampler_t sampler =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

kernel void test(const global float *a,
                 global float *c,
                 read_only image2d_t input,
                 sampler_t sampler
                 ) {
    const int gid = get_global_id(0);

    int2 coord = {get_global_id(0), get_global_id(1)};
    float4 data = read_imagef(input, coord);
    c[gid] = a[gid] + data.x;
}
