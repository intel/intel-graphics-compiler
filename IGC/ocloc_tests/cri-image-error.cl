/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-cl-std=CL2.0" -device cri 2>&1 | FileCheck %s

// CHECK: error: For CRI platform images should not be used
// XFAIL: *

kernel void test(read_only image2d_t input) {}
