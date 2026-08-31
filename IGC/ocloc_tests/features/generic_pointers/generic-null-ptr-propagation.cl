/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, debug, dg2-supported, llvm-16-plus
// UNSUPPORTED: sys32
// RUN: ocloc compile -file %s -options "-cl-std=CL2.0 -cl-opt-disable -igc_opts 'PrintToConsole=1, PrintAfter=GenericNullPtrPropagation, EnableOpaquePointersBackend=1'" -device dg2 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM

__kernel void foo() {
  // CHECK-LLVM: [[PRV_PTR:%.*]] = load ptr, ptr %prv_ptr
  // CHECK-LLVM: [[CAST:%.*]] = addrspacecast ptr [[PRV_PTR]] to ptr addrspace(4)
  // CHECK-LLVM: [[PRED:%.*]] = icmp ne ptr [[PRV_PTR]], null
  // CHECK-LLVM: [[SEL:%.*]] = select i1 [[PRED]], ptr addrspace(4) [[CAST]], ptr addrspace(4) null
  int* a = NULL;
  __private int* prv_ptr = to_private(a);
  int* generic_ptr = (__generic int*) prv_ptr;
  bool test = (generic_ptr == prv_ptr);
}
