;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s

; Long ldrawvector/storerawvector accesses are split into element ldraw/storeraw accesses at
; offset + index * element size.

; CHECK-LABEL: @ldraw_extract(
; CHECK-NEXT:    [[O:%.*]] = mul i32 %j, 4
; CHECK-NEXT:    [[OFF:%.*]] = add i32 %off, [[O]]
; CHECK-NEXT:    [[E:%.*]] = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2490368(ptr addrspace(2490368) %buf, i32 [[OFF]], i32 4, i1 false)
; CHECK-NEXT:    ret float [[E]]
define float @ldraw_extract(ptr addrspace(2490368) %buf, i32 %off, i32 %j) {
  %v = call <64 x float> @llvm.genx.GenISA.ldrawvector.indexed.v64f32.p2490368(ptr addrspace(2490368) %buf, i32 %off, i32 16, i1 false)
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; A 64-bit index is truncated to the offset type.
; CHECK-LABEL: @ldraw_extract_i64(
; CHECK-NEXT:    [[J:%.*]] = trunc i64 %j to i32
; CHECK-NEXT:    [[O:%.*]] = mul i32 [[J]], 4
; CHECK-NEXT:    [[OFF:%.*]] = add i32 %off, [[O]]
; CHECK-NEXT:    [[E:%.*]] = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2490368(ptr addrspace(2490368) %buf, i32 [[OFF]], i32 4, i1 false)
; CHECK-NEXT:    ret float [[E]]
define float @ldraw_extract_i64(ptr addrspace(2490368) %buf, i32 %off, i64 %j) {
  %v = call <64 x float> @llvm.genx.GenISA.ldrawvector.indexed.v64f32.p2490368(ptr addrspace(2490368) %buf, i32 %off, i32 16, i1 false)
  %e = extractelement <64 x float> %v, i64 %j
  ret float %e
}

; Volatile ldraws are not split. The variable-index extract reads from a private-memory copy of the loaded vector.
; CHECK-LABEL: @ldraw_volatile(
; CHECK-NEXT:    [[V0:%.*]] = alloca <64 x float>, align 256
; CHECK-NEXT:    %v = call <64 x float> @llvm.genx.GenISA.ldrawvector.indexed.v64f32.p2490368(ptr addrspace(2490368) %buf, i32 %off, i32 16, i1 true)
; CHECK-NEXT:    store <64 x float> %v, ptr [[V0]], align 256
; CHECK-NEXT:    [[J:%.*]] = freeze i32 %j
; CHECK-NEXT:    [[V1:%.*]] = and i32 [[J]], 63
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 [[V1]]
; CHECK-NEXT:    [[V3:%.*]] = load float, ptr [[V2]], align 4
; CHECK-NEXT:    ret float [[V3]]
define float @ldraw_volatile(ptr addrspace(2490368) %buf, i32 %off, i32 %j) {
  %v = call <64 x float> @llvm.genx.GenISA.ldrawvector.indexed.v64f32.p2490368(ptr addrspace(2490368) %buf, i32 %off, i32 16, i1 true)
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; The base vector is stored first, then the inserted element. The store alignment (2) is smaller than the element
; size, so it is kept.
; CHECK-LABEL: @storeraw_insert(
; CHECK-NEXT:    [[OFF0:%.*]] = add i32 %off, 0
; CHECK-NEXT:    call void @llvm.genx.GenISA.storerawvector.indexed.p2490368.v64f32(ptr addrspace(2490368) %buf, i32 [[OFF0]], <64 x float> %v, i32 2, i1 false)
; CHECK-NEXT:    [[O:%.*]] = mul i32 %i, 4
; CHECK-NEXT:    [[OFF1:%.*]] = add i32 %off, [[O]]
; CHECK-NEXT:    call void @llvm.genx.GenISA.storeraw.indexed.p2490368.f32(ptr addrspace(2490368) %buf, i32 [[OFF1]], float %a, i32 2, i1 false)
; CHECK-NEXT:    ret void
define void @storeraw_insert(ptr addrspace(2490368) %buf, i32 %off, <64 x float> %v, float %a, i32 %i) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490368.v64f32(ptr addrspace(2490368) %buf, i32 %off, <64 x float> %v1, i32 2, i1 false)
  ret void
}

declare <64 x float> @llvm.genx.GenISA.ldrawvector.indexed.v64f32.p2490368(ptr addrspace(2490368), i32, i32, i1)
declare void @llvm.genx.GenISA.storerawvector.indexed.p2490368.v64f32(ptr addrspace(2490368), i32, <64 x float>, i32, i1)
