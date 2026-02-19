;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

target triple = "igil_32_GEN9"

%struct.data_t = type { [207 x i16] }

; Function Attrs: nounwind
define void @test_kernel(%struct.data_t addrspace(1)* %p, %struct.data_t addrspace(1)* %q) #0 {
; CHECK: [[pIV:%[a-zA-Z0-9_]+]] = alloca i32
; CHECK: [[vSrc:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to <16 x i16> addrspace(1)*
; CHECK: [[vDst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to <16 x i16> addrspace(1)*
; CHECK: memcpy.body:
; CHECK:   [[IV:%[a-zA-Z0-9_]+]] = load i32, i32* [[pIV]]
; CHECK:   [[GEP0:%[a-zA-Z0-9_]+]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[vSrc]], i32 [[IV]]
; CHECK:   [[GEP1:%[a-zA-Z0-9_]+]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[vDst]], i32 [[IV]]
; CHECK:   [[LD:%[a-zA-Z0-9_]+]] = load <16 x i16>, <16 x i16> addrspace(1)* [[GEP0]], align 2
; CHECK:   store <16 x i16> [[LD]], <16 x i16> addrspace(1)* [[GEP1]], align 2
; CHECK:   [[INC0:%[a-zA-Z0-9_]+]] = add i32 [[IV]], 1
; CHECK:   store i32 [[INC0]], i32* [[pIV]]
; CHECK:   [[CMP:%[a-zA-Z0-9_]+]] = icmp ult i32 [[INC0]], 12
; CHECK:   br i1 [[CMP]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.post:
; CHECK:   [[memcpy_src:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to i8 addrspace(1)*
; CHECK:   [[memcpy_dst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to i8 addrspace(1)*
; CHECK:   [[GEP2:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 384
; CHECK:   [[GEP3:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 384
; CHECK:   [[REM0:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP2]] to <8 x i16> addrspace(1)*
; CHECK:   [[REM1:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP3]] to <8 x i16> addrspace(1)*
; CHECK:   [[L0:%[_a-zA-Z0-9_]+]] = load <8 x i16>, <8 x i16> addrspace(1)* [[REM0]], align 2
; CHECK:   store <8 x i16> [[L0]], <8 x i16> addrspace(1)* [[REM1]], align 2
; CHECK:   [[GEP4:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 400
; CHECK:   [[GEP5:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 400
; CHECK:   [[REM2:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP4]] to <4 x i16> addrspace(1)*
; CHECK:   [[REM3:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP5]] to <4 x i16> addrspace(1)*
; CHECK:   [[L1:%[_a-zA-Z0-9_]+]] = load <4 x i16>, <4 x i16> addrspace(1)* [[REM2]], align 2
; CHECK:   store <4 x i16> [[L1]], <4 x i16> addrspace(1)* [[REM3]], align 2
; CHECK:   [[GEP6:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 408
; CHECK:   [[GEP7:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 408
; CHECK:   [[REM4:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP6]] to <2 x i16> addrspace(1)*
; CHECK:   [[REM5:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP7]] to <2 x i16> addrspace(1)*
; CHECK:   [[L2:%[_a-zA-Z0-9_]+]] = load <2 x i16>, <2 x i16> addrspace(1)* [[REM4]], align 2
; CHECK:   store <2 x i16> [[L2]], <2 x i16> addrspace(1)* [[REM5]], align 2
; CHECK:   [[GEP8:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 412
; CHECK:   [[GEP9:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 412
; CHECK:   [[REM6:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP8]] to i16 addrspace(1)*
; CHECK:   [[REM7:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP9]] to i16 addrspace(1)*
; CHECK:   [[L3:%[_a-zA-Z0-9_]+]] = load i16, i16 addrspace(1)* [[REM6]], align 2
; CHECK:   store i16 [[L3]], i16 addrspace(1)* [[REM7]], align 2
; CHECK:   ret void
  %Dst = bitcast %struct.data_t addrspace(1)* %p to i8 addrspace(1)*
  %Src = bitcast %struct.data_t addrspace(1)* %q to i8 addrspace(1)*
  call void @llvm.memcpy.p1i8.p1i8.i32(i8 addrspace(1)* %Dst, i8 addrspace(1)* %Src, i32 414, i32 2, i1 false)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p1i8.p1i8.i32(i8 addrspace(1)* nocapture, i8 addrspace(1)* nocapture, i32, i32, i1) #0

attributes #0 = { nounwind }
