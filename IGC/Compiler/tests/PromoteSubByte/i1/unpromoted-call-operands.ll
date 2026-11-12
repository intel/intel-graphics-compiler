;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S < %s | FileCheck %s

; Calls to SPIR-V builtins are not promoted, so they keep returning i1 values.
; When such a value is an operand of a promoted instruction, it has to be
; zero extended to i8 first. The icmp in each function starts the promotion.

declare spir_func i1 @_Z16__spirv_IsFinitef(float)
declare spir_func <4 x i1> @_Z16__spirv_IsFiniteDv4_f(<4 x float>)

; CHECK-LABEL: define spir_kernel void @insertelement_scalar(
; CHECK:         [[CMP:%.*]] = icmp slt i32 %n, 0
; CHECK-NEXT:    [[CMP_I8:%.*]] = zext i1 [[CMP]] to i8
; CHECK-NEXT:    [[CALL:%.*]] = call spir_func i1 @_Z16__spirv_IsFinitef(float %f)
; CHECK-NEXT:    [[V0:%.*]] = insertelement <4 x i8> zeroinitializer, i8 [[CMP_I8]], i32 0
; CHECK-NEXT:    [[CALL_I8:%.*]] = zext i1 [[CALL]] to i8
; CHECK-NEXT:    [[V1:%.*]] = insertelement <4 x i8> [[V0]], i8 [[CALL_I8]], i32 1
; CHECK-NEXT:    store <4 x i8> [[V1]], ptr addrspace(1) %out, align 4
define spir_kernel void @insertelement_scalar(i32 %n, float %f, ptr addrspace(1) %out) {
  %cmp = icmp slt i32 %n, 0
  %call = call spir_func i1 @_Z16__spirv_IsFinitef(float %f)
  %v0 = insertelement <4 x i1> zeroinitializer, i1 %cmp, i32 0
  %v1 = insertelement <4 x i1> %v0, i1 %call, i32 1
  store <4 x i1> %v1, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @insertelement_vector(
; CHECK:         [[CMP:%.*]] = icmp slt i32 %n, 0
; CHECK-NEXT:    [[CMP_I8:%.*]] = zext i1 [[CMP]] to i8
; CHECK-NEXT:    [[CALL:%.*]] = call spir_func <4 x i1> @_Z16__spirv_IsFiniteDv4_f(<4 x float> %f)
; CHECK-NEXT:    [[CALL_I8:%.*]] = zext <4 x i1> [[CALL]] to <4 x i8>
; CHECK-NEXT:    [[V:%.*]] = insertelement <4 x i8> [[CALL_I8]], i8 [[CMP_I8]], i32 1
; CHECK-NEXT:    store <4 x i8> [[V]], ptr addrspace(1) %out, align 4
define spir_kernel void @insertelement_vector(i32 %n, <4 x float> %f, ptr addrspace(1) %out) {
  %cmp = icmp slt i32 %n, 0
  %call = call spir_func <4 x i1> @_Z16__spirv_IsFiniteDv4_f(<4 x float> %f)
  %v = insertelement <4 x i1> %call, i1 %cmp, i32 1
  store <4 x i1> %v, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @shufflevector(
; CHECK:         [[CMP:%.*]] = icmp slt <4 x i32> %n, zeroinitializer
; CHECK-NEXT:    [[CMP_I8:%.*]] = zext <4 x i1> [[CMP]] to <4 x i8>
; CHECK-NEXT:    [[CALL:%.*]] = call spir_func <4 x i1> @_Z16__spirv_IsFiniteDv4_f(<4 x float> %f)
; CHECK-NEXT:    [[CALL_I8:%.*]] = zext <4 x i1> [[CALL]] to <4 x i8>
; CHECK-NEXT:    [[S:%.*]] = shufflevector <4 x i8> [[CMP_I8]], <4 x i8> [[CALL_I8]], <4 x i32> <i32 0, i32 5, i32 2, i32 7>
; CHECK-NEXT:    store <4 x i8> [[S]], ptr addrspace(1) %out, align 4
define spir_kernel void @shufflevector(<4 x i32> %n, <4 x float> %f, ptr addrspace(1) %out) {
  %cmp = icmp slt <4 x i32> %n, zeroinitializer
  %call = call spir_func <4 x i1> @_Z16__spirv_IsFiniteDv4_f(<4 x float> %f)
  %s = shufflevector <4 x i1> %cmp, <4 x i1> %call, <4 x i32> <i32 0, i32 5, i32 2, i32 7>
  store <4 x i1> %s, ptr addrspace(1) %out, align 4
  ret void
}

%struct = type { i1, i32 }

; CHECK-LABEL: define spir_kernel void @insertvalue(
; CHECK:         [[CMP:%.*]] = icmp slt i32 %n, 0
; CHECK-NEXT:    [[CMP_I8:%.*]] = zext i1 [[CMP]] to i8
; CHECK-NEXT:    [[CALL:%.*]] = call spir_func i1 @_Z16__spirv_IsFinitef(float %f)
; CHECK-NEXT:    [[A0:%.*]] = insertvalue %struct poison, i8 [[CMP_I8]], 0
; CHECK-NEXT:    [[CALL_I8:%.*]] = zext i1 [[CALL]] to i8
; CHECK-NEXT:    [[A1:%.*]] = insertvalue %struct [[A0]], i8 [[CALL_I8]], 0
; CHECK-NEXT:    store %struct [[A1]], ptr addrspace(1) %out, align 4
define spir_kernel void @insertvalue(i32 %n, float %f, ptr addrspace(1) %out) {
  %cmp = icmp slt i32 %n, 0
  %call = call spir_func i1 @_Z16__spirv_IsFinitef(float %f)
  %a0 = insertvalue %struct poison, i1 %cmp, 0
  %a1 = insertvalue %struct %a0, i1 %call, 0
  store %struct %a1, ptr addrspace(1) %out, align 4
  ret void
}

; A builtin returning a struct with an i1 keeps the unpromoted struct type. With
; a scalar condition both select operands have to be cast to the promoted struct.

declare spir_func %struct @_Z14__spirv_Structi(i32)

; CHECK-LABEL: define spir_kernel void @select_struct(
; CHECK:         [[CALL:%.*]] = call spir_func %struct.unpromoted @_Z14__spirv_Structi(i32 %n)
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i32 %n, 0
; CHECK-NEXT:    [[CMP_I8:%.*]] = zext i1 [[CMP]] to i8
; CHECK-NEXT:    [[OTHER:%.*]] = insertvalue %struct zeroinitializer, i8 [[CMP_I8]], 0
; CHECK-NEXT:    [[C:%.*]] = trunc i8 %c to i1
; CHECK-NEXT:    [[E0:%.*]] = extractvalue %struct.unpromoted [[CALL]], 0
; CHECK-NEXT:    [[E0_I8:%.*]] = zext i1 [[E0]] to i8
; CHECK-NEXT:    [[A0:%.*]] = insertvalue %struct poison, i8 [[E0_I8]], 0
; CHECK-NEXT:    [[E1:%.*]] = extractvalue %struct.unpromoted [[CALL]], 1
; CHECK-NEXT:    [[A1:%.*]] = insertvalue %struct [[A0]], i32 [[E1]], 1
; CHECK-NEXT:    [[S:%.*]] = select i1 [[C]], %struct [[A1]], %struct [[OTHER]]
; CHECK-NEXT:    store %struct [[S]], ptr addrspace(1) %out, align 4
define spir_kernel void @select_struct(i1 %c, i32 %n, ptr addrspace(1) %out) {
  %call = call spir_func %struct @_Z14__spirv_Structi(i32 %n)
  %cmp = icmp slt i32 %n, 0
  %other = insertvalue %struct { i1 false, i32 0 }, i1 %cmp, 0
  %s = select i1 %c, %struct %call, %struct %other
  store %struct %s, ptr addrspace(1) %out, align 4
  ret void
}
