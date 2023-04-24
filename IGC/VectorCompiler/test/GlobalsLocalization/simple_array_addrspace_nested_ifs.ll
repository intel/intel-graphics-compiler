;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; COM: global variables in non-private addrspaces aren't localized
@simple_global_array = internal addrspace(2) constant [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4
; CHECK: @simple_global_array = internal addrspace(2) constant [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4

; Function Attrs: noinline nounwind
define dllexport void @simple_array(i64 %provided.offset) {
entry:
  %ptr = getelementptr inbounds [8 x i32], [8 x i32] addrspace(2)* @simple_global_array, i64 0, i64 %provided.offset
; CHECK: %ptr = getelementptr inbounds [8 x i32], [8 x i32] addrspace(2)* @simple_global_array, i64 0, i64 %provided.offset
  %level.0.cmp = icmp slt i64 %provided.offset, 4
  br i1 %level.0.cmp, label %level.1.less.4, label %level.1.greater.equal.4

level.1.less.4:               ; preds = %entry
  %level.1.ptr.cast = bitcast i32 addrspace(2)* %ptr to i8 addrspace(2)*
; CHECK-DAG: %level.1.ptr.cast = bitcast i32 addrspace(2)* %ptr to i8 addrspace(2)*
  %level.1.less.4.cmp = icmp slt i64 %provided.offset, -1
  br i1 %level.1.less.4.cmp, label %level.2.less.min.1, label %level.2.greater.equal.min.1

level.2.less.min.1:           ; preds = %level.1.less.4
  %level.2.val = load i8, i8 addrspace(2)* %level.1.ptr.cast, align 4
; CHECK-DAG: %level.2.val = load i8, i8 addrspace(2)* %level.1.ptr.cast, align 4
  %level.2.val.use = add i8 %level.2.val, 1
; CHECK-DAG: %level.2.val.use = add i8 %level.2.val, 1
  br label %exit

level.2.greater.equal.min.1:  ; preds = %level.1.less.4
  %level.2.ptr.cast.back = bitcast i8 addrspace(2)* %level.1.ptr.cast to i32 addrspace(2)*
; CHECK-DAG: %level.2.ptr.cast.back = bitcast i8 addrspace(2)* %level.1.ptr.cast to i32 addrspace(2)*
  br label %exit

level.1.greater.equal.4:      ; preds = %entry
  %shifted.offset = add nsw i64 %provided.offset, -1
  %level.1.ptr = getelementptr inbounds [8 x i32], [8 x i32] addrspace(2)* @simple_global_array, i64 0, i64 %shifted.offset
; CHECK-DAG: %level.1.ptr = getelementptr inbounds [8 x i32], [8 x i32] addrspace(2)* @simple_global_array, i64 0, i64 %shifted.offset
  %level.1.greater.equal.4.cmp = icmp sgt i64 %provided.offset, 7
  br i1 %level.1.greater.equal.4.cmp, label %level.2.greater.7, label %level.2.less.equal.7

level.2.less.equal.7:         ; preds = %level.1.greater.equal.4
  %level.2.less.equal.7.val = load i32, i32 addrspace(2)* %level.1.ptr, align 4
; CHECK-DAG: %level.2.less.equal.7.val = load i32, i32 addrspace(2)* %level.1.ptr, align 4
  %level.2.less.equal.7.val.use = add i32 %level.2.less.equal.7.val, 1
; CHECK-DAG: %level.2.less.equal.7.val.use = add i32 %level.2.less.equal.7.val, 1
  br label %exit

level.2.greater.7:            ; preds = %level.1.greater.equal.4
  %level.2.greater.7.ptr.cast = bitcast i32 addrspace(2)* %level.1.ptr to i8 addrspace(2)*
; CHECK-DAG: %level.2.greater.7.ptr.cast = bitcast i32 addrspace(2)* %level.1.ptr to i8 addrspace(2)*
  %level.2.greater.7.val = load i8, i8 addrspace(2)* %level.2.greater.7.ptr.cast, align 4
; CHEKC-DAG: %level.2.greater.7.val = load i8, i8 addrspace(2)* %level.2.greater.7.ptr.cast, align 4
  %level.2.greater.7.val.use = add i8 %level.2.greater.7.val, 1
; CHECK-DAG: %level.2.greater.7.val.use = add i8 %level.2.greater.7.val, 1
  br label %exit

exit:                         ; preds = %level.2.greater.7, %level.2.less.equal.7, %level.2.greater.equal.min.1, %level.2.less.min.1
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (i64)* @simple_array}
