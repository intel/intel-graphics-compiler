;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; Regression test: when ExpectedBranchBB is null (DstTrue/DstFalse each end with
; a conditional branch so getUncondSucc returns null), the adjacency guard was
; skipped and any later branch on the same condition was accepted as a source
; triple.  This deleted intermediate escape blocks and corrupted PHIs.
;
; Minimal CFG that triggers the old bug:
;
;   entry: br i1 %cond -> dst_true, dst_false
;   dst_true:  conditional branch  <- getUncondSucc = null -> ExpectedBranchBB = null
;   dst_false: conditional branch
;   ...intermediate blocks...
;   inner: br i1 %cond -> src_true, src_false  <- second use, was accepted without adjacency check
;   escape: br label %merge                    <- was deleted by old code
;
; After the fix (break when ExpectedBranchBB == null) the pass rejects the
; candidate and the IR must be unchanged: escape block and its PHI entry survive.
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s

target datalayout = "e-p:64:64-i64:64"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: @test
; The escape block must survive.
; CHECK: escape:
; CHECK: br label %merge
define double @test(i1 %cond, i1 %cond2, double %a, double %b) {
entry:
  br i1 %cond, label %dst_true, label %dst_false

dst_true:                                         ; preds = %entry
  br i1 %cond2, label %dt_a, label %dt_b

dst_false:                                        ; preds = %entry
  br i1 %cond2, label %df_a, label %df_b

dt_a:                                             ; preds = %dst_true
  br label %inner

dt_b:                                             ; preds = %dst_true
  br label %inner

df_a:                                             ; preds = %dst_false
  br label %inner

df_b:                                             ; preds = %dst_false
  br label %escape

; Second use of %cond. Old code: accepted without adjacency check (bug).
; New code: rejected because ExpectedBranchBB == null.
inner:                                            ; preds = %dt_a, %dt_b, %df_a
  br i1 %cond, label %src_true, label %src_false

src_true:                                         ; preds = %inner
  br label %merge

src_false:                                        ; preds = %inner
  br label %merge

escape:                                           ; preds = %df_b
  br label %merge

merge:                                            ; preds = %src_true, %src_false, %escape
  %r = phi double [ %a, %src_true ], [ %b, %src_false ], [ 0.0, %escape ]
  ret double %r
}
