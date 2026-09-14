;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -dce -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; The assumption is a nested `and` of the implied condition, so proving %mask => %c1 has to
; decompose both levels. The walk also proves two implications from the same root - %mask =>
; %c1 and then %mask => %mask - with the non-implied select %outer in between.
;
; CHECK-LABEL: define float @and_mask_chain(
; CHECK-NEXT:    %c1 = icmp ult i32 %n, 4243200
; CHECK-NEXT:    %c2 = icmp ult i32 %m, 200
; CHECK-NEXT:    %and1 = and i1 %c1, %c2
; CHECK-NEXT:    %c3 = icmp ult i32 %k, 25
; CHECK-NEXT:    %mask = and i1 %c3, %and1
; CHECK-NEXT:    %other = icmp eq i32 %q, 0
; CHECK-NEXT:    %outer = select i1 %other, float %x, float %y
; CHECK-NEXT:    %root = select i1 %mask, float %outer, float 0.000000e+00
; CHECK-NEXT:    ret float %root

define float @and_mask_chain(i32 %n, i32 %m, i32 %k, i32 %q, float %x, float %y, float %z) {
  %c1    = icmp ult i32 %n, 4243200
  %c2    = icmp ult i32 %m, 200
  %and1  = and i1 %c1, %c2
  %c3    = icmp ult i32 %k, 25
  %mask  = and i1 %c3, %and1
  %other = icmp eq i32 %q, 0
  %inner = select i1 %mask, float %x, float 0.000000e+00
  %mid   = select i1 %c1, float %inner, float %z
  %outer = select i1 %other, float %mid, float %y
  %root  = select i1 %mask, float %outer, float 0.000000e+00
  ret float %root
}
