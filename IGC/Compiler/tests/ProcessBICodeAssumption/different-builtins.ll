;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-process-bi-code-assumption -S < %s 2>&1 | FileCheck %s

; Test different builtins.

; CHECK-LABEL: @get_global_id
; CHECK-NEXT:  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK-NEXT:  %2 = trunc i64 %1 to i32
; CHECK-NEXT:  %3 = zext i32 %2 to i64
; CHECK-NEXT:  %4 = icmp ult i64 %3, 2147483648
; CHECK-NEXT:  call void @llvm.assume(i1 %4)
; CHECK-NEXT:  ret void
define void @get_global_id() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %2 = icmp ult i64 %1, 2147483648
  call void @llvm.assume(i1 %2)
  ret void
}

; CHECK-LABEL: @get_global_linear_id
; CHECK-NEXT:  %1 = call spir_func i64 @_Z29__spirv_BuiltInGlobalLinearIdv()
; CHECK-NEXT:  %2 = trunc i64 %1 to i32
; CHECK-NEXT:  %3 = zext i32 %2 to i64
; CHECK-NEXT:  %4 = icmp ult i64 %3, 2147483648
; CHECK-NEXT:  call void @llvm.assume(i1 %4)
; CHECK-NEXT:  ret void
define void @get_global_linear_id() {
  %1 = call spir_func i64 @_Z29__spirv_BuiltInGlobalLinearIdv()
  %2 = icmp ult i64 %1, 2147483648
  call void @llvm.assume(i1 %2)
  ret void
}

declare void @llvm.assume(i1)
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)
declare spir_func i64 @_Z29__spirv_BuiltInGlobalLinearIdv()
