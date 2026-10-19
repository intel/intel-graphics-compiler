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
; HoistFMulInLoopPass: what leaves when something outside the tree reads a product (steps 3 and 4)
; ------------------------------------------------
;
; Steps 3 and 4 where something outside the tree reads a product, in the loop or after it.
; Such a product keeps its value, and so does everything it is computed from; whatever sits
; above it can still leave.

; %x is read by an unrelated fadd, so it has to keep computing %v*%k every iteration.
; Taking %k out of %m would not remove a multiply from the loop -- %x stays for %ext --
; and would add one after it. Nothing may be transformed.

define void @test_leaf_alias_external(ptr %pa, ptr %pe, ptr %pt, float %k) {
; CHECK-LABEL: @test_leaf_alias_external(
; CHECK:  for.body:
; CHECK:    %x = fmul float %v, %k
; CHECK:    %m = fmul float %x, %t
; CHECK:    %sum.next = fadd float %m, %sum
; CHECK:    %ext = fadd float %x, 1.000000e+00
; CHECK:  for.end:
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %tp = getelementptr float, ptr %pt, i32 %i
  %t = load float, ptr %tp, align 4

  %x = fmul float %v, %k
  %m = fmul float %x, %t
  %sum.next = fadd float %m, %sum
  %ext = fadd float %x, 1.000000e+00
  store float %ext, ptr %pe, align 4

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Minimal form of a miscompile where an unrelated reader loses a factor. Taking %k2
; out of %x rewires %p, which changes %w, so %ext -- which reads %w -- silently loses
; %k2 while the accumulator path is compensated and looks fine. %w is read outside the
; tree, so it and everything beneath it must keep their values. Nothing in this loop
; may be transformed.

define void @test_nonleaf_external(ptr %pa, ptr %pe, ptr %py, ptr %pu, float %k, float %k2) {
; CHECK-LABEL: @test_nonleaf_external(
; CHECK:  for.body:
; CHECK:    %x = fmul float %v, %k2
; CHECK:    %p = fmul float %x, %y
; CHECK:    %w = fmul float %p, %k
; CHECK:    %m = fmul float %w, %u
; CHECK:    %sum.next = fadd float %m, %sum
; CHECK:    %ext = fadd float %w, 1.000000e+00
; CHECK:  for.end:
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %yp = getelementptr float, ptr %py, i32 %i
  %y = load float, ptr %yp, align 4
  %up = getelementptr float, ptr %pu, i32 %i
  %u = load float, ptr %up, align 4

  %x = fmul float %v, %k2
  %p = fmul float %x, %y
  %w = fmul float %p, %k
  %m = fmul float %w, %u
  %sum.next = fadd float %m, %sum
  %ext = fadd float %w, 1.000000e+00
  store float %ext, ptr %pe, align 4

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; An interior product, %w, is read after the loop as well as inside it. Taking %k2 out
; of %x would rewire %p and change %w, and the sum's compensation does not reach this
; store: it would see the last iteration's %w without %k2 -- while the sum, which is
; compensated, still comes out right. %w has to keep its value, and so does everything
; it is computed from.

define void @test_outside_loop_reader(ptr %pa, ptr %pw, ptr %py, float %k, float %k2) {
; CHECK-LABEL: @test_outside_loop_reader(
; CHECK:  for.body:
; CHECK:    %x = fmul float %v, %k2
; CHECK:    %p = fmul float %x, %y
; CHECK:    %w = fmul float %p, %k
; CHECK:    %m = fmul float %w, %t
; CHECK:    %sum.next = fadd float %m, %sum
; CHECK:  for.end:
; CHECK-NEXT: store float %w, ptr %pw, align 4
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %t = fadd float %v, 1.000000e+00
  %yp = getelementptr float, ptr %py, i32 %i
  %y = load float, ptr %yp, align 4

  %x = fmul float %v, %k2
  %p = fmul float %x, %y
  %w = fmul float %p, %k
  %m = fmul float %w, %t
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 2
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %w, ptr %pw, align 4
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Same as test_outside_loop_reader, with the interior product leaving the loop
; through an exit phi. A phi is no less a reader than any other instruction.

define void @test_outside_loop_lcssa_reader(ptr %pa, ptr %pw, ptr %py, float %k, float %k2) {
; CHECK-LABEL: @test_outside_loop_lcssa_reader(
; CHECK:  for.body:
; CHECK:    %x = fmul float %v, %k2
; CHECK:    %p = fmul float %x, %y
; CHECK:    %w = fmul float %p, %k
; CHECK:    %m = fmul float %w, %t
; CHECK:    %sum.next = fadd float %m, %sum
; CHECK:  for.end:
; CHECK-NEXT: %w.lcssa = phi float [ %w, %for.body ]
; CHECK-NEXT: store float %w.lcssa, ptr %pw, align 4
; CHECK-NEXT: store float %sum.next, ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %t = fadd float %v, 1.000000e+00
  %yp = getelementptr float, ptr %py, i32 %i
  %y = load float, ptr %yp, align 4

  %x = fmul float %v, %k2
  %p = fmul float %x, %y
  %w = fmul float %p, %k
  %m = fmul float %w, %t
  %sum.next = fadd float %m, %sum

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 2
  br i1 %cond, label %for.end, label %for.body

for.end:
  %w.lcssa = phi float [ %w, %for.body ]
  store float %w.lcssa, ptr %pw, align 4
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Cross-accumulator version: %X is shared by both accumulators, and each of its
; readers (%p1, %p2) is a product in a tree, so judged by its own readers alone %X
; looks free to rewrite. But %p2 is read by an unrelated fadd (%ext), so %p2 has to
; keep its value -- and so does everything it is computed from, %X and %m0 included.
; Taking %k0 out of %m0 anyway would change %p2 under %ext, and the second
; accumulator would never be compensated for it. Nothing in this loop may be
; transformed.

define void @test_sibling_bail(ptr %pa1, ptr %pa2, ptr %pe, ptr %py, ptr %pb1, ptr %pb2, float %k0, float %k2) {
; CHECK-LABEL: @test_sibling_bail(
; CHECK:  for.body:
; CHECK:    %m0 = fmul float %v, %k0
; CHECK:    %X = fmul float %m0, %y
; CHECK:    %p1 = fmul float %X, %s1
; CHECK:    %p2 = fmul float %X, %k2
; CHECK:    %d2 = fmul float %p2, %s2
; CHECK:    %sum1.next = fadd float %p1, %sum1
; CHECK:    %sum2.next = fadd float %d2, %sum2
; CHECK:    %ext = fadd float %p2, 1.000000e+00
; CHECK:  for.end:
; CHECK-NEXT: store float %sum1.next, ptr %pa1, align 4
; CHECK-NEXT: store float %sum2.next, ptr %pa2, align 4
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
  %s1p = getelementptr float, ptr %pb1, i32 %i
  %s1 = load float, ptr %s1p, align 4
  %s2p = getelementptr float, ptr %pb2, i32 %i
  %s2 = load float, ptr %s2p, align 4

  %m0 = fmul float %v, %k0
  %X = fmul float %m0, %y
  %p1 = fmul float %X, %s1
  %p2 = fmul float %X, %k2
  %d2 = fmul float %p2, %s2
  %sum1.next = fadd float %p1, %sum1
  %sum2.next = fadd float %d2, %sum2
  %ext = fadd float %p2, 1.000000e+00
  store float %ext, ptr %pe, align 4

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum1.next, ptr %pa1, align 4
  store float %sum2.next, ptr %pa2, align 4
  ret void
}

; Only %b has an outside reader (%ext), so %b and everything beneath it -- %a, and
; %k1 inside %b -- keep their values. %c = %b * %k2 sits above the dirty node and %k2
; has no other reader, so it alone is hoisted: %d reads %b directly, and %k2 is applied
; to the sum after the loop. Companion of test_sibling_bail, where the dirty
; node's operands are shared with another product and nothing can move.

define void @test_partial_hoist(ptr %pa, ptr %pe, ptr %pt, float %k1, float %k2) {
; CHECK-LABEL: @test_partial_hoist(
; CHECK:  for.body:
; CHECK:    %a = fmul float %v, %v
; CHECK:    %b = fmul float %a, %k1
; CHECK:    %d = fmul float %b, %t
; CHECK:    %sum.next = fadd float %d, %sum
; CHECK:    %ext = fadd float %b, 1.000000e+00
; CHECK:  for.end:
; CHECK:    [[H:%[A-Za-z0-9_.]+]] = fmul float %sum.next, %k2
; CHECK:    store float [[H]], ptr %pa, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %for.body ]

  %v = sitofp i32 %i to float
  %tp = getelementptr float, ptr %pt, i32 %i
  %t = load float, ptr %tp, align 4

  %a = fmul float %v, %v
  %b = fmul float %a, %k1
  %c = fmul float %b, %k2
  %d = fmul float %c, %t
  %sum.next = fadd float %d, %sum
  %ext = fadd float %b, 1.000000e+00
  store float %ext, ptr %pe, align 4

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum.next, ptr %pa, align 4
  ret void
}

; Partial match: %x is read outside the tree (%ext), so %x and %k0 inside it stay as they
; are. The accumulator whose product sits above %x still gives up its own factor %k1,
; while the one with nothing but %x under it is left alone.

define void @test_partial_match(ptr %pa1, ptr %pa2, ptr %pe, ptr %ps1, ptr %ps2, float %k0, float %k1) {
; CHECK-LABEL: @test_partial_match(
; CHECK:  for.body:
; CHECK:    %x = fmul float %v, %k0
; CHECK:    %p1 = fmul float %x, %s1
; CHECK:    %d1 = fmul float %p1, %k1
; CHECK:    %p2 = fmul float %x, %s2
; CHECK:    %sum1.next = fadd float %p1, %sum1
; CHECK:    %sum2.next = fadd float %p2, %sum2
; CHECK:    %ext = fadd float %x, 1.000000e+00
; CHECK:  for.end:
; CHECK-NEXT: [[H1:%[A-Za-z0-9_.]+]] = fmul float %sum1.next, %k1
; CHECK-NEXT: store float [[H1]], ptr %pa1, align 4
; CHECK-NEXT: store float %sum2.next, ptr %pa2, align 4
;
entry:
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %for.body ]
  %sum1 = phi float [ 0.000000e+00, %entry ], [ %sum1.next, %for.body ]
  %sum2 = phi float [ 0.000000e+00, %entry ], [ %sum2.next, %for.body ]

  %v = sitofp i32 %i to float
  %s1p = getelementptr float, ptr %ps1, i32 %i
  %s1 = load float, ptr %s1p, align 4
  %s2p = getelementptr float, ptr %ps2, i32 %i
  %s2 = load float, ptr %s2p, align 4

  %x = fmul float %v, %k0
  %p1 = fmul float %x, %s1
  %d1 = fmul float %p1, %k1
  %p2 = fmul float %x, %s2
  %sum1.next = fadd float %d1, %sum1
  %sum2.next = fadd float %p2, %sum2
  %ext = fadd float %x, 1.000000e+00
  store float %ext, ptr %pe, align 4

  %i.next = add i32 %i, 1
  %cond = icmp eq i32 %i.next, 8
  br i1 %cond, label %for.end, label %for.body

for.end:
  store float %sum1.next, ptr %pa1, align 4
  store float %sum2.next, ptr %pa2, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
