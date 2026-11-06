;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s

; Long-vector PHIs are moved to private memory: each incoming value is stored at the end of its predecessor and the
; PHI is replaced with a load, whose extracts are split into element loads.

; CHECK-LABEL: @phi(
; CHECK:       entry:
; CHECK-NEXT:    [[SLOT:%.*]] = alloca <64 x float>
; CHECK:       a:
; CHECK-NEXT:    store <64 x float> %x, ptr [[SLOT]]
; CHECK-NEXT:    br label %m
; CHECK:       b:
; CHECK-NEXT:    store <64 x float> %y, ptr [[SLOT]]
; CHECK-NEXT:    br label %m
; CHECK:       m:
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr [[P]], align 4
; CHECK-NEXT:    ret float [[E]]
define float @phi(i1 %c, <64 x float> %x, <64 x float> %y, i32 %j) {
entry:
  br i1 %c, label %a, label %b
a:
  br label %m
b:
  br label %m
m:
  %v = phi <64 x float> [ %x, %a ], [ %y, %b ]
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; An insert chain coming into the PHI is split into element stores to the PHI slot.
; CHECK-LABEL: @phi_insert(
; CHECK:       entry:
; CHECK-NEXT:    [[SLOT:%.*]] = alloca <64 x float>
; CHECK-NEXT:    [[P0:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %x, ptr [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[P1]], align 4
; CHECK-NEXT:    br i1 %c, label %t, label %m
; CHECK:       t:
; CHECK-NEXT:    store <64 x float> %x, ptr [[SLOT]]
; CHECK-NEXT:    br label %m
; CHECK:       m:
; CHECK-NEXT:    [[P2:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr [[P2]], align 4
; CHECK-NEXT:    ret float [[E]]
define float @phi_insert(i1 %c, <64 x float> %x, float %a, i32 %i, i32 %j) {
entry:
  %x1 = insertelement <64 x float> %x, float %a, i32 %i
  br i1 %c, label %t, label %m
t:
  br label %m
m:
  %v = phi <64 x float> [ %x1, %entry ], [ %x, %t ]
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}
