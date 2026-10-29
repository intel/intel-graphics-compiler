;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t.opaque %}
; RUN: %if llvm-16-plus %{ FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s < %t.opaque %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t.typed %}
; RUN: %if !llvm-17-plus %{ FileCheck --check-prefixes=CHECK,CHECK-TYPED %s < %t.typed %}

target triple = "igil_32_GEN12"

%struct.data_t = type { [207 x i16] }

; Function Attrs: nounwind
define void @test_kernel(%struct.data_t addrspace(1)* %p, %struct.data_t addrspace(1)* %q) #0 {
; CHECK: [[pIV:%[a-zA-Z0-9_]+]] = alloca i32
; CHECK-TYPED: [[vSrc:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to <16 x i16> addrspace(1)*
; CHECK-TYPED: [[vDst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to <16 x i16> addrspace(1)*
; CHECK: memcpy.body:
; CHECK-TYPED:   [[IV:%[a-zA-Z0-9_]+]] = load i32, i32* [[pIV]]
; CHECK-OPAQUE:   [[IV:%[a-zA-Z0-9_]+]] = load i32, ptr [[pIV]]
; CHECK-TYPED:   [[GEP0:%[a-zA-Z0-9_]+]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[vSrc]], i32 [[IV]]
; CHECK-OPAQUE:   [[GEP0:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, ptr addrspace(1) %q, i32 [[IV]]
; CHECK-TYPED:   [[GEP1:%[a-zA-Z0-9_]+]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[vDst]], i32 [[IV]]
; CHECK-OPAQUE:   [[GEP1:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, ptr addrspace(1) %p, i32 [[IV]]
; CHECK-TYPED:   [[LD:%[a-zA-Z0-9_]+]] = load <16 x i16>, <16 x i16> addrspace(1)* [[GEP0]], align 2
; CHECK-OPAQUE:   [[LD:%[a-zA-Z0-9_]+]] = load <8 x i32>, ptr addrspace(1) [[GEP0]], align 2
; CHECK-TYPED:   store <16 x i16> [[LD]], <16 x i16> addrspace(1)* [[GEP1]], align 2
; CHECK-OPAQUE:   store <8 x i32> [[LD]], ptr addrspace(1) [[GEP1]], align 2
; CHECK:   [[INC0:%[a-zA-Z0-9_]+]] = add i32 [[IV]], 1
; CHECK-TYPED:   store i32 [[INC0]], i32* [[pIV]]
; CHECK-OPAQUE:   store i32 [[INC0]], ptr [[pIV]]
; CHECK:   [[CMP:%[a-zA-Z0-9_]+]] = icmp ult i32 [[INC0]], 12
; CHECK:   br i1 [[CMP]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.post:
; CHECK-TYPED:   [[memcpy_src:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to i8 addrspace(1)*
; CHECK-TYPED:   [[memcpy_dst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to i8 addrspace(1)*
; CHECK-TYPED:   [[GEP2:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 384
; CHECK-OPAQUE:   [[GEP2:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %q, i32 384
; CHECK-TYPED:   [[GEP3:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 384
; CHECK-OPAQUE:   [[GEP3:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %p, i32 384
; CHECK-TYPED:   [[REM0:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP2]] to <8 x i16> addrspace(1)*
; CHECK-TYPED:   [[REM1:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP3]] to <8 x i16> addrspace(1)*
; CHECK-TYPED:   [[L0:%[_a-zA-Z0-9_]+]] = load <8 x i16>, <8 x i16> addrspace(1)* [[REM0]], align 2
; CHECK-OPAQUE:   [[L0:%[_a-zA-Z0-9_]+]] = load <4 x i32>, ptr addrspace(1) [[GEP2]], align 2
; CHECK-TYPED:   store <8 x i16> [[L0]], <8 x i16> addrspace(1)* [[REM1]], align 2
; CHECK-OPAQUE:   store <4 x i32> [[L0]], ptr addrspace(1) [[GEP3]], align 2
; CHECK-TYPED:   [[GEP4:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 400
; CHECK-OPAQUE:   [[GEP4:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %q, i32 400
; CHECK-TYPED:   [[GEP5:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 400
; CHECK-OPAQUE:   [[GEP5:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %p, i32 400
; CHECK-TYPED:   [[REM2:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP4]] to <4 x i16> addrspace(1)*
; CHECK-TYPED:   [[REM3:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP5]] to <4 x i16> addrspace(1)*
; CHECK-TYPED:   [[L1:%[_a-zA-Z0-9_]+]] = load <4 x i16>, <4 x i16> addrspace(1)* [[REM2]], align 2
; CHECK-OPAQUE:   [[L1:%[_a-zA-Z0-9_]+]] = load <2 x i32>, ptr addrspace(1) [[GEP4]], align 2
; CHECK-TYPED:   store <4 x i16> [[L1]], <4 x i16> addrspace(1)* [[REM3]], align 2
; CHECK-OPAQUE:   store <2 x i32> [[L1]], ptr addrspace(1) [[GEP5]], align 2
; CHECK-TYPED:   [[GEP6:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 408
; CHECK-OPAQUE:   [[GEP6:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %q, i32 408
; CHECK-TYPED:   [[GEP7:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 408
; CHECK-OPAQUE:   [[GEP7:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %p, i32 408
; CHECK-TYPED:   [[REM4:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP6]] to <2 x i16> addrspace(1)*
; CHECK-TYPED:   [[REM5:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP7]] to <2 x i16> addrspace(1)*
; CHECK-TYPED:   [[L2:%[_a-zA-Z0-9_]+]] = load <2 x i16>, <2 x i16> addrspace(1)* [[REM4]], align 2
; CHECK-OPAQUE:   [[L2:%[_a-zA-Z0-9_]+]] = load i32, ptr addrspace(1) [[GEP6]], align 2
; CHECK-TYPED:   store <2 x i16> [[L2]], <2 x i16> addrspace(1)* [[REM5]], align 2
; CHECK-OPAQUE:   store i32 [[L2]], ptr addrspace(1) [[GEP7]], align 2
; CHECK-TYPED:   [[GEP8:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 412
; CHECK-OPAQUE:   [[GEP8:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %q, i32 412
; CHECK-TYPED:   [[GEP9:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 412
; CHECK-OPAQUE:   [[GEP9:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %p, i32 412
; CHECK-TYPED:   [[REM6:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP8]] to i16 addrspace(1)*
; CHECK-TYPED:   [[REM7:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP9]] to i16 addrspace(1)*
; CHECK-TYPED:   [[L3:%[_a-zA-Z0-9_]+]] = load i16, i16 addrspace(1)* [[REM6]], align 2
; CHECK-OPAQUE:   [[L3:%[_a-zA-Z0-9_]+]] = load <2 x i8>, ptr addrspace(1) [[GEP8]], align 2
; CHECK-TYPED:   store i16 [[L3]], i16 addrspace(1)* [[REM7]], align 2
; CHECK-OPAQUE:   store <2 x i8> [[L3]], ptr addrspace(1) [[GEP9]], align 2
; CHECK:   ret void
  %Dst = bitcast %struct.data_t addrspace(1)* %p to i8 addrspace(1)*
  %Src = bitcast %struct.data_t addrspace(1)* %q to i8 addrspace(1)*
  call void @llvm.memcpy.p1i8.p1i8.i32(i8 addrspace(1)* %Dst, i8 addrspace(1)* %Src, i32 414, i32 2, i1 false)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p1i8.p1i8.i32(i8 addrspace(1)* nocapture, i8 addrspace(1)* nocapture, i32, i32, i1) #0

attributes #0 = { nounwind }
