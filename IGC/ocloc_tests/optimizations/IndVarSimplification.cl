/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys,pvc-supported,llvm-16-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnableOpaquePointersBackend=1 EnableIndVarSimplification=1,PrintToConsole=1,PrintAfter=EmitPass'" 2>&1 | FileCheck %s

// CHECK-NOT: phi
// CHECK: [[LOAD:%.*]] = load i32, ptr addrspace(1) [[PTR:%.*]]
// CHECK: [[ADD:%.*]] = add i32 [[LOAD]], 10000
// CHECK: store i32 [[ADD]], ptr addrspace(1) [[PTR]]

kernel void test(global int* ptr)
{
    size_t id = get_global_id(0);
    int val = ptr[id];
    for (int i = 0; i < 10000; i++)
    {
        val += 1;
    }
    ptr[id] = val;
}
