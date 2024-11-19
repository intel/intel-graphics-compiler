;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% %pass_pref%CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% %pass_pref%CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; COM: global variables in non-private addrspaces aren't localized
@simple_global_array = internal addrspace(2) constant [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4
; CHECK: @simple_global_array = internal addrspace(2) constant [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4

; Function Attrs: noinline nounwind
define dllexport void @simple_array(i64 %provided_offset) {
  %ptr = getelementptr inbounds [8 x i32], [8 x i32] addrspace(2)* @simple_global_array, i64 0, i64 %provided_offset
; CHECK-TYPED-PTRS: %ptr = getelementptr inbounds [8 x i32], [8 x i32] addrspace(2)* @simple_global_array, i64 0, i64 %provided_offset
; CHECK-OPAQUE-PTRS: %ptr = getelementptr inbounds [8 x i32], ptr addrspace(2) @simple_global_array, i64 0, i64 %provided_offset
  %ptr.cast = bitcast i32 addrspace(2)* %ptr to i8 addrspace(2)*
; CHECK-TYPED-PTRS: %ptr.cast = bitcast i32 addrspace(2)* %ptr to i8 addrspace(2)*
; CHECK-OPAQUE-PTRS: %ptr.cast = bitcast ptr addrspace(2) %ptr to ptr addrspace(2)
  %val = load i8, i8 addrspace(2)* %ptr.cast, align 4
; CHECK-TYPED-PTRS: %val = load i8, i8 addrspace(2)* %ptr.cast, align 4
; CHECK-OPAQUE-PTRS: %val = load i8, ptr addrspace(2) %ptr.cast, align 4
  %val.use = add i8 %val, 1
; CHECK: %val.use = add i8 %val, 1
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (i64)* @simple_array}
