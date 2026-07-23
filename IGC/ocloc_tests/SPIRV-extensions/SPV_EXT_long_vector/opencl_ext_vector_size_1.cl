/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// OpenCL C with a size-1 ext_vector. Checks that the frontend emits the vector
// under SPV_EXT_long_vector / LongVectorEXT.

// UNSUPPORTED: lib-igc-clang
// FIXME: This test is currently failing because igc-clang's SPIRVWriter does not yet support SPV_EXT_long_vector.
//        Once it does, this test should be enabled for igc-clang as well.

// REQUIRES: llvm-spirv, pvc-supported

// RUN: rm -rf %t.d && mkdir %t.d
// RUN: ocloc compile -file %s -device pvc -spv_only -output_no_suffix -out_dir %t.d
// RUN: llvm-spirv %t.d/*.spv -to-text -o - | FileCheck %s --check-prefix=CHECK-SPIRV

// CHECK-SPIRV-DAG: Capability LongVectorEXT
// CHECK-SPIRV-DAG: Extension "SPV_EXT_long_vector"
// CHECK-SPIRV-DAG: TypeVector [[#]] [[#]] 1

typedef float float1 __attribute__((ext_vector_type(1)));

__kernel void test_vec1(__global float1 *in, __global float1 *out) {
  out[0] = in[0] + in[0];
}
