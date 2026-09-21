/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: cri-supported

// RUN: not ocloc compile -file %s -options "-cl-std=CL2.0" -device cri 2>&1 | FileCheck %s

// CHECK: Images should not be used for this platform!

kernel void test(read_only image2d_t input) {}
