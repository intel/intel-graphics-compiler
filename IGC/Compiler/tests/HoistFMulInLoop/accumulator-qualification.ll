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
; HoistFMulInLoopPass: what is an accumulator (step 1)
; ------------------------------------------------
;
; Step 1: a phi qualifies when it is `sum = phi + product` or `sum = phi - product` with a
; single back edge, relaxed math, entries of 0.0 or one seed, and nothing in the loop that
; reads the partial sums. The functions after the first marker run with module-level relaxed
; math on; the one after the second is judged by the flags on its own instructions.

;--- relaxed.ll
; The accumulator phi is the first operand of the fadd, not the second as in the
; other tests. The pattern is the same and the operand order must be preserved.

define void @test_fadd_operand_order(ptr %pa, float %k) {
; CHECK-LABEL: @test_fadd_operand_order(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %sum, %v
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
  %m = fmul float %v, %k
  %sum.next = fadd float %sum, %m

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; The accumulator is a double. Scalar half, float and double are all accepted, and the
; compensation has to be built in the same type.

define void @test_double_accumulator(ptr %pa, double %k) {
; CHECK-LABEL: @test_double_accumulator(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd double %v, %sum
; CHECK:  for.end:
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul double %sum.next, %k
; CHECK:    store double [[H]], ptr %pa, align 8
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi double [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to double
  %m = fmul double %v, %k
  %sum.next = fadd double %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store double %sum.next, ptr %pa, align 8
  ret void
}

; The accumulator phi has three inputs: two ways into the loop, both 0.0, and the
; back edge. A single non-zero seed is only handled for a phi with exactly two inputs,
; but all-zero entries need no seed at all, so this still hoists.

define void @test_two_entry_zero_seeds(ptr %pa, i32 %n, float %k) {
; CHECK-LABEL: @test_two_entry_zero_seeds(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  %c0 = icmp sgt i32 %n, 4
  br i1 %c0, label %a, label %b

a:
  br label %for.body

b:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %a ], [ 0, %b ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %a ], [ 0.000000e+00, %b ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Accumulator is seeded with a non-zero value defined outside the loop.
; The phi init must be rewritten to 0.0 and %init re-added after the loop, after
; the compensating multiply: sum_old = sum_new * k + init.
; Isolates the init rewrite only -- single factor, single accumulator.

define void @test_nonzero_init(ptr %pa, float %k, float %init) {
; CHECK-LABEL: @test_nonzero_init(
; CHECK:  for.body:
; CHECK:    %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k
; CHECK:    [[S:%[A-Za-z0-9_.]+]] = fadd float [[H]], %init
; CHECK:    store float [[S]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ %init, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; An accumulator that subtracts the product is minus a sum of products, and scales the same
; way: the loop subtracts %v, and the sum is multiplied by %k after it.

define void @test_fsub_accumulator(ptr %pa, float %k) {
; CHECK-LABEL: @test_fsub_accumulator(
; CHECK:  for.body:
; CHECK:    %sum.next = fsub float %sum, %v
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
  %m = fmul float %v, %k
  %sum.next = fsub float %sum, %m

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; The phi as the subtrahend, `product - phi`, is not an accumulation: the sign of the sum
; flips every iteration, so it is not the sum of the products and scaling it is wrong.

define void @test_fsub_phi_subtrahend(ptr %pa, float %k) {
; CHECK-LABEL: @test_fsub_phi_subtrahend(
; CHECK:  for.body:
; CHECK:    %m = fmul float %v, %k
; CHECK:    %sum.next = fsub float %m, %sum
; CHECK:  for.end:
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fsub float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; The running sum is observed inside the loop by %side. Partial sums would be
; wrong after the factor is pulled out, so the accumulator must be rejected.
; The fadd still consuming %m (not %v) is the signal that nothing was hoisted.

define void @test_extra_fsum_use(ptr %pa, ptr %ps, float %k) {
; CHECK-LABEL: @test_extra_fsum_use(
; CHECK:  for.body:
; CHECK:    %m = fmul float %v, %k
; CHECK:    %sum.next = fadd float %m, %sum
; CHECK:    %side = fmul float %sum.next, %v
; CHECK:  for.end:
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum
  %side = fmul float %sum.next, %v
  store float %side, ptr %ps, align 4

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Same obligation as test_extra_fsum_use, other half: the partial sum is
; observed through the PHI rather than through the fadd. Also must be rejected.

define void @test_extra_phi_use(ptr %pa, ptr %ps, float %k) {
; CHECK-LABEL: @test_extra_phi_use(
; CHECK:  for.body:
; CHECK:    %m = fmul float %v, %k
; CHECK:    %sum.next = fadd float %m, %sum
; CHECK:    %side = fmul float %sum, %v
; CHECK:  for.end:
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum
  %side = fmul float %sum, %v
  store float %side, ptr %ps, align 4

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; The product feeding the accumulator has a second reader, %keep, so it is not
; private to the accumulator: taking %k out of it would change what %keep sees, with
; nothing to compensate it. The accumulator must be rejected outright.

define void @test_multi_use_root(ptr %pa, ptr %ps, float %k) {
; CHECK-LABEL: @test_multi_use_root(
; CHECK:  for.body:
; CHECK:    %m = fmul float %v, %k
; CHECK:    %sum.next = fadd float %m, %sum
; CHECK:    %keep = fadd float %m, %v
; CHECK:  for.end:
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum
  %keep = fadd float %m, %v
  store float %keep, ptr %ps, align 4

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Only scalar half, float and double accumulators are recognised. A vector one is left
; alone.

define void @test_vector_accumulator(ptr %pa, ptr %px, <2 x float> %k) {
; CHECK-LABEL: @test_vector_accumulator(
; CHECK:  for.body:
; CHECK:    %m = fmul <2 x float> %x, %k
; CHECK:    %sum.next = fadd <2 x float> %m, %sum
; CHECK:  for.end:
; CHECK-NEXT: store <2 x float> %sum.next, ptr %pa, align 8
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi <2 x float> [ zeroinitializer, %entry ], [ %sum.next, %for.body ]

  %xp = getelementptr <2 x float>, ptr %px, i32 %i
  %x = load <2 x float>, ptr %xp, align 8
  %m = fmul <2 x float> %x, %k
  %sum.next = fadd <2 x float> %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store <2 x float> %sum.next, ptr %pa, align 8
  ret void
}

; The product is computed before the loop (as loop-invariant code motion would leave
; it), so what the loop accumulates is itself an invariant value and there is no
; product in the loop to take factors out of. Nothing may be transformed.

define void @test_root_outside_loop(ptr %pa, float %k1, float %k2) {
; CHECK-LABEL: @test_root_outside_loop(
; CHECK:  entry:
; CHECK:    %m = fmul float %k1, %k2
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %m, %sum
; CHECK:  for.end:
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  %m = fmul float %k1, %k2
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; A constant seed costs nothing to keep: after the loop it is an immediate, where a seed
; that is a value has to stay live through the loop to be added there.

define void @test_constant_seed(ptr %pa, float %k) {
; CHECK-LABEL: @test_constant_seed(
; CHECK:  for.body:
; CHECK:    %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k
; CHECK-NEXT: [[S:%[A-Za-z0-9_.]+]] = fadd float [[H]], 2.500000e+00
; CHECK-NEXT: store float [[S]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 2.500000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; The seed is the very factor being taken out: sum = k + k * sum(x) is k * (1 + sum(x)), and
; the seed is still added after the loop as the value it is, not scaled with the sum.

define void @test_seed_is_the_factor(ptr %pa, float %k) {
; CHECK-LABEL: @test_seed_is_the_factor(
; CHECK:  for.body:
; CHECK:    %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k
; CHECK-NEXT: [[S:%[A-Za-z0-9_.]+]] = fadd float [[H]], %k
; CHECK-NEXT: store float [[S]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ %k, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; A seed of -0.0 is a zero: adding it back after the loop changes nothing, so there is no
; seed to keep and nothing is added.

define void @test_negative_zero_seed(ptr %pa, float %k) {
; CHECK-LABEL: @test_negative_zero_seed(
; CHECK:  for.body:
; CHECK:    %sum = phi float [ -0.000000e+00, %entry ], [ %sum.next, %for.body ]
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k
; CHECK-NEXT: store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ -0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; A seed on a subtracting accumulator: sum = init - k * sum(x). The loop subtracts %v from
; 0.0 and what comes out is scaled by %k, with the seed added after it all the same.

define void @test_seeded_fsub_accumulator(ptr %pa, float %k, float %init) {
; CHECK-LABEL: @test_seeded_fsub_accumulator(
; CHECK:  for.body:
; CHECK:    %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]
; CHECK:    %sum.next = fsub float %sum, %v
; CHECK:  for.end:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k
; CHECK-NEXT: [[S:%[A-Za-z0-9_.]+]] = fadd float [[H]], %init
; CHECK-NEXT: store float [[S]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ %init, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fsub float %sum, %m

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
; Relaxed math is judged per instruction here (module-level FastRelaxedMath is off).
; The product is `fast` but the accumulating fadd is not, so the sum may not be
; reassociated: scaling the whole sum after the loop is exactly such a change.

define void @test_nonrelaxed_fadd(ptr %pa, float %k) {
; FLAGS-LABEL: @test_nonrelaxed_fadd(
; FLAGS:  for.body:
; FLAGS:    %m = fmul fast float %v, %k
; FLAGS:    %sum.next = fadd float %m, %sum
; FLAGS:  for.end:
; FLAGS-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul fast float %v, %k
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
!2 = !{!"FastRelaxedMath", i1 false}
