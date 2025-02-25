/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: sys32
// REQUIRES: regkeys, pvc-supported, llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'PrintToConsole=1,PrintAfter=Layout'" 2>&1 | FileCheck %s

// When get_global_id is truncated to i32, the code can be optimized to use 32-bit arithmetic.
// Output should not contain any 64-bit arithmetic operations (except add offset to pointer).
//
// CHECK-LABEL: define spir_kernel void @get_global_id_int32
// CHECK-NOT:     mul i64
// CHECK-NOT:     llvm.genx.GenISA.mul.pair
// CHECK-COUNT-1: mul i32
// CHECK-COUNT-2: add{{( .*)?}} i32
// CHECK:         [[ADDR:%.*]] = add{{( .*)?}} i64
// CHECK-NOT:     mul i64
// CHECK-NOT:     llvm.genx.GenISA.mul.pair
// CHECK-NOT:     add{{( .*)?}} i64
// CHECK:         inttoptr i64 [[ADDR]] to i32 addrspace(1)*
// CHECK:         ret void
kernel void get_global_id_int32(global int* ptr)
{
    const int id = get_global_id(0);
    ptr[id] = 39;
}

// When get_global_id is used as i64 value, it can't be optimized. All arithmetic operations should be 64-bit.
//
// CHECK-LABEL: define spir_kernel void @get_global_id_int64
// CHECK-NOT:     mul i32
// CHECK-NOT:     add{{( .*)?}} i32
// CHECK:         llvm.genx.GenISA.mul.pair
// CHECK-NOT:     mul i32
// CHECK-NOT:     add{{( .*)?}} i32
// CHECK-COUNT-3: add{{( .*)?}} i64
// CHECK:         ret void
kernel void get_global_id_int64(global int* ptr)
{
    const long id = get_global_id(0);
    ptr[id] = 39;
}
