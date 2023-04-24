;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@simple_global_array = internal global [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4

; Function Attrs: noinline nounwind
define internal spir_func i32 @bar(i64 %passed.offset) {
; CHECK: define internal spir_func i32 @bar(i64 %passed.offset, [8 x i32]* %simple_global_array.in) {
  %elem.ptr = getelementptr inbounds [8 x i32], [8 x i32]* @simple_global_array, i64 0, i64 %passed.offset
; CHECK: %elem.ptr = getelementptr inbounds [8 x i32], [8 x i32]* %simple_global_array.in, i64 0, i64 %passed.offset
  %elem = load i32, i32* %elem.ptr, align 4
  ret i32 %elem
}

; Function Attrs: noinline nounwind
define dllexport void @foo_kernel(i64 %offset) {
; CHECK: %simple_global_array.local = alloca [8 x i32], align 4
; CHECK: store [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], [8 x i32]* %simple_global_array.local
  %ret.val = call spir_func i32 @bar(i64 %offset)
; CHECK: %ret.val = call spir_func i32 @bar(i64 %offset, [8 x i32]* %simple_global_array.local)
  %just.use = add nsw i32 %ret.val, 1
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (i64)* @foo_kernel}
