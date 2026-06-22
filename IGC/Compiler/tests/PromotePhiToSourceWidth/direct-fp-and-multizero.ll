;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-promote-phi-to-source-width -S < %s | FileCheck %s
; ------------------------------------------------
; PromotePhiToSourceWidth - direct fp half PHI (no bitcast) and multi-zero edges
; ------------------------------------------------

; Direct half PHI (no bitcast): widen to float, sink fptrunc only.
define void @direct_half(i1 %c, float %a, ptr %p) {
; CHECK-LABEL: @direct_half(
; CHECK:       A:
; CHECK-NEXT:    br label %M
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi float [ %a, %A ], [ 0.000000e+00, %B ]
; CHECK-NEXT:    [[FT:%.*]] = fptrunc float [[W]] to half
; CHECK-NEXT:    store half [[FT]], ptr %p
entry:
  br i1 %c, label %A, label %B
A:
  %t = fptrunc float %a to half
  br label %M
B:
  br label %M
M:
  %phi = phi half [ %t, %A ], [ 0xH0000, %B ]
  store half %phi, ptr %p
  ret void
}

; Three predecessors: one conv edge + two zero edges -> all zero edges widened.
define void @three_way(i32 %s, i32 %a, ptr %p) {
; CHECK-LABEL: @three_way(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ 0, %B ], [ 0, %C ]
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[W]] to i16
; CHECK-NEXT:    store i16 [[T]], ptr %p
entry:
  switch i32 %s, label %A [ i32 0, label %B
                            i32 1, label %C ]
A:
  %t = trunc i32 %a to i16
  br label %M
B:
  br label %M
C:
  br label %M
M:
  %phi = phi i16 [ %t, %A ], [ 0, %B ], [ 0, %C ]
  store i16 %phi, ptr %p
  ret void
}
