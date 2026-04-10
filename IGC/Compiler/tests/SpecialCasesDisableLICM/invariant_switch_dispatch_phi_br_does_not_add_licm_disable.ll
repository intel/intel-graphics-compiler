;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-special-cases-disable-licm -S < %s | FileCheck %s
; ------------------------------------------------
; SpecialCasesDisableLICM : LoopHasInvariantSwitchDispatch
; ------------------------------------------------

; A loop containing many phi+br blocks whose branch condition is an icmp
; defined in a *different* block (not in the phi+br block itself).
; This models the Int64Div emulation pattern: after inlining 30 copies of
; __igcbuiltin_s64_sdiv_dp, each copy produces a merge block of the form:
;
;   sdiv_exit:
;     %result = phi i64 [...]
;     br i1 %cmp_divisor_hi, ...   ; %cmp defined in loop header, not here
;
; Such blocks have size==2 (phi+br) and the branch condition traces back to
; an icmp eq i32 %divisor_hi, -1 where %divisor_hi is loop-invariant.
; The heuristic must not count these as dispatch blocks because the icmp
; is not local to the block — they are merge points, not BST dispatch nodes.
; LICM should remain enabled.

; CHECK-LABEL: @test_phi_br_external_icmp(
; CHECK-NOT: !{!"llvm.licm.disable"}

define spir_kernel void @test_phi_br_external_icmp(i32 %divisor_hi, i32 %n) {
entry:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %i.next, %latch ]
  ; icmp on loop-invariant value defined here, used as branch condition in
  ; all merge blocks below — but NOT local to those blocks.
  %cmp = icmp eq i32 %divisor_hi, -1
  br label %split0

split0:
  %c0 = icmp ult i32 %i, 10
  br i1 %c0, label %a0, label %b0
a0:
  br label %merge0
b0:
  br label %merge0
merge0:
  %d0 = phi i32 [ 0, %a0 ], [ 1, %b0 ]
  br i1 %cmp, label %special, label %split1

split1:
  %c1 = icmp ult i32 %i, 11
  br i1 %c1, label %a1, label %b1
a1:
  br label %merge1
b1:
  br label %merge1
merge1:
  %d1 = phi i32 [ 0, %a1 ], [ 1, %b1 ]
  br i1 %cmp, label %special, label %split2

split2:
  %c2 = icmp ult i32 %i, 12
  br i1 %c2, label %a2, label %b2
a2:
  br label %merge2
b2:
  br label %merge2
merge2:
  %d2 = phi i32 [ 0, %a2 ], [ 1, %b2 ]
  br i1 %cmp, label %special, label %split3

split3:
  %c3 = icmp ult i32 %i, 13
  br i1 %c3, label %a3, label %b3
a3:
  br label %merge3
b3:
  br label %merge3
merge3:
  %d3 = phi i32 [ 0, %a3 ], [ 1, %b3 ]
  br i1 %cmp, label %special, label %split4

split4:
  %c4 = icmp ult i32 %i, 14
  br i1 %c4, label %a4, label %b4
a4:
  br label %merge4
b4:
  br label %merge4
merge4:
  %d4 = phi i32 [ 0, %a4 ], [ 1, %b4 ]
  br i1 %cmp, label %special, label %split5

split5:
  %c5 = icmp ult i32 %i, 15
  br i1 %c5, label %a5, label %b5
a5:
  br label %merge5
b5:
  br label %merge5
merge5:
  %d5 = phi i32 [ 0, %a5 ], [ 1, %b5 ]
  br i1 %cmp, label %special, label %split6

split6:
  %c6 = icmp ult i32 %i, 16
  br i1 %c6, label %a6, label %b6
a6:
  br label %merge6
b6:
  br label %merge6
merge6:
  %d6 = phi i32 [ 0, %a6 ], [ 1, %b6 ]
  br i1 %cmp, label %special, label %split7

split7:
  %c7 = icmp ult i32 %i, 17
  br i1 %c7, label %a7, label %b7
a7:
  br label %merge7
b7:
  br label %merge7
merge7:
  %d7 = phi i32 [ 0, %a7 ], [ 1, %b7 ]
  br i1 %cmp, label %special, label %split8

split8:
  %c8 = icmp ult i32 %i, 18
  br i1 %c8, label %a8, label %b8
a8:
  br label %merge8
b8:
  br label %merge8
merge8:
  %d8 = phi i32 [ 0, %a8 ], [ 1, %b8 ]
  br i1 %cmp, label %special, label %split9

split9:
  %c9 = icmp ult i32 %i, 19
  br i1 %c9, label %a9, label %b9
a9:
  br label %merge9
b9:
  br label %merge9
merge9:
  %d9 = phi i32 [ 0, %a9 ], [ 1, %b9 ]
  br i1 %cmp, label %special, label %split10

split10:
  %c10 = icmp ult i32 %i, 20
  br i1 %c10, label %a10, label %b10
a10:
  br label %merge10
b10:
  br label %merge10
merge10:
  %d10 = phi i32 [ 0, %a10 ], [ 1, %b10 ]
  br i1 %cmp, label %special, label %split11

split11:
  %c11 = icmp ult i32 %i, 21
  br i1 %c11, label %a11, label %b11
a11:
  br label %merge11
b11:
  br label %merge11
merge11:
  %d11 = phi i32 [ 0, %a11 ], [ 1, %b11 ]
  br i1 %cmp, label %special, label %split12

split12:
  %c12 = icmp ult i32 %i, 22
  br i1 %c12, label %a12, label %b12
a12:
  br label %merge12
b12:
  br label %merge12
merge12:
  %d12 = phi i32 [ 0, %a12 ], [ 1, %b12 ]
  br i1 %cmp, label %special, label %split13

split13:
  %c13 = icmp ult i32 %i, 23
  br i1 %c13, label %a13, label %b13
a13:
  br label %merge13
b13:
  br label %merge13
merge13:
  %d13 = phi i32 [ 0, %a13 ], [ 1, %b13 ]
  br i1 %cmp, label %special, label %split14

split14:
  %c14 = icmp ult i32 %i, 24
  br i1 %c14, label %a14, label %b14
a14:
  br label %merge14
b14:
  br label %merge14
merge14:
  %d14 = phi i32 [ 0, %a14 ], [ 1, %b14 ]
  br i1 %cmp, label %special, label %latch

special:
  br label %latch

latch:
  %i.next = add i32 %i, 1
  %exit_cond = icmp eq i32 %i.next, %n
  br i1 %exit_cond, label %exit, label %header

exit:
  ret void
}

!igc.functions = !{}
