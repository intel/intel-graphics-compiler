;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

target triple = "igil_32_GEN8"

define void @A(i8 addrspace(1)* %Src, i8 addrspace(3)* %Dst, i32 %S, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
entry:
; CHECK-LABEL: A
; CHECK:  %pIV = alloca i32
; CHECK:  %0 = icmp ugt i32 %S, 1
; CHECK:  %umax = select i1 %0, i32 %S, i32 1
; CHECK:  store i32 0, i32* %pIV
; CHECK:  [[CMP0:%[a-zA-Z0-9]+]] = icmp ult i32 0, %umax
; CHECK:  br i1 [[CMP0]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.body:
; CHECK:   %IV = load i32, i32* %pIV
; CHECK:   [[GEP0:%[a-zA-Z0-9]+]] = getelementptr i8, i8 addrspace(1)* %Src, i32 %IV
; CHECK:   [[GEP1:%[a-zA-Z0-9]+]] = getelementptr i8, i8 addrspace(3)* %Dst, i32 %IV
; CHECK:   [[LD:%[a-zA-Z0-9]+]] = load i8, i8 addrspace(1)* [[GEP0]], align 1
; CHECK:   store i8 [[LD]], i8 addrspace(3)* [[GEP1]], align 1
; CHECK:   [[INC0:%[a-zA-Z0-9]+]] = add i32 %IV, 1
; CHECK:   store i32 [[INC0]], i32* %pIV
; CHECK:   [[CMP1:%[a-zA-Z0-9]+]] = icmp ult i32 [[INC0]], %umax
; CHECK:   br i1 [[CMP1]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.post:
; CHECK: ret
  %0 = icmp ugt i32 %S, 1
  %umax = select i1 %0, i32 %S, i32 1
  call void @llvm.memcpy.p0i8.p0i8.i32(i8 addrspace(3)* %Dst, i8 addrspace(1)* %Src, i32 %umax, i32 1, i1 false)
  ret void
}

declare void @llvm.memcpy.p0i8.p0i8.i32(i8 addrspace(3)* nocapture, i8 addrspace(1)* nocapture, i32, i32, i1) #0

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind }
