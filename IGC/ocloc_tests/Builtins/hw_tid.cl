/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if a call to  __builtin_IB_hw_thread_id in a stack call
// produces a code sequence with sr0 usage.

// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1'" -device dg2 | FileCheck %s --check-prefix=CHECK-VISA
// CHECK-VISA: recursive_call{{.*}}:
// CHECK-VISA: and (M1_NM, 1) HWTID(0,0)<1> %sr0(0,0)<0;1,0> 0x{{.*}}:d

int __builtin_IB_hw_thread_id();
int recursive_call(int n) {
  if(n < 10) return __builtin_IB_hw_thread_id();
  return recursive_call(n-1);
}

kernel void test(global int* buf) {
  int gid = get_global_id(0);
  buf[gid] = recursive_call(gid);
}
