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
; COM: @simple_global_array should be mentioned only once. Global DCE will remove it.
; CHECK: @simple_global_array
; CHECK-NOT: @simple_global_array

; Function Attrs: noinline nounwind
define dllexport void @simple_array(i64 %provided_offset) {
; CHECK: %simple_global_array.local = alloca [8 x i32], align 4
; CHECK-NEXT: store [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], [8 x i32]* %simple_global_array.local
  %ptr = getelementptr inbounds [8 x i32], [8 x i32]* @simple_global_array, i64 0, i64 %provided_offset
; CHECK-NEXT: %ptr = getelementptr inbounds [8 x i32], [8 x i32]* %simple_global_array.local, i64 0, i64 %provided_offset
  %val = load i32, i32* %ptr, align 4
; CHECK-NEXT: %val = load i32, i32* %ptr, align 4
; CHECK-NOT: @simple_global_array
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (i64)* @simple_array}
