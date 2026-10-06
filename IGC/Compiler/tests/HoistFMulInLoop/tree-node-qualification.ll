;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: split-file %s %t
; RUN: igc_opt --opaque-pointers -igc-hoist-fmul-in-loop-pass -S < %t/relaxed.ll | FileCheck %s
; RUN: igc_opt --opaque-pointers -igc-hoist-fmul-in-loop-pass -S < %t/per-instruction.ll \
; RUN:   | FileCheck %s --check-prefix=FLAGS
; ------------------------------------------------
; HoistFMulInLoopPass: what is in the tree (step 2)
; ------------------------------------------------
;
; Step 2: under a product, a multiply in the loop with relaxed math is looked through; a
; constant or a value defined outside the loop is an invariant factor; anything else is
; opaque. The functions after the first marker run with module-level relaxed math on; the
; ones after the second are judged by the flags on their own instructions.

;--- relaxed.ll
; A constant is an invariant like any other: the sum is scaled by 2.0 after the loop.

define void @test_constant_factor(ptr %pa) {
; CHECK-LABEL: @test_constant_factor(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, 2.000000e+00
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, 2.000000e+00
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; A multiply computed before the loop is one invariant value, not a tree to look through:
; the sum is scaled by %kk as a whole, and %k1 and %k2 are not multiplied in again.

define void @test_outside_fmul_is_one_factor(ptr %pa, float %k1, float %k2) {
; CHECK-LABEL: @test_outside_fmul_is_one_factor(
; CHECK:  entry:
; CHECK:    %kk = fmul float %k1, %k2
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %kk
; CHECK-NEXT: store float [[H]], ptr %pa, align 4
;
entry:
  %kk = fmul float %k1, %k2
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %kk
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; A value computed inside the loop is not invariant here, even when everything it is made
; of is: moving it out is loop-invariant code motion's job. It is an opaque operand, so
; there is nothing to take out of the product and the loop is left alone.

define void @test_inloop_expression_is_opaque(ptr %pa, float %a, float %b) {
; CHECK-LABEL: @test_inloop_expression_is_opaque(
; CHECK:  for.body:
; CHECK:    %kk = fadd float %a, %b
; CHECK:    %m = fmul float %v, %kk
; CHECK:    %sum.next = fadd float %m, %sum
; CHECK:  for.end:
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %kk = fadd float %a, %b
  %m = fmul float %v, %kk
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; An operand computed in the loop that is not a multiply -- here a sum -- is opaque: the
; tree does not look into it, and the invariant beside it is taken out all the same.

define void @test_opaque_operand(ptr %pa, float %k) {
; CHECK-LABEL: @test_opaque_operand(
; CHECK:  for.body:
; CHECK:    %t = fadd float %v, 1.000000e+00
; CHECK:    %sum.next = fadd float %t, %sum
; CHECK:  for.end:
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %t = fadd float %v, 1.000000e+00
  %m = fmul float %t, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}

;--- per-instruction.ll
; Positive control for test_nonrelaxed_interior: module-level FastRelaxedMath is
; off, but every instruction carries `fast`, so the whole chain qualifies and the
; invariant %k is hoisted out of the interior multiply %b. Shows that qualification
; is judged per instruction rather than switched off wholesale.

define void @test_per_instruction_flags(ptr %pa, ptr %py, float %k) {
; FLAGS-LABEL: @test_per_instruction_flags(
; FLAGS:  for.body:
; FLAGS:    %m = fmul fast float %v, %y
; FLAGS:    %sum.next = fadd fast float %m, %sum
; FLAGS:  for.end:
; FLAGS:    [[H:%[A-Za-z0-9_.]+]] = fmul {{.*}}float %sum.next, %k
; FLAGS:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %yp = getelementptr float, ptr %py, i32 %i
  %y = load float, ptr %yp, align 4

  %b = fmul fast float %v, %k
  %m = fmul fast float %b, %y
  %sum.next = fadd fast float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Relaxed math is judged per instruction here (module-level FastRelaxedMath is off).
; The accumulator and the root product are `fast`, but the interior multiply %b is
; not, so it may not be reassociated: the tree must stop at %b and treat it as an
; opaque value, which leaves nothing invariant to hoist. Companion of
; test_per_instruction_flags, which differs only in %b carrying `fast`.

define void @test_nonrelaxed_interior(ptr %pa, ptr %py, float %k) {
; FLAGS-LABEL: @test_nonrelaxed_interior(
; FLAGS:  for.body:
; FLAGS:    %b = fmul float %v, %k
; FLAGS:    %m = fmul fast float %b, %y
; FLAGS:    %sum.next = fadd fast float %m, %sum
; FLAGS:  for.end:
; FLAGS-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %yp = getelementptr float, ptr %py, i32 %i
  %y = load float, ptr %yp, align 4

  %b = fmul float %v, %k
  %m = fmul fast float %b, %y
  %sum.next = fadd fast float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 false}
