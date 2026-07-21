;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s

; A SYCL kernel-argument linearization buffer stores USM device pointers
; (ptr addrspace(4)) into a byte array through i8 GEPs. The promotion
; "transpose" logic sizes vector elements with Type::getScalarSizeInBits(),
; which returns 0 for pointer types. Promoting such an alloca therefore used to
; crash with a division/modulo by zero (SIGFPE). GenXPromoteArray must instead
; reject the alloca and leave it in memory, unchanged.

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"

define dllexport spir_kernel void @store_data_ptr_no_promotion(i32 %arg0, ptr addrspace(4) %arg8, ptr addrspace(4) %arg16, ptr addrspace(4) %out) {
entry:
; CHECK-LABEL: @store_data_ptr_no_promotion
; CHECK: %[[ALLOCA:[^ ]+]] = alloca <24 x i8>
; CHECK-NEXT: store i32 %arg0, ptr %[[ALLOCA]]
; CHECK-NEXT: %[[IDX8:[^ ]+]] = getelementptr inbounds i8, ptr %[[ALLOCA]], i64 8
; CHECK-NEXT: store ptr addrspace(4) %arg8, ptr %[[IDX8]]
; CHECK-NEXT: %[[IDX16:[^ ]+]] = getelementptr inbounds i8, ptr %[[ALLOCA]], i64 16
; CHECK-NEXT: store ptr addrspace(4) %arg16, ptr %[[IDX16]]
; CHECK-NEXT: %[[LOAD:[^ ]+]] = load <24 x i8>, ptr %[[ALLOCA]]
; CHECK-NEXT: store <24 x i8> %[[LOAD]], ptr addrspace(4) %out
; CHECK-NEXT: ret void
  %linearization = alloca <24 x i8>, align 32
  store i32 %arg0, ptr %linearization, align 32
  %idx8 = getelementptr inbounds i8, ptr %linearization, i64 8
  store ptr addrspace(4) %arg8, ptr %idx8, align 8
  %idx16 = getelementptr inbounds i8, ptr %linearization, i64 16
  store ptr addrspace(4) %arg16, ptr %idx16, align 16
  %load = load <24 x i8>, ptr %linearization, align 32
  store <24 x i8> %load, ptr addrspace(4) %out, align 32
  ret void
}
