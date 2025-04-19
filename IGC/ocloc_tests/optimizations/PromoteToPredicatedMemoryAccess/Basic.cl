/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Basic test that verifies predicated load/store

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM

// CHECK-ASM: {{[_a-z0-9A-Z]+}}:
// CHECK-ASM-DAG:         cmp (32|M0)   (lt)[[F1:f[0-9\.]+]]   null<1>:d     r{{[0-9\.]+}}<1;1,0>:d    r{{[0-9\.]+}}<0;1,0>:d
// CHECK-ASM-DAG:         cmp (32|M0)   (gt)[[F2:f[0-9\.]+]]   null<1>:d     r{{[0-9\.]+}}<1;1,0>:d    r{{[0-9\.]+}}<0;1,0>:d
// CHECK-ASM-DAG: (W)     mov (4|M0)               [[Z1:r[0-9]+]].0<1>:ud    0x0:ud
// CHECK-ASM:         mov (32|M0)              [[L1:r[0-9]+]].0<1>:ud   [[Z1]].0<0;1,0>:ud
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].1<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].2<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].3<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              [[L2:r[0-9]+]].0<1>:ud   [[Z1]].0<0;1,0>:ud
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].1<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].2<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].3<0;1,0>:f
// CHECK-ASM: ([[F1]])  load.ugm.d32x4.a64 (32|M0)  [[L1]]:8       [{{[a-z0-9:]+}}]
// CHECK-ASM: ([[F2]])  load.ugm.d32x4.a64 (32|M0)  [[L2]]:8       [{{[a-z0-9:]+}}]
// CHECK-ASM:         mov (32|M0)              [[L3:r[0-9]+]].0<1>:ud   [[Z1]].0<0;1,0>:ud
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].1<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].2<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].3<0;1,0>:f
// CHECK-ASM: ([[F1]])  load.ugm.d32x4.a64 (32|M0)  [[L3]]:8       [{{[a-z0-9:]+}}]
// CHECK-ASM:         mov (32|M0)              [[L4:r[0-9]+]].0<1>:ud   [[Z1]].0<0;1,0>:ud
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].1<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].2<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:f    [[Z1]].3<0;1,0>:f
// CHECK-ASM: ([[F2]])  load.ugm.d32x4.a64 (32|M0)  [[L4]]:8       [{{[a-z0-9:]+}}]
// CHECK-ASM: (~[[F1]]) goto (32|M0)                         [[LABEL:[_a-z0-9A-Z]+]]   [[LABEL]]
// CHECK-ASM: {{[_a-z0-9A-Z]+}}:
// CHECK-ASM:         store.ugm.d32x4.a64 (32|M0)  [{{[a-z0-9:]+}}]    r{{[0-9]+}}:8
// CHECK-ASM: [[LABEL]]:
// CHECK-ASM: ([[F2]])  store.ugm.d32x4.a64 (32|M0)  [{{[a-z0-9:]+}}]    r{{[0-9]+}}:8

// Constant merge value
__attribute__((intel_reqd_sub_group_size(32)))
__kernel void add_kernel(__global const float4* in0, __global const float4* in1, __global float4* out, const int predicate) {
  int gid = get_global_id(0);

  int readIdx1 = gid % 2; // 0, 1
  int readIdx2 = 2 + gid % 3; // 2, 3, 4

  int fltGid = gid < predicate;
  int fgtGid = gid > predicate;

  float4 f4In00 = 0;
  float4 f4In01 = 0;
  float4 f4In10 = 0;
  float4 f4In11 = 0;

  if (fltGid)
    f4In00 = in0[readIdx1];

  if (fgtGid)
    f4In01 = in0[readIdx2];

  if (fltGid)
    f4In10 = in1[readIdx1];

  if (fgtGid)
    f4In11 = in1[readIdx2];

  float4 result = f4In00 + f4In01 + f4In10 + f4In11;
  if(fltGid) {
    result = result - f4In00 - f4In01;
    out[gid] = result;
  }

  if(fgtGid) {
    result = result - f4In10 - f4In11;
    out[gid] = result;
  }
}
