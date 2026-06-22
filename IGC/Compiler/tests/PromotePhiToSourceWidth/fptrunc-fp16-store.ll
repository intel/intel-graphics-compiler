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
; PromotePhiToSourceWidth
; ------------------------------------------------
;
define void @fptrunc_store_half(i1 %c, float %a, ptr %p) {
; CHECK-LABEL: @fptrunc_store_half(
; CHECK:       loopexit:
; CHECK-NEXT:    br label %merge
; CHECK:       bypass:
; CHECK-NEXT:    br label %merge
; CHECK:       merge:
; CHECK-NEXT:    [[W:%.*]] = phi float [ %a, %loopexit ], [ 0.000000e+00, %bypass ]
; CHECK-NEXT:    [[FT:%.*]] = fptrunc float [[W]] to half
; CHECK-NEXT:    [[BC:%.*]] = bitcast half [[FT]] to i16
; CHECK-NEXT:    store i16 [[BC]], ptr %p
; CHECK-NEXT:    ret void
entry:
  br i1 %c, label %loopexit, label %bypass

loopexit:                                         ; preds = %entry
  %t = fptrunc float %a to half
  %i = bitcast half %t to i16
  br label %merge

bypass:                                           ; preds = %entry
  br label %merge

merge:                                            ; preds = %bypass, %loopexit
  %phi = phi i16 [ %i, %loopexit ], [ 0, %bypass ]
  store i16 %phi, ptr %p
  ret void
}
