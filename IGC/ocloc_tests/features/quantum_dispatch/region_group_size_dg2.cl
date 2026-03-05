/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// UNSUPPORTED: system-windows
// REQUIRES: regkeys, dg2-supported
// RUN: not ocloc compile -file %s -options "-igc_opts 'VISAOptions=-asmToConsole'" -device dg2 2>&1 | FileCheck %s --check-prefix=CHECK-OCLOC

// CHECK-OCLOC: region_group_size is not supported on this platform

uint __builtin_IB_get_region_group_size(int dim);
kernel void test(global uint* out, int dim) {
  out[0] = __builtin_IB_get_region_group_size(dim);
}
