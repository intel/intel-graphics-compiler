;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-fold-provable-fcmp -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; FoldProvableFCmp replaces an fcmp that FPRangeAnalysis can decide with a
; constant, then deletes the control flow that makes dead. The first section
; covers that clean-up. The rest return the compare directly, so each one only
; asks whether it was decided.
;
; ------------------------------------------------

declare void @slow()
declare float @llvm.exp.f32(float)
declare float @llvm.exp2.f32(float)
declare float @llvm.cos.f32(float)
declare float @llvm.maxnum.f32(float, float)
declare float @llvm.minnum.f32(float, float)
declare float @llvm.genx.GenISA.fsat.f32(float)
declare half @llvm.genx.GenISA.fsat.f16(half)
declare float @llvm.fabs.f32(float)
declare float @llvm.sqrt.f32(float)
declare float @llvm.log.f32(float)
declare float @llvm.powi.f32(float, i32)

; ===== Control flow =====
; sitofp i16 is in [-32768, 32767] and never NaN, so these need no fast-math flags.

; CHECK-LABEL: define float @fold_guard(
; CHECK-NOT:     fcmp
; CHECK-NOT:     call void @slow
define float @fold_guard(i16 %i) {
entry:
  %x = sitofp i16 %i to float
  %guard = fcmp oge float %x, 1.000000e+06
  br i1 %guard, label %slow, label %done
slow:
  call void @slow()
  br label %done
done:
  ret float %x
}

; Folding both compares leaves `or i1 false, false`, which has to be simplified
; before the branch can fold.
; CHECK-LABEL: define float @fold_guard_or(
; CHECK-NOT:     fcmp
; CHECK-NOT:     or i1
; CHECK-NOT:     call void @slow
define float @fold_guard_or(i16 %i) {
entry:
  %x = sitofp i16 %i to float
  %hi = fcmp oge float %x, 1.000000e+06
  %lo = fcmp ole float %x, -1.000000e+06
  %guard = or i1 %hi, %lo
  br i1 %guard, label %slow, label %done
slow:
  call void @slow()
  br label %done
done:
  ret float %x
}

; `or %unknown, true` decides the branch without the other arm being provable.
; CHECK-LABEL: define float @fold_or_short_circuit_true(
; CHECK-NOT:     or i1
; CHECK-NOT:     call void @slow
define float @fold_or_short_circuit_true(i16 %i, float %v) {
entry:
  %x = sitofp i16 %i to float
  %unknown = fcmp olt float %v, 1.000000e+00
  %inrange = fcmp olt float %x, 1.000000e+06
  %guard = or i1 %unknown, %inrange
  br i1 %guard, label %done, label %slow
slow:
  call void @slow()
  br label %done
done:
  ret float %x
}

; CHECK-LABEL: define float @fold_and_short_circuit_false(
; CHECK-NOT:     and i1
; CHECK-NOT:     call void @slow
define float @fold_and_short_circuit_false(i16 %i, float %v) {
entry:
  %x = sitofp i16 %i to float
  %unknown = fcmp olt float %v, 1.000000e+00
  %outofrange = fcmp oge float %x, 1.000000e+06
  %guard = and i1 %unknown, %outofrange
  br i1 %guard, label %slow, label %done
slow:
  call void @slow()
  br label %done
done:
  ret float %x
}

; Only one arm is decided, and false does not decide an `or`: the condition
; collapses to the other compare and the slow path stays.
; CHECK-LABEL: define float @keep_guard_on_undecided_or(
; CHECK-NOT:     or i1
; CHECK:         fcmp olt float %v
; CHECK:         call void @slow
define float @keep_guard_on_undecided_or(i16 %i, float %v) {
entry:
  %x = sitofp i16 %i to float
  %unknown = fcmp olt float %v, 1.000000e+00
  %outofrange = fcmp oge float %x, 1.000000e+06
  %guard = or i1 %outofrange, %unknown
  br i1 %guard, label %slow, label %done
slow:
  call void @slow()
  br label %done
done:
  ret float %x
}

; The merge block loses the dead predecessor's PHI entry.
; CHECK-LABEL: define float @fold_guard_with_phi(
; CHECK-NOT:     fcmp
; CHECK-NOT:     fmul
; CHECK:         ret float %x
define float @fold_guard_with_phi(i16 %i) {
entry:
  %x = sitofp i16 %i to float
  %guard = fcmp ogt float %x, 1.000000e+06
  br i1 %guard, label %big, label %merge
big:
  %y = fmul float %x, 2.000000e+00
  br label %merge
merge:
  %r = phi float [ %y, %big ], [ %x, %entry ]
  ret float %r
}

; A guard inside a loop folds, and the loop itself survives.
; CHECK-LABEL: define void @fold_guard_in_loop(
; CHECK-NOT:     fcmp
; CHECK-NOT:     call void @slow
; CHECK:         phi i32
; CHECK:         icmp
define void @fold_guard_in_loop(i16 %i, i32 %n) {
entry:
  %x = sitofp i16 %i to float
  br label %loop
loop:
  %iv = phi i32 [ 0, %entry ], [ %iv.next, %latch ]
  %guard = fcmp ogt float %x, 1.000000e+06
  br i1 %guard, label %rare, label %latch
rare:
  call void @slow()
  br label %latch
latch:
  %iv.next = add i32 %iv, 1
  %done = icmp eq i32 %iv.next, %n
  br i1 %done, label %exit, label %loop
exit:
  ret void
}

; ===== Bounds and NaN =====

; pi * sigmoid(x) is in [0, pi].
; CHECK-LABEL: define i1 @fold_through_logistic(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 true
define i1 @fold_through_logistic(float %logit) {
  %e = fmul fast float %logit, 0xBFF7154760000000
  %ex = call fast float @llvm.exp2.f32(float %e)
  %d = fadd fast float %ex, 1.000000e+00
  %sig = fdiv fast float 1.000000e+00, %d
  %ang = fmul fast float 0x400921FB60000000, %sig
  %c = fcmp ult float %ang, 1.000000e+06
  ret i1 %c
}

; Without nnan anywhere the NaN %logit may hold reaches the compare, and a NaN
; operand makes every ordered predicate false whatever the bounds.
; CHECK-LABEL: define i1 @keep_without_nnan(
; CHECK:         fcmp ult float
define i1 @keep_without_nnan(float %logit) {
  %e = fmul float %logit, 0xBFF7154760000000
  %ex = call float @llvm.exp2.f32(float %e)
  %d = fadd float %ex, 1.000000e+00
  %sig = fdiv float 1.000000e+00, %d
  %ang = fmul float 0x400921FB60000000, %sig
  %c = fcmp ult float %ang, 1.000000e+06
  ret i1 %c
}

; The value crosses a vector lane, as after SROA rewrites an alloca round-trip.
; CHECK-LABEL: define i1 @fold_through_vector_lane(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 true
define i1 @fold_through_vector_lane(i16 %i) {
  %x = sitofp i16 %i to float
  %ins = insertelement <3 x float> undef, float %x, i32 2
  %ext = extractelement <3 x float> %ins, i32 2
  %c = fcmp ult float %ext, 1.000000e+06
  ret i1 %c
}

; nnan alone leaves the value unbounded.
; CHECK-LABEL: define i1 @keep_on_unbounded(
; CHECK:         fcmp ult float
define i1 @keep_on_unbounded(float %v) {
  %x = fmul fast float %v, 0x400921FB60000000
  %c = fcmp ult float %x, 1.000000e+06
  ret i1 %c
}

; maxnum/minnum return the non-NaN operand (IEEE 754-2008), so a clamp is bounded
; and non-NaN with no fast-math flags.
; CHECK-LABEL: define i1 @fold_through_clamp(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 false
define i1 @fold_through_clamp(float %v) {
  %lo = call float @llvm.maxnum.f32(float %v, float 0.000000e+00)
  %x = call float @llvm.minnum.f32(float %lo, float 1.000000e+00)
  %c = fcmp oge float %x, 1.000000e+06
  ret i1 %c
}

; nnan leaves the exp argument unbounded, so exp overflows to +Inf and the fdiv is
; +Inf / (1 + +Inf) = NaN. The logistic pattern supplies bounds only and must not
; clear the NaN flag the fdiv sets.
; CHECK-LABEL: define i1 @keep_guard_on_overflowing_logistic(
; CHECK:         fcmp ole float
define i1 @keep_guard_on_overflowing_logistic(float %a, float %b) {
  %e = fmul nnan float %a, %b
  %ex = call float @llvm.exp.f32(float %e)
  %den = fadd float 1.000000e+00, %ex
  %sig = fdiv float %ex, %den
  %c = fcmp ole float %sig, 2.000000e+00
  ret i1 %c
}

; The same shape with nnan throughout: a NaN is poison, so the fdiv is non-NaN.
; The pattern's view of the bare exp argument must not block it.
; CHECK-LABEL: define i1 @fold_guard_on_nnan_logistic(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 true
define i1 @fold_guard_on_nnan_logistic(float %logit) {
  %ex = call fast float @llvm.exp2.f32(float %logit)
  %den = fadd fast float %ex, 1.000000e+00
  %sig = fdiv fast float %ex, %den
  %c = fcmp ult float %sig, 1.000000e+06
  ret i1 %c
}

; ===== Margin =====
; FPRangeAnalysis widens approximated results (fdiv, sqrt, exp, log, sin, cos) by
; 2^-5 of themselves, and computed bounds are relaxed by 2^-5 more before
; deciding; literals are charged nothing.

; 1/maxnum(v, 1) is [0, 1], but the arcp fdiv lowers to a reciprocal that can
; land an ULP above 1.0.
; CHECK-LABEL: define i1 @keep_at_bound(
; CHECK:         fcmp ogt float
define i1 @keep_at_bound(float %v) {
  %d = call float @llvm.maxnum.f32(float %v, float 1.000000e+00)
  %s = fdiv fast float 1.000000e+00, %d
  %c = fcmp ogt float %s, 1.000000e+00
  ret i1 %c
}

; [0, 1] widens to 1.03125 and relaxes to about 1.0635, clear of the literal 1.125.
; CHECK-LABEL: define i1 @fold_outside_margin(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 false
define i1 @fold_outside_margin(float %v) {
  %d = call float @llvm.maxnum.f32(float %v, float 1.000000e+00)
  %s = fdiv fast float 1.000000e+00, %d
  %c = fcmp ogt float %s, 1.125000e+00
  ret i1 %c
}

; Approximated results are widened where they are computed: cos past [-1, 1],
; which is why such a clamp gets written; cos - 1 above zero, as the widening
; carries through the subtraction; and log(x) for x >= 1 below zero, since
; hardware log near 1 has an absolute error.
; CHECK-LABEL: define i1 @keep_near_approximated_bound(
; CHECK:         fcmp ogt float %x
; CHECK:         fcmp ogt float %y
; CHECK:         fcmp fast oge float %l
define i1 @keep_near_approximated_bound(float %t, float %v) {
  %x = call fast float @llvm.cos.f32(float %t)
  %c1 = fcmp ogt float %x, 1.000000e+00
  %y = fsub fast float %x, 1.000000e+00
  %c2 = fcmp ogt float %y, 0x3DDB7CDFE0000000
  %a = call fast float @llvm.fabs.f32(float %v)
  %p = fadd fast float %a, 1.000000e+00
  %l = call fast float @llvm.log.f32(float %p)
  %c3 = fcmp fast oge float %l, 0.000000e+00
  %o = or i1 %c1, %c2
  %c = or i1 %o, %c3
  ret i1 %c
}

; fdiv is lowered to mul(x, inv(y)). For |y| above 2^126, inv(y) flushes to zero,
; so the quotient can be zero even though x / y is a small normal.
; CHECK-LABEL: define i1 @keep_quotient_of_huge_divisor(
; CHECK:         fcmp ogt float
define i1 @keep_quotient_of_huge_divisor(i16 %i) {
  %x = sitofp i16 %i to float
  %a = call float @llvm.fabs.f32(float %x)
  %p = fadd float %a, 1.024000e+03
  %q = fdiv float %p, 0x47E0000000000000
  %c = fcmp ogt float %q, 0.000000e+00
  ret i1 %c
}

; CHECK-LABEL: define i1 @fold_quotient_of_large_divisor(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 true
define i1 @fold_quotient_of_large_divisor(i16 %i) {
  %x = sitofp i16 %i to float
  %a = call float @llvm.fabs.f32(float %x)
  %p = fadd float %a, 1.024000e+03
  %q = fdiv float %p, 0x4630000000000000
  %c = fcmp ogt float %q, 0.000000e+00
  ret i1 %c
}

; ===== minnum / maxnum =====
; minnum and maxnum return the other operand when one is NaN. sqrt of a negative
; sitofp is NaN, so maxnum(sqrt(x), -5.0) can be -5 although sqrt is never negative.

; CHECK-LABEL: define i1 @keep_minmax_of_maybe_nan(
; CHECK:         fcmp olt float %mx
; CHECK:         fcmp ogt float %mn
define i1 @keep_minmax_of_maybe_nan(i32 %i) {
  %x = sitofp i32 %i to float
  %s = call float @llvm.sqrt.f32(float %x)
  %mx = call float @llvm.maxnum.f32(float %s, float -5.000000e+00)
  %c1 = fcmp olt float %mx, 0.000000e+00
  %mn = call float @llvm.minnum.f32(float %s, float 1.000000e+10)
  %c2 = fcmp ogt float %mn, 1.000000e+06
  %c = or i1 %c1, %c2
  ret i1 %c
}

; A clamp of an unknown, possibly NaN, value still gets the literal's bound: if
; %v is NaN, maxnum returns 2.0.
; CHECK-LABEL: define i1 @fold_maxnum_of_unknown(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 false
define i1 @fold_maxnum_of_unknown(float %v) {
  %m = call float @llvm.maxnum.f32(float %v, float 2.000000e+00)
  %c = fcmp olt float %m, 1.000000e+00
  ret i1 %c
}

; ===== Saturate =====
; GenISA_fsat is [0, 1] and maps NaN to 0, so it needs no fast-math flags.

; Both sides of the range: if either compare stayed, the `or` would not fold.
; CHECK-LABEL: define i1 @fold_fsat_outside_range(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 false
define i1 @fold_fsat_outside_range(float %v) {
  %s = call float @llvm.genx.GenISA.fsat.f32(float %v)
  %above = fcmp ogt float %s, 2.000000e+00
  %below = fcmp olt float %s, -1.000000e+00
  %c = or i1 %above, %below
  ret i1 %c
}

; CHECK-LABEL: define i1 @keep_fsat_inside_range(
; CHECK:         fcmp ogt float
define i1 @keep_fsat_inside_range(float %v) {
  %s = call float @llvm.genx.GenISA.fsat.f32(float %v)
  %c = fcmp ogt float %s, 5.000000e-01
  ret i1 %c
}

; A zero bound that the producing construct guarantees skips the margin's
; smallest-normal floor. The walk sees through products, sums, fneg, fpext and
; maxnum with a literal, including of fsat; an upper bound of zero mirrors it.
; CHECK-LABEL: define i1 @fold_structural_zero(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 true
define i1 @fold_structural_zero(float %v, float %w, half %h) {
  %s = call float @llvm.genx.GenISA.fsat.f32(float %v)
  %c1 = fcmp oge float %s, 0.000000e+00
  %t = call float @llvm.genx.GenISA.fsat.f32(float %w)
  %prod = fmul float %s, %t
  %c2 = fcmp oge float %prod, 0.000000e+00
  %ha = call half @llvm.genx.GenISA.fsat.f16(half %h)
  %ext = fpext half %ha to float
  %c3 = fcmp oge float %ext, 0.000000e+00
  %av = call float @llvm.fabs.f32(float %v)
  %aw = call float @llvm.fabs.f32(float %w)
  %sum = fadd nnan float %av, %aw
  %c4 = fcmp oge float %sum, 0.000000e+00
  %mx = call float @llvm.maxnum.f32(float %v, float 0.000000e+00)
  %c5 = fcmp oge float %mx, 0.000000e+00
  %neg = fneg float %s
  %c6 = fcmp ole float %neg, 0.000000e+00
  %scaled = fmul float %s, -2.000000e+00
  %c7 = fcmp ole float %scaled, 0.000000e+00
  %a1 = and i1 %c1, %c2
  %a2 = and i1 %a1, %c3
  %a3 = and i1 %a2, %c4
  %a4 = and i1 %a3, %c5
  %a5 = and i1 %a4, %c6
  %c = and i1 %a5, %c7
  ret i1 %c
}

; The same zeros reached by a subtraction keep the floor, as reassociation can
; take them past zero. So does maxnum with a non-negative side that may be NaN,
; since maxnum then returns the other side.
; CHECK-LABEL: define i1 @keep_arithmetic_zero(
; CHECK:         fcmp fast oge float %lo
; CHECK:         fcmp fast ole float %hi
; CHECK:         fcmp oge float %m
define i1 @keep_arithmetic_zero(float %v, i32 %i) {
  %a = call fast float @llvm.maxnum.f32(float %v, float 0.000000e+00)
  %b = call fast float @llvm.minnum.f32(float %a, float 1.000000e+00)
  %lo = fsub fast float 1.000000e+00, %b
  %c1 = fcmp fast oge float %lo, 0.000000e+00
  %hi = fsub fast float %b, 1.000000e+00
  %c2 = fcmp fast ole float %hi, 0.000000e+00
  %x = sitofp i32 %i to float
  %r = call float @llvm.sqrt.f32(float %x)
  %s = call float @llvm.genx.GenISA.fsat.f32(float %v)
  %d = fsub float 1.000000e+00, %s
  %m = call float @llvm.maxnum.f32(float %r, float %d)
  %c3 = fcmp oge float %m, 0.000000e+00
  %o = or i1 %c1, %c2
  %c = or i1 %o, %c3
  ret i1 %c
}

; A phi is structurally non-negative when every incoming value is.
; CHECK-LABEL: define i1 @fold_structural_zero_through_phi(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 true
define i1 @fold_structural_zero_through_phi(i1 %b, float %v, float %w) {
entry:
  br i1 %b, label %left, label %right
left:
  %s = call float @llvm.genx.GenISA.fsat.f32(float %v)
  br label %merge
right:
  %t = call float @llvm.genx.GenISA.fsat.f32(float %w)
  br label %merge
merge:
  %x = phi float [ %s, %left ], [ %t, %right ]
  %c = fcmp oge float %x, 0.000000e+00
  ret i1 %c
}

; ===== powi =====
; powi is emitted as pow, expanded to exp2(log2(|x|) * n) unless something folds it.

; Exponent 1 gives an approximation of |x| when expanded, so a negative base can
; come out positive. Exponent 0 gives exp2(-Inf * 0) = NaN for a zero base unless
; it is folded to 1; uge is true on NaN, so folding it to false would be wrong.
; CHECK-LABEL: define i1 @keep_powi_expansion(
; CHECK:         fcmp ogt float %p1
; CHECK:         fcmp uge float %p0
define i1 @keep_powi_expansion(i8 %i) {
  %x = sitofp i8 %i to float
  %a = call float @llvm.fabs.f32(float %x)
  %n = fsub float -1.000000e+00, %a
  %p1 = call float @llvm.powi.f32(float %n, i32 1)
  %c1 = fcmp ogt float %p1, 0.000000e+00
  %p0 = call float @llvm.powi.f32(float %x, i32 0)
  %c2 = fcmp uge float %p0, 2.000000e+00
  %c = or i1 %c1, %c2
  ret i1 %c
}

; CHECK-LABEL: define i1 @fold_powi_zero_of_nonzero(
; CHECK-NOT:     fcmp
; CHECK:         ret i1 false
define i1 @fold_powi_zero_of_nonzero(float %v) {
  %s = call float @llvm.genx.GenISA.fsat.f32(float %v)
  %x = fadd float %s, 1.000000e+00
  %p = call float @llvm.powi.f32(float %x, i32 0)
  %c = fcmp uge float %p, 2.000000e+00
  ret i1 %c
}
