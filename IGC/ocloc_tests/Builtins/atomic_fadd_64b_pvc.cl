/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported, arl-supported, llvm-16-plus

// RUN: ocloc compile -file %s -device arl -options "-cl-std=CL2.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintAfter=EmitPass'" 2>&1 | FileCheck %s --check-prefix=CHECK-BASE
// RUN: ocloc compile -file %s -device pvc -options "-cl-std=CL2.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintAfter=EmitPass'" 2>&1 | FileCheck %s --check-prefix=CHECK-PVC

// The things which should be in the compiled module for each of variant for AtomicFAddEXT
// CHECK-BASE: @spinlock = addrspace(3) global i32 0, section "localSLM"
// CHECK-PVC: i64 @llvm.genx.GenISA.icmpxchgatomicraw.i64.p3.i32

// The things which should not be in the compiled module for each of variant for AtomicFAddEXT
// CHECK-BASE-NOT: i64 @llvm.genx.GenISA.icmpxchgatomicraw.i64.p3.i32
// CHECK-PVC-NOT: @spinlock = addrspace(3) global i32 0, section "localSLM"

#define SPIRV_OVERLOADABLE __attribute__((overloadable))
#define SPIRV_BUILTIN(opcode, old_mangling, new_mangling) \
    __spirv_##opcode##new_mangling

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p3f64_i32_i32_f64, )(local double *Pointer, int Scope, int Semantics, double Value);

double test_atomic(__local double* array)
{
    return SPIRV_BUILTIN(AtomicFAddEXT, _p3f64_i32_i32_f64, )(array, 1, 0, 123.0L);
}

kernel void test(__global double* out)
{
    __local double tmp;
    *out = test_atomic(&tmp);
}
