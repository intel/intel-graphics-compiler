;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

target triple = "igil_32_GEN9"

%struct.data_t = type { [98 x <2 x i8>] }

; Function Attrs: nounwind
define void @test_kernel(%struct.data_t addrspace(1)* %p, %struct.data_t addrspace(1)* %q) #0 {
; CHECK: [[pIV:%[a-zA-Z0-9_]+]] = alloca i32
; CHECK: [[vSrc:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to <8 x i32> addrspace(1)*
; CHECK: [[vDst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to <8 x i32> addrspace(1)*
; CHECK: memcpy.body:
; CHECK:   [[IV:%[a-zA-Z0-9_]+]] = load i32, i32* [[pIV]]
; CHECK:   [[GEP0:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[vSrc]], i32 [[IV]]
; CHECK:   [[GEP1:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[vDst]], i32 [[IV]]
; CHECK:   [[LD:%[a-zA-Z0-9_]+]] = load <8 x i32>, <8 x i32> addrspace(1)* [[GEP0]], align 2
; CHECK:   store <8 x i32> [[LD]], <8 x i32> addrspace(1)* [[GEP1]], align 2
; CHECK:   [[INC0:%[a-zA-Z0-9_]+]] = add i32 [[IV]], 1
; CHECK:   store i32 [[INC0]], i32* [[pIV]]
; CHECK:   [[CMP:%[a-zA-Z0-9_]+]] = icmp ult i32 [[INC0]], 6
; CHECK:   br i1 [[CMP]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.post:
; CHECK:   [[memcpy_src:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to i8 addrspace(1)*
; CHECK:   [[memcpy_dst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to i8 addrspace(1)*
; CHECK:   [[GEP2:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 192
; CHECK:   [[GEP3:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 192
; CHECK:   [[PTR0:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP2]] to i32 addrspace(1)*
; CHECK:   [[PTR1:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP3]] to i32 addrspace(1)*
; CHECK:   [[L0:%[_a-zA-Z0-9_]+]] = load i32, i32 addrspace(1)* [[PTR0]], align 2
; CHECK:   store i32 [[L0]], i32 addrspace(1)* [[PTR1]], align 2
; CHECK:   ret void
  %Dst = bitcast %struct.data_t addrspace(1)* %p to i8 addrspace(1)*
  %Src = bitcast %struct.data_t addrspace(1)* %q to i8 addrspace(1)*
  call void @llvm.memcpy.p1i8.p1i8.i32(i8 addrspace(1)* %Dst, i8 addrspace(1)* %Src, i32 196, i32 2, i1 false)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p1i8.p1i8.i32(i8 addrspace(1)* nocapture, i8 addrspace(1)* nocapture, i32, i32, i1) #0

attributes #0 = { nounwind }

