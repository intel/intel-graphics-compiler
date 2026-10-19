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
; HoistFMulInLoopPass: what leaves and what the code becomes, plain cases (steps 3 and 4)
; ------------------------------------------------
;
; Steps 3 and 4 where nothing outside the tree reads a product: which factors leave, what the
; product and the sums become, and the code that makes up for the factors after the loop,
; for each way the sum can be read there.

; The simplest hoist: sum += v * k with the invariant on the right. The accumulator
; reads %v instead, and %k is applied to the sum after the loop. One factor, one
; accumulator, no sharing.

define void @test_inv_rhs(ptr %pa, float %k) {
; CHECK-LABEL: @test_inv_rhs(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %v, %sum
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
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Operand-order symmetry of test_inv_rhs: the invariant is the left operand.
; The result must be identical.

define void @test_inv_lhs(ptr %pa, float %k) {
; CHECK-LABEL: @test_inv_lhs(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %v, %sum
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
  %m = fmul float %k, %v
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Both operands of %ki are invariant, so %ki is itself just two factors: the product
; is taken out as a whole and applied after the loop as %k1 * %k2. Isolates that and
; the construction of a multi-factor epilogue. %ki is only read, never rewritten.

define void @test_both_invariant(ptr %pa, float %k1, float %k2) {
; CHECK-LABEL: @test_both_invariant(
; CHECK:  for.body:
; CHECK:    %ki = fmul float %k1, %k2
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK:    [[P:%[A-Za-z0-9_.]+]] = fmul float %k1, %k2
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, [[P]]
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %ki = fmul float %k1, %k2
  %m = fmul float %v, %ki
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; The invariant is paired with an operand that is itself a product (%a = %v * %v).
; Nothing outside the tree reads %a, so the hoist happens: the accumulator reads %a
; directly and %k is applied after the loop. %a keeps computing %v * %v unchanged.

define void @test_nonleaf_operand(ptr %pa, float %k) {
; CHECK-LABEL: @test_nonleaf_operand(
; CHECK:  for.body:
; CHECK:    %a = fmul float %v, %v
; CHECK:    %sum.next = fadd float %a, %sum
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
  %a = fmul float %v, %v
  %m = fmul float %a, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Base case: the whole product is invariant, sum += %k1 * %k2. The sum of n equal products
; is the count times the product, so the loop is left counting 1.0 per iteration and the
; product is applied once after it.

define void @test_invariant_root(ptr %pa, float %k1, float %k2) {
; CHECK-LABEL: @test_invariant_root(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float 1.000000e+00, %sum
; CHECK:  for.end:
; CHECK:    [[P:%[A-Za-z0-9_.]+]] = fmul float %k1, %k2
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, [[P]]
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %m = fmul float %k1, %k2
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Squaring twice, q2 = (x*x)*(x*x), puts %x on four paths into the root, so its
; invariant %k is owed four times: %k^4 after the loop. The factor count has to follow
; the paths through the shared nodes, not the number of nodes -- one contribution per
; node would give %k^2. The power is built by squaring: two multiplies, not three.

define void @test_repeated_squaring(ptr %pa, float %k) {
; CHECK-LABEL: @test_repeated_squaring(
; CHECK:  for.body:
; CHECK:    %q = fmul float %v, %v
; CHECK:    %q2 = fmul float %q, %q
; CHECK:    %sum.next = fadd float %q2, %sum
; CHECK:  for.end:
; CHECK:    [[P1:%[A-Za-z0-9_.]+]] = fmul float %k, %k
; CHECK:    [[P2:%[A-Za-z0-9_.]+]] = fmul float [[P1]], [[P1]]
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, [[P2]]
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %x = fmul float %v, %k
  %q = fmul float %x, %x
  %q2 = fmul float %q, %q
  %sum.next = fadd float %q2, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Five squarings put %k on 32 paths. The power is built by squaring, five multiplies, and
; not by multiplying 31 times.

define void @test_deep_squaring(ptr %pa, float %k) {
; CHECK-LABEL: @test_deep_squaring(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %q5, %sum
; CHECK:  for.end:
; CHECK-NEXT: [[P1:%[A-Za-z0-9_.]+]] = fmul float %k, %k
; CHECK-NEXT: [[P2:%[A-Za-z0-9_.]+]] = fmul float [[P1]], [[P1]]
; CHECK-NEXT: [[P3:%[A-Za-z0-9_.]+]] = fmul float [[P2]], [[P2]]
; CHECK-NEXT: [[P4:%[A-Za-z0-9_.]+]] = fmul float [[P3]], [[P3]]
; CHECK-NEXT: [[P5:%[A-Za-z0-9_.]+]] = fmul float [[P4]], [[P4]]
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, [[P5]]
; CHECK-NEXT: store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %x = fmul float %v, %k
  %q1 = fmul float %x, %x
  %q2 = fmul float %q1, %q1
  %q3 = fmul float %q2, %q2
  %q4 = fmul float %q3, %q3
  %q5 = fmul float %q4, %q4
  %sum.next = fadd float %q5, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Sixty-four squarings would put %k on 2^64 paths, more than a 64-bit count can hold. The
; loop is left alone, and not hoisted with a wrong exponent.

define void @test_squaring_overflow(ptr %pa, float %k) {
; CHECK-LABEL: @test_squaring_overflow(
; CHECK:  for.body:
; CHECK:    %x = fmul float %v, %k
; CHECK:    %q1 = fmul float %x, %x
; CHECK:    %q64 = fmul float %q63, %q63
; CHECK:    %sum.next = fadd float %q64, %sum
; CHECK:  for.end:
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %x = fmul float %v, %k
  %q1 = fmul float %x, %x
  %q2 = fmul float %q1, %q1
  %q3 = fmul float %q2, %q2
  %q4 = fmul float %q3, %q3
  %q5 = fmul float %q4, %q4
  %q6 = fmul float %q5, %q5
  %q7 = fmul float %q6, %q6
  %q8 = fmul float %q7, %q7
  %q9 = fmul float %q8, %q8
  %q10 = fmul float %q9, %q9
  %q11 = fmul float %q10, %q10
  %q12 = fmul float %q11, %q11
  %q13 = fmul float %q12, %q12
  %q14 = fmul float %q13, %q13
  %q15 = fmul float %q14, %q14
  %q16 = fmul float %q15, %q15
  %q17 = fmul float %q16, %q16
  %q18 = fmul float %q17, %q17
  %q19 = fmul float %q18, %q18
  %q20 = fmul float %q19, %q19
  %q21 = fmul float %q20, %q20
  %q22 = fmul float %q21, %q21
  %q23 = fmul float %q22, %q22
  %q24 = fmul float %q23, %q23
  %q25 = fmul float %q24, %q24
  %q26 = fmul float %q25, %q25
  %q27 = fmul float %q26, %q26
  %q28 = fmul float %q27, %q27
  %q29 = fmul float %q28, %q28
  %q30 = fmul float %q29, %q29
  %q31 = fmul float %q30, %q30
  %q32 = fmul float %q31, %q31
  %q33 = fmul float %q32, %q32
  %q34 = fmul float %q33, %q33
  %q35 = fmul float %q34, %q34
  %q36 = fmul float %q35, %q35
  %q37 = fmul float %q36, %q36
  %q38 = fmul float %q37, %q37
  %q39 = fmul float %q38, %q38
  %q40 = fmul float %q39, %q39
  %q41 = fmul float %q40, %q40
  %q42 = fmul float %q41, %q41
  %q43 = fmul float %q42, %q42
  %q44 = fmul float %q43, %q43
  %q45 = fmul float %q44, %q44
  %q46 = fmul float %q45, %q45
  %q47 = fmul float %q46, %q46
  %q48 = fmul float %q47, %q47
  %q49 = fmul float %q48, %q48
  %q50 = fmul float %q49, %q49
  %q51 = fmul float %q50, %q50
  %q52 = fmul float %q51, %q51
  %q53 = fmul float %q52, %q52
  %q54 = fmul float %q53, %q53
  %q55 = fmul float %q54, %q54
  %q56 = fmul float %q55, %q55
  %q57 = fmul float %q56, %q56
  %q58 = fmul float %q57, %q57
  %q59 = fmul float %q58, %q58
  %q60 = fmul float %q59, %q59
  %q61 = fmul float %q60, %q60
  %q62 = fmul float %q61, %q61
  %q63 = fmul float %q62, %q62
  %q64 = fmul float %q63, %q63
  %sum.next = fadd float %q64, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; The expression is a DAG, not a tree: %q squares %x, so %x's invariant factor must
; be counted TWICE in the epilogue. Pins the factor collection as a multiset
; combined per edge -- a set, or one contribution per node, silently drops a %k.

define void @test_shared_operand(ptr %pa, float %k) {
; CHECK-LABEL: @test_shared_operand(
; CHECK:  for.body:
; CHECK:    %q = fmul float %v, %v
; CHECK:    %sum.next = fadd float %q, %sum
; CHECK:  for.end:
; CHECK:    [[P:%[A-Za-z0-9_.]+]] = fmul float %k, %k
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, [[P]]
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %x = fmul float %v, %k
  %q = fmul float %x, %x
  %sum.next = fadd float %q, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; One invariant-bearing sub-product %s feeds two independent accumulators. Both must
; hoist, each picking up %k. This is the pattern the pass was extended for, and it
; only works because the tree is collected across every accumulator in the loop:
; handled one accumulator at a time, %m2 would look like a foreign reader of %s and
; both would be refused. Regression net for that decision.

define void @test_multi_accumulator(ptr %pa1, ptr %pa2, ptr %py, ptr %pb1, ptr %pb2, float %k) {
; CHECK-LABEL: @test_multi_accumulator(
; CHECK:  for.body:
; CHECK:    %x = fmul float %v, %k
; CHECK:    %s = fmul float %v, %y
; CHECK:    %m1 = fmul float %s, %a1
; CHECK:    %m2 = fmul float %s, %a2
; CHECK:    %sum1.next = fadd float %m1, %sum1
; CHECK:    %sum2.next = fadd float %m2, %sum2
; CHECK:  for.end:
; CHECK:    [[H1:%[A-Za-z0-9_.]+]] = fmul float %sum1.next, %k
; CHECK:    store float [[H1]], ptr %pa1, align 4
; CHECK:    [[H2:%[A-Za-z0-9_.]+]] = fmul float %sum2.next, %k
; CHECK:    store float [[H2]], ptr %pa2, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum1 = phi float [ 0.000000e+00, %entry ], [ %sum1.next, %for.body ]
  %sum2 = phi float [ 0.000000e+00, %entry ], [ %sum2.next, %for.body ]

  %v = sitofp i32 %i to float
  %yp = getelementptr float, ptr %py, i32 %i
  %y = load float, ptr %yp, align 4
  %a1p = getelementptr float, ptr %pb1, i32 %i
  %a1 = load float, ptr %a1p, align 4
  %a2p = getelementptr float, ptr %pb2, i32 %i
  %a2 = load float, ptr %a2p, align 4

  %x = fmul float %v, %k
  %s = fmul float %x, %y
  %m1 = fmul float %s, %a1
  %m2 = fmul float %s, %a2
  %sum1.next = fadd float %m1, %sum1
  %sum2.next = fadd float %m2, %sum2

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum1.next, ptr %pa1, align 4
  store float %sum2.next, ptr %pa2, align 4
  ret void
}

; Two readers of the sum after the loop, in the same block. Both are owed the factor:
; the compensation is made once, in front of the first of them, and every reader there
; uses it. Compensating only the first reader leaves the second one on the raw sum.

define void @test_multi_reader(ptr %pa, ptr %pb, float %k) {
; CHECK-LABEL: @test_multi_reader(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k
; CHECK-NEXT: %u1 = fadd float [[H]], 1.000000e+00
; CHECK-NEXT: %u2 = fadd float [[H]], 2.000000e+00
; CHECK-NEXT: store float %u1, ptr %pa, align 4
; CHECK-NEXT: store float %u2, ptr %pb, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  %u1 = fadd float %sum.next, 1.000000e+00
  %u2 = fadd float %sum.next, 2.000000e+00
  store float %u1, ptr %pa, align 4
  store float %u2, ptr %pb, align 4
  ret void
}

; The sum after the loop is read by a phi with several inputs. A multiply cannot go in
; front of a phi, so the compensation is made at the end of the incoming block, on that
; edge only. The other input of the phi is not the loop's sum and is left alone.

define void @test_exit_phi_edge(ptr %pa, i32 %n, float %k) {
; CHECK-LABEL: @test_exit_phi_edge(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k
; CHECK-NEXT: br label %merge
; CHECK:  merge:
; CHECK-NEXT: %r = phi float [ [[H]], %for.end ], [ 0.000000e+00, %entry ]
; CHECK-NEXT: store float %r, ptr %pa, align 4
;
entry:
  %c0 = icmp sgt i32 %n, 0
  br i1 %c0, label %for.body, label %merge

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, %n
  br i1 %cond, label %for.end, label %for.body

for.end:
  br label %merge

merge:
  %r = phi float [ %sum.next, %for.end ], [ 0.000000e+00, %entry ]
  store float %r, ptr %pa, align 4
  ret void
}

; The sum leaves the loop through a single-input exit phi (LCSSA form). The
; compensation goes right after the phis, and everything that read the phi -- both
; stores here -- reads the compensated value instead.

define void @test_exit_lcssa_phi(ptr %pa, ptr %pb, float %k) {
; CHECK-LABEL: @test_exit_lcssa_phi(
; CHECK:  for.body:
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  for.end:
; CHECK-NEXT: %sum.lcssa = phi float [ %sum.next, %for.body ]
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.lcssa, %k
; CHECK-NEXT: store float [[H]], ptr %pa, align 4
; CHECK-NEXT: store float [[H]], ptr %pb, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  %sum.lcssa = phi float [ %sum.next, %for.body ]
  store float %sum.lcssa, ptr %pa, align 4
  store float %sum.lcssa, ptr %pb, align 4
  ret void
}

; The loop exits from its header, so what leaves it is the accumulator phi itself and
; not the fadd. Readers of the phi after the loop are owed the factor just the same.

define void @test_exit_reads_phi(ptr %pa, float %k) {
; CHECK-LABEL: @test_exit_reads_phi(
; CHECK:  body:
; CHECK:    %sum.next = fadd float %v, %sum
; CHECK:  exit:
; CHECK-NEXT: [[H:%[A-Za-z0-9_.]+]] = fmul float %sum, %k
; CHECK-NEXT: store float [[H]], ptr %pa, align 4
;
entry:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %i.next, %body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %body ]
  %c = icmp slt i32 %i, 8
  br i1 %c, label %body, label %exit

body:
  %v = sitofp i32 %i to float
  %m = fmul float %v, %k
  %sum.next = fadd float %m, %sum
  %i.next = add i32 %i, 1
  br label %header

exit:
  store float %sum, ptr %pa, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
