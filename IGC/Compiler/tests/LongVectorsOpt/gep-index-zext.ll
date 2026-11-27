;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s

; Vector element indices are unsigned and GEP indices are signed, so an index narrower than i32 is zero-extended. As
; an i8, index 199 would address element -57.

; CHECK-LABEL: @extract_i8_index(
; CHECK-NEXT:    [[I:%.*]] = zext i8 %i to i32
; CHECK-NEXT:    [[P:%.*]] = getelementptr <200 x i8>, ptr addrspace(1) %p, i32 0, i32 [[I]]
; CHECK-NEXT:    [[E:%.*]] = load i8, ptr addrspace(1) [[P]], align 1
; CHECK-NEXT:    store i8 [[E]], ptr addrspace(1) %o, align 1
define void @extract_i8_index(ptr addrspace(1) %p, ptr addrspace(1) %o, i8 %i) {
  %v = load <200 x i8>, ptr addrspace(1) %p, align 1
  %e = extractelement <200 x i8> %v, i8 %i
  store i8 %e, ptr addrspace(1) %o, align 1
  ret void
}

; CHECK-LABEL: @insert_i8_index(
; CHECK:         [[I:%.*]] = zext i8 %i to i32
; CHECK-NEXT:    [[P:%.*]] = getelementptr <200 x i8>, ptr addrspace(1) %p, i32 0, i32 [[I]]
; CHECK-NEXT:    store i8 %x, ptr addrspace(1) [[P]], align 1
define void @insert_i8_index(ptr addrspace(1) %p, i8 %i, i8 %x) {
  %v = load <200 x i8>, ptr addrspace(1) %p, align 1
  %n = insertelement <200 x i8> %v, i8 %x, i8 %i
  store <200 x i8> %n, ptr addrspace(1) %p, align 1
  ret void
}

; CHECK-LABEL: @extract_i8_index_private_copy(
; CHECK:         [[C:%.*]] = select i1 {{.*}}, i8 {{.*}}, i8 -57
; CHECK-NEXT:    [[I:%.*]] = zext i8 [[C]] to i32
; CHECK-NEXT:    getelementptr <200 x i8>, ptr {{.*}}, i32 0, i32 [[I]]
define i8 @extract_i8_index_private_copy(<200 x i8> %v, i8 %i) {
  %e = extractelement <200 x i8> %v, i8 %i
  ret i8 %e
}
