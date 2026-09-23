;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-hoist-fmul-in-loop-pass -S < %s | FileCheck %s
; ------------------------------------------------
; HoistFMulInLoopPass
; ------------------------------------------------
;
; Triage test: %w's cubed source is consumed by both %sum_num (via %num.term, hoistable) and %sum_den (direct fadd, unrelated) -- %w/%q3/%q2/%q/%x must survive untouched and no "hoist" code must appear.

define void @test_shared_weight(ptr %pa, ptr %ps) {
; CHECK-LABEL: @test_shared_weight(
; CHECK:  for.body:
; CHECK:    %x = fmul float %idx.f, {{.*}}
; CHECK:    %one_minus_x = fsub float {{.*}}, %x
; CHECK:    %q = fmul float %one_minus_x, %x
; CHECK:    %q2 = fmul float %q, %q
; CHECK:    %q3 = fmul float %q2, %q
; CHECK:    %w = fmul float %q3, {{.*}}
; CHECK:    %sample = load float, ptr %sample.ptr, align 4
; CHECK:    %num.term = fmul float %sample, %w
; CHECK:    %sum_num.next = fadd float %num.term, %sum_num
; CHECK:    %sum_den.next = fadd float %w, %sum_den
; CHECK-NOT: hoist
;
entry:
  br label %for.body

for.body:
  %idx = phi i32 [ 0, %entry ], [ %idx.next, %for.body ]
  %sum_num = phi float [ 0.000000e+00, %entry ], [ %sum_num.next, %for.body ]
  %sum_den = phi float [ 0.000000e+00, %entry ], [ %sum_den.next, %for.body ]

  %idx.f = sitofp i32 %idx to float
  %x = fmul float %idx.f, 0x3FC99999A0000000
  %one_minus_x = fsub float 1.000000e+00, %x
  %q = fmul float %one_minus_x, %x
  %q2 = fmul float %q, %q
  %q3 = fmul float %q2, %q
  %w = fmul float %q3, 0x4047555320000000

  %sample.ptr = getelementptr float, ptr %ps, i32 %idx
  %sample = load float, ptr %sample.ptr, align 4

  %num.term = fmul float %sample, %w
  %sum_num.next = fadd float %num.term, %sum_num
  %sum_den.next = fadd float %w, %sum_den

  %idx.next = add i32 %idx, 1
  %cond = icmp eq i32 %idx.next, 5
  br i1 %cond, label %for.end, label %for.body

for.end:
  %result = fdiv float %sum_num.next, %sum_den.next
  store float %result, ptr %pa, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
