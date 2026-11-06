;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s

; Insert chains feeding a long stateless store are split into element stores.

; Volatile stores are not split.
; CHECK-LABEL: @volatile_store(
; CHECK-NEXT:    %v1 = insertelement <64 x float> %v, float %a, i32 %i
; CHECK-NEXT:    store volatile <64 x float> %v1, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
define void @volatile_store(<64 x float> %v, float %a, i32 %i, ptr addrspace(1) %out) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  store volatile <64 x float> %v1, ptr addrspace(1) %out, align 4
  ret void
}

; Later inserts to the same index overwrite earlier ones, so the element stores keep the insert order.
; CHECK-LABEL: @same_index(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %out, i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr addrspace(1) [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %out, i32 0, i32 %i
; CHECK-NEXT:    store float %b, ptr addrspace(1) [[P1]], align 4
; CHECK-NEXT:    ret void
define void @same_index(float %a, float %b, i32 %i, ptr addrspace(1) %out) {
  %v1 = insertelement <64 x float> undef, float %a, i32 %i
  %v2 = insertelement <64 x float> %v1, float %b, i32 %i
  store <64 x float> %v2, ptr addrspace(1) %out, align 4
  ret void
}

; A constant base vector is stored as a whole before the elements.
; CHECK-LABEL: @constant_base(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %out, i32 0, i32 0
; CHECK-NEXT:    store <64 x float> zeroinitializer, ptr addrspace(1) [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %out, i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr addrspace(1) [[P1]], align 4
; CHECK-NEXT:    ret void
define void @constant_base(float %a, i32 %i, ptr addrspace(1) %out) {
  %v1 = insertelement <64 x float> zeroinitializer, float %a, i32 %i
  store <64 x float> %v1, ptr addrspace(1) %out, align 16
  ret void
}

; An insert used by both a store and an extract goes through private memory, and the store copies the whole vector
; out of it.
; CHECK-LABEL: @store_and_extract(
; CHECK-NEXT:    [[SLOT:%.*]] = alloca <64 x float>
; CHECK-NEXT:    [[P0:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[P1]], align 4
; CHECK-NEXT:    [[P2:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr [[P2]], align 4
; CHECK-NEXT:    [[VEC:%.*]] = load <64 x float>, ptr [[SLOT]]
; CHECK-NEXT:    store <64 x float> [[VEC]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret float [[E]]
define float @store_and_extract(<64 x float> %v, float %a, i32 %i, i32 %j, ptr addrspace(1) %out) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  store <64 x float> %v1, ptr addrspace(1) %out, align 4
  %e = extractelement <64 x float> %v1, i32 %j
  ret float %e
}
