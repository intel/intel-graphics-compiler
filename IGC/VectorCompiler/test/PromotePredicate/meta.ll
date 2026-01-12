;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPromotePredicate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Xe2 -logical-ops-threshold=2 -S < %s | FileCheck %s

define dllexport spir_kernel void @foo(<8 x i32> %A) {
  %cmp.0 = icmp ult <8 x i32> %A, <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7>
  %cmp.1 = icmp uge <8 x i32> %A, <i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4>
  ; CHECK: %and = and <8 x i1> %cmp.0, %cmp.1, !pred.index !0
  %and = and <8 x i1> %cmp.0, %cmp.1
  ret void
}

; CHECK: !0 = !{i64 1}
