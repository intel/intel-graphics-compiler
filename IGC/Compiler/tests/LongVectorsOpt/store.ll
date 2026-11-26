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

; Volatile stores are not split. The variable-index insert goes through private memory instead, and the volatile
; store writes the whole vector from there.
; CHECK-LABEL: @volatile_store(
; CHECK-NEXT:    [[V0:%.*]] = alloca <64 x float>, align 256
; CHECK-NEXT:    [[V1:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[V1]], align 4
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[V2]], align 4
; CHECK-NEXT:    [[V3:%.*]] = load <64 x float>, ptr [[V0]], align 256
; CHECK-NEXT:    store volatile <64 x float> [[V3]], ptr addrspace(1) %out, align 4
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

; An insert used by both a store and an extract is split by the store. The extract comes after the store, so it reads
; the element back from the stored memory.
; CHECK-LABEL: @store_and_extract(
; CHECK-NEXT:    [[V0:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %out, i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr addrspace(1) [[V0]], align 4
; CHECK-NEXT:    [[V1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %out, i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr addrspace(1) [[V1]], align 4
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %out, i32 0, i32 0
; CHECK-NEXT:    [[V3:%.*]] = getelementptr <64 x float>, ptr addrspace(1) [[V2]], i32 0, i32 %j
; CHECK-NEXT:    [[V4:%.*]] = load float, ptr addrspace(1) [[V3]], align 4
; CHECK-NEXT:    ret float [[V4]]
define float @store_and_extract(<64 x float> %v, float %a, i32 %i, i32 %j, ptr addrspace(1) %out) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  store <64 x float> %v1, ptr addrspace(1) %out, align 4
  %e = extractelement <64 x float> %v1, i32 %j
  ret float %e
}

; The extract's result is used before the store, so the vector can't be reloaded at the store for it. The chain ends
; there, and the insert goes through private memory instead; the store copies the whole vector out of it.
; CHECK-LABEL: @extract_used_before_store(
; CHECK-NEXT:    [[V0:%.*]] = alloca <64 x float>, align 256
; CHECK-NEXT:    [[V1:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[V1]], align 4
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[V2]], align 4
; CHECK-NEXT:    [[V3:%.*]] = load <64 x float>, ptr [[V0]], align 256
; CHECK-NEXT:    [[V4:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %j
; CHECK-NEXT:    [[V5:%.*]] = load float, ptr [[V4]], align 4
; CHECK-NEXT:    store float [[V5]], ptr addrspace(1) %q, align 4
; CHECK-NEXT:    store <64 x float> [[V3]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
define void @extract_used_before_store(<64 x float> %v, float %a, i32 %i, i32 %j, ptr addrspace(1) %out, ptr addrspace(1) %q) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  %e = extractelement <64 x float> %v1, i32 %j
  store float %e, ptr addrspace(1) %q, align 4
  store <64 x float> %v1, ptr addrspace(1) %out, align 4
  ret void
}

; The same for a user other than an extract.
; CHECK-LABEL: @shuffle_before_store(
; CHECK-NEXT:    [[V0:%.*]] = alloca <64 x float>, align 256
; CHECK-NEXT:    [[V1:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[V1]], align 4
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[V2]], align 4
; CHECK-NEXT:    [[V3:%.*]] = load <64 x float>, ptr [[V0]], align 256
; CHECK-NEXT:    %s = shufflevector <64 x float> [[V3]], <64 x float> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT:    store <64 x float> [[V3]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret <4 x float> %s
define <4 x float> @shuffle_before_store(<64 x float> %v, float %a, i32 %i, ptr addrspace(1) %out) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  %s = shufflevector <64 x float> %v1, <64 x float> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  store <64 x float> %v1, ptr addrspace(1) %out, align 4
  ret <4 x float> %s
}

; A user in a branch the store doesn't dominate ends the chain too.
; CHECK-LABEL: @user_in_other_branch(
; CHECK:       entry:
; CHECK-NEXT:    [[V0:%.*]] = alloca <64 x float>, align 256
; CHECK-NEXT:    [[V1:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[V1]], align 4
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[V2]], align 4
; CHECK-NEXT:    [[V3:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %j
; CHECK-NEXT:    [[V4:%.*]] = load float, ptr [[V3]], align 4
; CHECK-NEXT:    [[V5:%.*]] = load <64 x float>, ptr [[V0]], align 256
; CHECK-NEXT:    br i1 %c, label %st, label %ex
; CHECK:       st:
; CHECK-NEXT:    store <64 x float> [[V5]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret float 0.000000e+00
; CHECK:       ex:
; CHECK-NEXT:    ret float [[V4]]
define float @user_in_other_branch(i1 %c, <64 x float> %v, float %a, i32 %i, i32 %j, ptr addrspace(1) %out) {
entry:
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  br i1 %c, label %st, label %ex
st:
  store <64 x float> %v1, ptr addrspace(1) %out, align 4
  ret float 0.0
ex:
  %e = extractelement <64 x float> %v1, i32 %j
  ret float %e
}

; An insert without users is left alone.
; CHECK-LABEL: @dead_insert(
; CHECK-NEXT:    %v1 = insertelement <64 x float> %v, float %a, i32 %i
; CHECK-NEXT:    ret void
define void @dead_insert(<64 x float> %v, float %a, i32 %i) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  ret void
}
