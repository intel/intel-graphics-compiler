/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: typed-pointers
// UNSUPPORTED: sys32
// RUN: ocloc compile -file %s -options "-cl-std=CL2.0 -cl-opt-disable -igc_opts 'PrintToConsole=1 PrintAfter=GenericNullPtrPropagation'" -device dg2 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM

__kernel void foo() {
  // CHECK-LLVM: [[PRV_PTR:%.*]] = load i32*, i32** %prv_ptr
  // CHECK-LLVM: [[CAST:%.*]] = addrspacecast i32* [[PRV_PTR]] to i32 addrspace(4)*
  // CHECK-LLVM: [[PRED:%.*]] = icmp ne i32* [[PRV_PTR]], null
  // CHECK-LLVM: [[SEL:%.*]] = select i1 [[PRED]], i32 addrspace(4)* [[CAST]], i32 addrspace(4)* null
  int* a = NULL;
  __private int* prv_ptr = to_private(a);
  int* generic_ptr = (__generic int*) prv_ptr;
  bool test = (generic_ptr == prv_ptr);
}
