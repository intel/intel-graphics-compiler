;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-replace-unsupported-intrinsics -verify -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

target triple = "igil_32_GEN12"

define void @A(i8 addrspace(1)* %B, i32 %S, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
entry:
; CHECK-LABEL: define void @A
; CHECK:  %pIV = alloca i32
; CHECK:  %0 = icmp ugt i32 %S, 1
; CHECK:  %umax = select i1 %0, i32 %S, i32 1
; CHECK-TYPED:  store i32 0, i32* %pIV
; CHECK-OPAQUE:  store i32 0, ptr %pIV
; CHECK:  [[CMP0:%[a-zA-Z0-9]+]] = icmp ult i32 0, %umax
; CHECK:  br i1 [[CMP0]], label %memset.body, label %memset.post
; CHECK: memset.body:
; CHECK-TYPED:   %IV = load i32, i32* %pIV
; CHECK-OPAQUE:   %IV = load i32, ptr %pIV
; CHECK-TYPED:   [[GEP0:%[a-zA-Z0-9]+]] = getelementptr i8, i8 addrspace(1)* %B, i32 %IV
; CHECK-OPAQUE:   [[GEP0:%[a-zA-Z0-9]+]] = getelementptr i8, ptr addrspace(1) %B, i32 %IV
; CHECK-TYPED:   store i8 42, i8 addrspace(1)* [[GEP0]], align 1
; CHECK-OPAQUE:   store i8 42, ptr addrspace(1) [[GEP0]], align 1
; CHECK:   [[INC0:%[a-zA-Z0-9]+]] = add i32 %IV, 1
; CHECK-TYPED:   store i32 [[INC0]], i32* %pIV
; CHECK-OPAQUE:   store i32 [[INC0]], ptr %pIV
; CHECK:   [[CMP1:%[a-zA-Z0-9]+]] = icmp ult i32 [[INC0]], %umax
; CHECK:   br i1 [[CMP1]], label %memset.body, label %memset.post
; CHECK: memset.post:
; CHECK: ret
  %0 = icmp ugt i32 %S, 1
  %umax = select i1 %0, i32 %S, i32 1
  call void @llvm.memset.p1i8.i32(i8 addrspace(1)* %B, i8 42, i32 %umax, i32 1, i1 false)
  ret void
}

define void @B(i8 addrspace(1)* %B, i32 %S, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
entry:
; CHECK-LABEL: define void @B
; CHECK-NOT: call
; CHECK: ret
  call void @llvm.memset.p1i8.i32(i8 addrspace(1)* %B, i8 42, i32 %S, i32 1, i1 false)
  ret void
}

define void @C(i8 addrspace(1)* %B, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
entry:
; CHECK-LABEL: define void @C
; CHECK-NOT: call
; CHECK: ret
  call void @llvm.memset.p1i8.i32(i8 addrspace(1)* %B, i8 42, i32 1, i32 1, i1 false)
  ret void
}

declare void @llvm.memset.p1i8.i32(i8 addrspace(1)* nocapture, i8, i32, i32, i1) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind }
