;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s

; Extracts from long stateless loads become element loads right before their first use if nothing can write the
; memory before it, otherwise at the position of the original load. The long load is removed once it has no other
; users.

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

; Extracts placed right after the loads are loaded where the elements are used.
; CHECK-LABEL: @elementwise(
; CHECK-NEXT:    [[PA0:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %a, i32 0, i32 0
; CHECK-NEXT:    [[A0:%.*]] = load float, ptr addrspace(1) [[PA0]], align 4
; CHECK-NEXT:    [[PB0:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %b, i32 0, i32 0
; CHECK-NEXT:    [[B0:%.*]] = load float, ptr addrspace(1) [[PB0]], align 4
; CHECK-NEXT:    %s0 = fadd float [[A0]], [[B0]]
; CHECK-NEXT:    store float %s0, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    [[PA1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %a, i32 0, i32 1
; CHECK-NEXT:    [[A1:%.*]] = load float, ptr addrspace(1) [[PA1]], align 4
; CHECK-NEXT:    [[PB1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %b, i32 0, i32 1
; CHECK-NEXT:    [[B1:%.*]] = load float, ptr addrspace(1) [[PB1]], align 4
; CHECK-NEXT:    %s1 = fadd float [[A1]], [[B1]]
define void @elementwise(ptr addrspace(1) %a, ptr addrspace(1) %b, ptr addrspace(1) noalias %out) {
  %va = load <64 x float>, ptr addrspace(1) %a, align 4
  %vb = load <64 x float>, ptr addrspace(1) %b, align 4
  %a0 = extractelement <64 x float> %va, i32 0
  %a1 = extractelement <64 x float> %va, i32 1
  %b0 = extractelement <64 x float> %vb, i32 0
  %b1 = extractelement <64 x float> %vb, i32 1
  %s0 = fadd float %a0, %b0
  store float %s0, ptr addrspace(1) %out, align 4
  %s1 = fadd float %a1, %b1
  %out1 = getelementptr float, ptr addrspace(1) %out, i32 1
  store float %s1, ptr addrspace(1) %out1, align 4
  ret void
}

; An extract with several users is loaded right before the first of them.
; CHECK-LABEL: @shared_extract(
; CHECK-NEXT:    [[PA0:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %a, i32 0, i32 0
; CHECK-NEXT:    [[A0:%.*]] = load float, ptr addrspace(1) [[PA0]], align 4
; CHECK-NEXT:    [[PB0:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %b, i32 0, i32 0
; CHECK-NEXT:    [[B0:%.*]] = load float, ptr addrspace(1) [[PB0]], align 4
; CHECK-NEXT:    %c0 = fcmp ogt float [[A0]], [[B0]]
; CHECK-NEXT:    %m0 = select i1 %c0, float [[A0]], float [[B0]]
; CHECK-NEXT:    store float %m0, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    [[PA1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %a, i32 0, i32 1
; CHECK-NEXT:    [[A1:%.*]] = load float, ptr addrspace(1) [[PA1]], align 4
; CHECK-NEXT:    [[PB1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %b, i32 0, i32 1
; CHECK-NEXT:    [[B1:%.*]] = load float, ptr addrspace(1) [[PB1]], align 4
; CHECK-NEXT:    %c1 = fcmp ogt float [[A1]], [[B1]]
; CHECK-NEXT:    %m1 = select i1 %c1, float [[A1]], float [[B1]]
define void @shared_extract(ptr addrspace(1) %a, ptr addrspace(1) %b, ptr addrspace(1) noalias %out) {
  %va = load <64 x float>, ptr addrspace(1) %a, align 4
  %vb = load <64 x float>, ptr addrspace(1) %b, align 4
  %a0 = extractelement <64 x float> %va, i32 0
  %a1 = extractelement <64 x float> %va, i32 1
  %b0 = extractelement <64 x float> %vb, i32 0
  %b1 = extractelement <64 x float> %vb, i32 1
  %c0 = fcmp ogt float %a0, %b0
  %m0 = select i1 %c0, float %a0, float %b0
  store float %m0, ptr addrspace(1) %out, align 4
  %c1 = fcmp ogt float %a1, %b1
  %m1 = select i1 %c1, float %a1, float %b1
  %out1 = getelementptr float, ptr addrspace(1) %out, i32 1
  store float %m1, ptr addrspace(1) %out1, align 4
  ret void
}

; A use in another block doesn't keep the element load at the extract: it is created before the first use in the
; block of the extract, which dominates the other uses.
; CHECK-LABEL: @user_in_other_block(
; CHECK:       entry:
; CHECK-NEXT:    %x = fadd float %b, %b
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %a, i32 0, i32 0
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
; CHECK-NEXT:    %s = fmul float [[E]], %x
; CHECK:       then:
; CHECK-NEXT:    %t = fadd float [[E]], %b
define float @user_in_other_block(ptr addrspace(1) %a, float %b, i1 %c) {
entry:
  %va = load <64 x float>, ptr addrspace(1) %a, align 4
  %a0 = extractelement <64 x float> %va, i32 0
  %x = fadd float %b, %b
  %s = fmul float %a0, %x
  br i1 %c, label %then, label %exit

then:
  %t = fadd float %a0, %b
  br label %exit

exit:
  %r = phi float [ %s, %entry ], [ %t, %then ]
  ret float %r
}

; A store that may write the loaded memory before the use keeps the element loads at the original load.
; CHECK-LABEL: @write_before_use(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %a, i32 0, i32 0
; CHECK-NEXT:    [[E0:%.*]] = load float, ptr addrspace(1) [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %a, i32 0, i32 1
; CHECK-NEXT:    [[E1:%.*]] = load float, ptr addrspace(1) [[P1]], align 4
; CHECK-NEXT:    store float 0.000000e+00, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    %s = fadd float [[E0]], [[E1]]
define void @write_before_use(ptr addrspace(1) %a, ptr addrspace(1) %out) {
  %va = load <64 x float>, ptr addrspace(1) %a, align 4
  %a0 = extractelement <64 x float> %va, i32 0
  %a1 = extractelement <64 x float> %va, i32 1
  store float 0.0, ptr addrspace(1) %out, align 4
  %s = fadd float %a0, %a1
  store float %s, ptr addrspace(1) %out, align 4
  ret void
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

; Volatile loads are not split. The variable-index extract reads from a private-memory copy of the loaded vector.
; CHECK-LABEL: @volatile_load(
; CHECK-NEXT:    [[V0:%.*]] = alloca <64 x float>, align 256
; CHECK-NEXT:    %v = load volatile <64 x float>, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    store <64 x float> %v, ptr [[V0]], align 256
; CHECK-NEXT:    [[JJ:%.*]] = freeze i32 %j
; CHECK-NEXT:    [[V1:%.*]] = and i32 [[JJ]], 63
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 [[V1]]
; CHECK-NEXT:    [[V3:%.*]] = load float, ptr [[V2]], align 4
; CHECK-NEXT:    ret float [[V3]]
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

; The index is computed after the load. It is cheap and safe to compute at the load, so it is computed there again
; and the load is split as usual.
; CHECK-LABEL: @index_after_load(
; CHECK-NEXT:    [[V0:%.*]] = add i32 %i, 1
; CHECK-NEXT:    [[V1:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i32 [[V0]]
; CHECK-NEXT:    [[V2:%.*]] = load float, ptr addrspace(1) [[V1]], align 4
; CHECK-NEXT:    ret float [[V2]]
define float @index_after_load(ptr addrspace(1) %p, i32 %i) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  %j = add i32 %i, 1
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}
