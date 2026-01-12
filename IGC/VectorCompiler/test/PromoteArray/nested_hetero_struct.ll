;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

%struct_t = type { %struct_1_t, i64 }
%struct_1_t = type { %struct_x_2_t, %struct_y_2_t }
%struct_x_2_t = type { %struct_x_3_t }
%struct_x_3_t = type { i64 }
%struct_y_2_t = type { i32 addrspace(4)*, %struct_y_3_t }
%struct_y_3_t = type { %struct_x_2_t, %struct_x_3_t }

; CHECK-LABEL: kernel
define spir_kernel void @kernel() {
; CHECK: %object = alloca %struct_t, align 8
  %object = alloca %struct_t, align 8
; CHECK-TYPED-PTRS: %sptr = getelementptr inbounds %struct_t, %struct_t* %object, i64 0, i32 0, i32 1
; CHECK-OPAQUE-PTRS: %sptr = getelementptr inbounds %struct_t, ptr %object, i64 0, i32 0, i32 1
  %sptr = getelementptr inbounds %struct_t, %struct_t* %object, i64 0, i32 0, i32 1
; CHECK-TYPED-PTRS: %cast = bitcast %struct_y_2_t* %sptr to <24 x i8>*
; CHECK-OPAQUE-PTRS: %cast = bitcast ptr %sptr to ptr
  %cast = bitcast %struct_y_2_t* %sptr to <24 x i8>*
; CHECK-TYPED-PTRS: store <24 x i8> zeroinitializer, <24 x i8>* %cast, align 8
; CHECK-OPAQUE-PTRS: store <24 x i8> zeroinitializer, ptr %cast, align 8
  store <24 x i8> zeroinitializer, <24 x i8>* %cast, align 8
  ret void
}
