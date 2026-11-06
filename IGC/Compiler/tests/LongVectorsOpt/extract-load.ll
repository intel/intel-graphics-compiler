;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s

; Extracts from long stateless loads become element loads at the position of the original load. The long load is
; removed once it has no other users.

; The element alignment is the smaller of the load alignment and the element size.
; CHECK-LABEL: @extract_load(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i32 %j
; CHECK-NEXT:    [[E0:%.*]] = load float, ptr addrspace(1) [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i32 5
; CHECK-NEXT:    [[E1:%.*]] = load float, ptr addrspace(1) [[P1]], align 4
; CHECK-NEXT:    %s = fadd float [[E0]], [[E1]]
; CHECK-NEXT:    ret float %s
define float @extract_load(ptr addrspace(1) %p, i32 %j) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 16
  %e0 = extractelement <64 x float> %v, i32 %j
  %e1 = extractelement <64 x float> %v, i32 5
  %s = fadd float %e0, %e1
  ret float %s
}

; CHECK-LABEL: @extract_load_i64(
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i64 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
; CHECK-NEXT:    ret float [[E]]
define float @extract_load_i64(ptr addrspace(1) %p, i64 %j) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  %e = extractelement <64 x float> %v, i64 %j
  ret float %e
}

; CHECK-LABEL: @i16_elements(
; CHECK-NEXT:    [[P:%.*]] = getelementptr <128 x i16>, ptr addrspace(1) %p, i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load i16, ptr addrspace(1) [[P]], align 2
; CHECK-NEXT:    ret i16 [[E]]
define i16 @i16_elements(ptr addrspace(1) %p, i32 %j) {
  %v = load <128 x i16>, ptr addrspace(1) %p, align 2
  %e = extractelement <128 x i16> %v, i32 %j
  ret i16 %e
}

; The long load stays when it has users other than extracts.
; CHECK-LABEL: @load_other_use(
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
; CHECK-NEXT:    %v = load <64 x float>, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    store <64 x float> %v, ptr addrspace(1) %q, align 4
; CHECK-NEXT:    ret float [[E]]
define float @load_other_use(ptr addrspace(1) %p, ptr addrspace(1) %q, i32 %j) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  store <64 x float> %v, ptr addrspace(1) %q, align 4
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; Volatile loads are not split.
; CHECK-LABEL: @volatile_load(
; CHECK-NEXT:    %v = load volatile <64 x float>, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    %e = extractelement <64 x float> %v, i32 %j
define float @volatile_load(ptr addrspace(1) %p, i32 %j) {
  %v = load volatile <64 x float>, ptr addrspace(1) %p, align 4
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; An out-of-range constant index yields poison and must not become an out-of-bounds load.
; CHECK-LABEL: @out_of_range_index(
; CHECK-NEXT:    %v = load <64 x float>, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    %e = extractelement <64 x float> %v, i32 64
define float @out_of_range_index(ptr addrspace(1) %p) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  %e = extractelement <64 x float> %v, i32 64
  ret float %e
}

; The index is not available at the load, so the extract is left alone.
; CHECK-LABEL: @index_after_load(
; CHECK-NEXT:    %v = load <64 x float>, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    %j = add i32 %i, 1
; CHECK-NEXT:    %e = extractelement <64 x float> %v, i32 %j
define float @index_after_load(ptr addrspace(1) %p, i32 %i) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  %j = add i32 %i, 1
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}
