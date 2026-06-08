/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'PrintToConsole=1 PrintAfter=Layout'" 2>&1 | FileCheck %s
// CHECK: !{{[0-9]+}} = !{!"sub_group_size", i32 32}

void __spirv_Subgroup2DBlockLoadINTEL(int element_size, int block_width, int block_height, int block_count,
                                      const global void *src_base_pointer, int memory_width, int memory_height,
                                      int memory_pitch, int2 coordinate, private void *dst_pointer);

void kernelTest(global void *src, global void *dst) {
  ushort data[2];

  __spirv_Subgroup2DBlockLoadINTEL(2, 32, 1, 1, src, 64, 32, 64, (int2)(0, 0), data);

  global ushort *dstPtr = (global ushort *)dst;
  int gid = get_global_id(0);
  int base = gid * 2;
  for (int i = 0; i < 2; i++)
    dstPtr[base + i] = data[i];
}

__attribute__((intel_reqd_sub_group_size(32))) kernel void test(global void *src, global void *dst) {
  kernelTest(src, dst);
}
