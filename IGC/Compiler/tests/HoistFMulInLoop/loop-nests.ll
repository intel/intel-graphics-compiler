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
; HoistFMulInLoopPass: which loops are looked at
; ------------------------------------------------
;
; Loops handler: every loop in a nest is looked at, innermost first, at any depth. What an
; inner loop's sum turns into after its factors are taken out is a multiply in the loop
; around it, which that loop's own accumulator can take the factors out of in turn.

; An accumulator in an inner loop is hoisted too. The compensation lands after the
; inner loop, which is still inside the outer one: the outer loop has nothing to
; accumulate here, so the factor is applied once per outer iteration.

define void @test_nested_inner_loop(ptr %pa, float %k) {
; CHECK-LABEL: @test_nested_inner_loop(
; CHECK:  inner:
; CHECK:    %isum.next = fadd float %v, %isum
; CHECK:  outer.latch:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %isum.next, %k
; CHECK-NEXT: store float [[H]], ptr %pa, align 4
;
entry:
  br label %outer

outer:
  %j = phi i32 [ 0, %entry ], [ %j.next, %outer.latch ]
  br label %inner

inner:
  %i = phi i32 [ 0, %outer ], [ %i.next, %inner ]
  %isum = phi float [ 0.000000e+00, %outer ], [ %isum.next, %inner ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %isum.next = fadd float %m, %isum

  %i.next = add i32 %i, 1
  %ic = icmp eq i32 %i.next, 4
  br i1 %ic, label %outer.latch, label %inner

outer.latch:
  store float %isum.next, ptr %pa, align 4
  %j.next = add i32 %j, 1
  %oc = icmp eq i32 %j.next, 4
  br i1 %oc, label %exit, label %outer

exit:
  ret void
}

; The inner loop's factor %k and the outer loop's factor %w both end up after the outer
; loop. The inner loop is done first: %k comes out of it as a multiply of the inner sum,
; in the outer loop. That is one more product for the outer accumulator, whose factors
; %k and %w both leave. Done outer first, %k would be left behind in the outer loop.

define void @test_nested_both(ptr %pa, float %k, float %w) {
; CHECK-LABEL: @test_nested_both(
; CHECK:  inner:
; CHECK:    %isum.next = fadd float %v, %isum
; CHECK:  outer.latch:
; CHECK:    %osum.next = fadd float %isum.next, %osum
; CHECK:  exit:
; CHECK:    [[P:%[A-Za-z0-9_.]+]] = fmul float %k, %w
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %osum.next, [[P]]
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %outer

outer:
  %j = phi i32 [ 0, %entry ], [ %j.next, %outer.latch ]
  %osum = phi float [ 0.000000e+00, %entry ], [ %osum.next, %outer.latch ]
  br label %inner

inner:
  %i = phi i32 [ 0, %outer ], [ %i.next, %inner ]
  %isum = phi float [ 0.000000e+00, %outer ], [ %isum.next, %inner ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %isum.next = fadd float %m, %isum

  %i.next = add i32 %i, 1
  %ic = icmp eq i32 %i.next, 4
  br i1 %ic, label %outer.latch, label %inner

outer.latch:
  %t = fmul float %isum.next, %w
  %osum.next = fadd float %t, %osum
  %j.next = add i32 %j, 1
  %oc = icmp eq i32 %j.next, 4
  br i1 %oc, label %exit, label %outer

exit:
  store float %osum.next, ptr %pa, align 4
  ret void
}

; Three loops deep, each level accumulating the one below scaled by its own invariant.
; Every loop in the nest is looked at, innermost first, so the factors of all three
; levels pass through to the end of the outermost loop and each accumulator is a plain
; sum.

define void @test_triple_nested(ptr %pa, float %k, float %w1, float %w2) {
; CHECK-LABEL: @test_triple_nested(
; CHECK:  inner:
; CHECK:    %isum.next = fadd float %v, %isum
; CHECK:  middle.latch:
; CHECK:    %msum.next = fadd float %isum.next, %msum
; CHECK:  outer.latch:
; CHECK:    %osum.next = fadd float %msum.next, %osum
; CHECK:  exit:
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %osum.next, {{.*}}
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %outer

outer:
  %j = phi i32 [ 0, %entry ], [ %j.next, %outer.latch ]
  %osum = phi float [ 0.000000e+00, %entry ], [ %osum.next, %outer.latch ]
  br label %middle

middle:
  %l = phi i32 [ 0, %outer ], [ %l.next, %middle.latch ]
  %msum = phi float [ 0.000000e+00, %outer ], [ %msum.next, %middle.latch ]
  br label %inner

inner:
  %i = phi i32 [ 0, %middle ], [ %i.next, %inner ]
  %isum = phi float [ 0.000000e+00, %middle ], [ %isum.next, %inner ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %isum.next = fadd float %m, %isum

  %i.next = add i32 %i, 1
  %ic = icmp eq i32 %i.next, 4
  br i1 %ic, label %middle.latch, label %inner

middle.latch:
  %t1 = fmul float %isum.next, %w1
  %msum.next = fadd float %t1, %msum
  %l.next = add i32 %l, 1
  %mc = icmp eq i32 %l.next, 4
  br i1 %mc, label %outer.latch, label %middle

outer.latch:
  %t2 = fmul float %msum.next, %w2
  %osum.next = fadd float %t2, %osum
  %j.next = add i32 %j, 1
  %oc = icmp eq i32 %j.next, 4
  br i1 %oc, label %exit, label %outer

exit:
  store float %osum.next, ptr %pa, align 4
  ret void
}

; A non-zero seed is invariant to the inner loop: it is computed before the loop starts, in
; the enclosing loop, and does not change while the inner loop runs. The inner accumulator
; starts at 0.0 and the seed is added back, once per run of the inner loop, after it.

define void @test_nested_nonzero_seed(ptr %pa, ptr %ps, float %k) {
; CHECK-LABEL: @test_nested_nonzero_seed(
; CHECK:  inner:
; CHECK:    %isum = phi float [ 0.000000e+00, %outer ], [ %isum.next, %inner ]
; CHECK:    %isum.next = fadd float %v, %isum
; CHECK:  outer.latch:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %isum.next, %k
; CHECK-NEXT: [[S:%[A-Za-z0-9_.]+]] = fadd float [[H]], %seed
; CHECK-NEXT: store float [[S]], ptr %pa, align 4
;
entry:
  br label %outer

outer:
  %j = phi i32 [ 0, %entry ], [ %j.next, %outer.latch ]
  %seed = load float, ptr %ps, align 4
  br label %inner

inner:
  %i = phi i32 [ 0, %outer ], [ %i.next, %inner ]
  %isum = phi float [ %seed, %outer ], [ %isum.next, %inner ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %isum.next = fadd float %m, %isum

  %i.next = add i32 %i, 1
  %ic = icmp eq i32 %i.next, 4
  br i1 %ic, label %outer.latch, label %inner

outer.latch:
  store float %isum.next, ptr %pa, align 4
  %j.next = add i32 %j, 1
  %oc = icmp eq i32 %j.next, 4
  br i1 %oc, label %exit, label %outer

exit:
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
