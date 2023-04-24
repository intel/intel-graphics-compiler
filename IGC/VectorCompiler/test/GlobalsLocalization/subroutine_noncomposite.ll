;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@simple_global_int = internal global i32 42, align 4
@simple_const_float = internal constant float 4.200000e+01, align 4

; Function Attrs: noinline nounwind
define internal spir_func float @bar() {
; CHECK: define internal spir_func { float, i32 } @bar(i32 %simple_global_int.in, float %simple_const_float.in) {
  %simple_global_int.load = load i32, i32* @simple_global_int, align 4
  %simple_global_int.inc = add nsw i32 %simple_global_int.load, 1
  store i32 %simple_global_int.inc, i32* @simple_global_int, align 4
  %simple_const_float.load = load float, float* @simple_const_float, align 4
  %simple_const_float.inc = fadd float %simple_const_float.load, 1.000000e+00
  ret float %simple_const_float.inc
; COM: no need to copy-out constant float
; CHECK: ret { float, i32 } %{{[^ ]+}}
}

; Function Attrs: noinline nounwind
define dllexport void @foo_kernel() {
; CHECK: %simple_const_float.local = alloca float, align 4
; CHECK: store float 4.200000e+01, float* %simple_const_float.local
; CHECK: %simple_global_int.local = alloca i32, align 4
; CHECK: store i32 42, i32* %simple_global_int.local
  %ret.val = call spir_func float @bar()
; COM: no need to store into a constant float
; CHECK-NOT: store float %{{[^ ]+}}, float* %simple_const_float.local
  %just.use = fadd float %ret.val, 1.000000e+00
  ret void
}

!genx.kernels = !{!0}
!0 = !{void ()* @foo_kernel}
