;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-fix-alignment -S < %s 2>&1 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

@globalConstantZero = internal unnamed_addr addrspace(2) constant [8 x i8] zeroinitializer
@globalNonconstantZero = internal unnamed_addr addrspace(2) global [8 x i8] zeroinitializer

; memcpy from a zeroinitialized constant global is equivalent to memset to zero.
; In this case, we can set the alignment of memcpy based only on destination pointer.
;
; CHECK-LABEL: @memcpy_from_global_constant_zero(
; CHECK:         call void @llvm.memcpy.p0i8.p2i8.i64(i8* align 8 %2, i8 addrspace(2)* %3, i64 8, i1 false)
define void @memcpy_from_global_constant_zero() {
entry:
  %0 = alloca [5 x [6 x double]], align 8
  %1 = getelementptr inbounds [5 x [6 x double]], [5 x [6 x double]]* %0, i64 0, i64 0, i64 0
  %2 = bitcast double* %1 to i8*
  %3 = getelementptr inbounds [8 x i8], [8 x i8] addrspace(2)* @globalConstantZero, i32 0, i32 0
  call void @llvm.memcpy.p0i8.p2i8.i64(i8* %2, i8 addrspace(2)* %3, i64 8, i1 false)
  ret void
}

; memcpy from a non-constant global has to be aligned based on both source and destination pointers.
;
; CHECK-LABEL: @memcpy_from_global_nonconstant_zero(
; CHECK:         call void @llvm.memcpy.p0i8.p2i8.i64(i8* align 1 %2, i8 addrspace(2)* %3, i64 8, i1 false)
define void @memcpy_from_global_nonconstant_zero() {
entry:
  %0 = alloca [5 x [6 x double]], align 8
  %1 = getelementptr inbounds [5 x [6 x double]], [5 x [6 x double]]* %0, i64 0, i64 0, i64 0
  %2 = bitcast double* %1 to i8*
  %3 = getelementptr inbounds [8 x i8], [8 x i8] addrspace(2)* @globalNonconstantZero, i32 0, i32 0
  call void @llvm.memcpy.p0i8.p2i8.i64(i8* %2, i8 addrspace(2)* %3, i64 8, i1 false)
  ret void
}

declare void @llvm.memcpy.p0i8.p2i8.i64(i8* noalias nocapture writeonly, i8 addrspace(2)* noalias nocapture readonly, i64, i1 immarg)
