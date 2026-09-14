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

; The fold takes two rounds, and the first one orphans %s1. A dead %s1 left in the function
; would still use %m, so the second round would see %m as shared and stop before folding %t.
; The pass has to retire its own orphans.
;
; CHECK-LABEL: define i32 @orphan_blocks_fold(
; CHECK:         %m = select i1 %E, i32 %x, i32 %z
; CHECK-NEXT:    %s2 = select i1 %A, i32 %m, i32 %w
; CHECK-NEXT:    %s3 = select i1 %D, i32 %s2, i32 %v
; CHECK-NEXT:    ret i32 %s3

define i32 @orphan_blocks_fold(i32 %n, i1 %D, i1 %E, i32 %x, i32 %y, i32 %z, i32 %u, i32 %w, i32 %v) {
  %A  = icmp ult i32 %n, 5
  %C  = icmp ult i32 %n, 10
  %t  = select i1 %C, i32 %x, i32 %y
  %m  = select i1 %E, i32 %t, i32 %z
  %s1 = select i1 %D, i32 %m, i32 %u
  %s2 = select i1 %A, i32 %s1, i32 %w
  %s3 = select i1 %D, i32 %s2, i32 %v
  ret i32 %s3
}
