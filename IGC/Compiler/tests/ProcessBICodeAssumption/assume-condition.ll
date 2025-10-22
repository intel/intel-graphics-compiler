;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-process-bi-code-assumption -S < %s 2>&1 | FileCheck %s

; Test different llvm.assume conditions.

; CHECK-LABEL: @ule_4294967295
; CHECK-NEXT:  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:  %2 = trunc i64 %1 to i32
; CHECK-NEXT:  %3 = zext i32 %2 to i64
; CHECK-NEXT:  %4 = icmp ule i64 %3, 4294967295
; CHECK-NEXT:  call void @llvm.assume(i1 %4)
; CHECK-NEXT:  ret void
define void @ule_4294967295() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = icmp ule i64 %1, 4294967295
  call void @llvm.assume(i1 %2)
  ret void
}

; CHECK-LABEL: @ule_4294967296
; CHECK-NEXT:  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:  %2 = icmp ule i64 %1, 4294967296
; CHECK-NEXT:  call void @llvm.assume(i1 %2)
; CHECK-NEXT:  ret void
define void @ule_4294967296() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = icmp ule i64 %1, 4294967296
  call void @llvm.assume(i1 %2)
  ret void
}

; CHECK-LABEL: @ult_4294967296
; CHECK-NEXT:  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:  %2 = trunc i64 %1 to i32
; CHECK-NEXT:  %3 = zext i32 %2 to i64
; CHECK-NEXT:  %4 = icmp ult i64 %3, 4294967296
; CHECK-NEXT:  call void @llvm.assume(i1 %4)
; CHECK-NEXT:  ret void
define void @ult_4294967296() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = icmp ult i64 %1, 4294967296
  call void @llvm.assume(i1 %2)
  ret void
}

; CHECK-LABEL: @ult_4294967297
; CHECK-NEXT:  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:  %2 = icmp ult i64 %1, 4294967297
; CHECK-NEXT:  call void @llvm.assume(i1 %2)
; CHECK-NEXT:  ret void
define void @ult_4294967297() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = icmp ult i64 %1, 4294967297
  call void @llvm.assume(i1 %2)
  ret void
}

; CHECK-LABEL: @sle_2147483647
; CHECK-NEXT:  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:  %2 = trunc i64 %1 to i32
; CHECK-NEXT:  %3 = zext i32 %2 to i64
; CHECK-NEXT:  %4 = icmp sle i64 %3, 2147483647
; CHECK-NEXT:  call void @llvm.assume(i1 %4)
; CHECK-NEXT:  ret void
define void @sle_2147483647() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = icmp sle i64 %1, 2147483647
  call void @llvm.assume(i1 %2)
  ret void
}

; CHECK-LABEL: @sle_2147483648
; CHECK-NEXT:  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:  %2 = icmp sle i64 %1, 2147483648
; CHECK-NEXT:  call void @llvm.assume(i1 %2)
; CHECK-NEXT:  ret void
define void @sle_2147483648() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = icmp sle i64 %1, 2147483648
  call void @llvm.assume(i1 %2)
  ret void
}

; CHECK-LABEL: @slt_2147483648
; CHECK-NEXT:  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:  %2 = trunc i64 %1 to i32
; CHECK-NEXT:  %3 = zext i32 %2 to i64
; CHECK-NEXT:  %4 = icmp slt i64 %3, 2147483648
; CHECK-NEXT:  call void @llvm.assume(i1 %4)
; CHECK-NEXT:  ret void
define void @slt_2147483648() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = icmp slt i64 %1, 2147483648
  call void @llvm.assume(i1 %2)
  ret void
}

; CHECK-LABEL: @slt_2147483649
; CHECK-NEXT:  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:  %2 = icmp slt i64 %1, 2147483649
; CHECK-NEXT:  call void @llvm.assume(i1 %2)
; CHECK-NEXT:  ret void
define void @slt_2147483649() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = icmp slt i64 %1, 2147483649
  call void @llvm.assume(i1 %2)
  ret void
}

declare void @llvm.assume(i1)
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)
