;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops : i64 ConstantExpr operands
; ------------------------------------------------

; The high half of a non-ptrtoint ConstantExpr is an lshr instruction. It is
; expanded once, in the entry block, which dominates every use.

@g = external addrspace(1) global i32

define void @test_same_block(i64 %x, ptr addrspace(1) %out) {
; CHECK-LABEL: @test_same_block(
; CHECK:         [[SH:%.*]] = lshr i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), 32
; CHECK-NEXT:    [[HI:%.*]] = trunc i64 [[SH]] to i32
; CHECK:         and i32 trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32),
; CHECK-NEXT:    and i32 [[HI]],
; CHECK-NEXT:    or i32 trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32),
; CHECK-NEXT:    or i32 [[HI]],
; CHECK-NOT:     lshr
; CHECK:         ret void
;
  %a = and i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %x
  %b = or i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %a
  store i64 %b, ptr addrspace(1) %out
  ret void
}

define void @test_dominated(i64 %x, i1 %c, ptr addrspace(1) %out) {
; CHECK-LABEL: @test_dominated(
; CHECK:       {{^}}entry:
; CHECK:         [[SH:%.*]] = lshr i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), 32
; CHECK-NEXT:    [[HI:%.*]] = trunc i64 [[SH]] to i32
; CHECK:         and i32 [[HI]],
; CHECK:       {{^}}a:
; CHECK-NEXT:    or i32 trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32),
; CHECK-NEXT:    or i32 [[HI]],
; CHECK-NOT:     lshr
; CHECK:         ret void
;
entry:
  %r0 = and i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %x
  store i64 %r0, ptr addrspace(1) %out
  br i1 %c, label %a, label %end
a:
  %r1 = or i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %x
  store i64 %r1, ptr addrspace(1) %out
  br label %end
end:
  ret void
}

define void @test_split_block(i64 %x, i64 %n, ptr addrspace(1) %out) {
; CHECK-LABEL: @test_split_block(
; CHECK:       {{^}}entry:
; CHECK:         [[SH:%.*]] = lshr i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), 32
; CHECK-NEXT:    [[HI:%.*]] = trunc i64 [[SH]] to i32
; CHECK:         and i32 [[HI]],
; CHECK:         %.shl.outer.merge.hi = phi i32
; CHECK-NEXT:    or i32 trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32), %.shl.outer.merge.lo
; CHECK-NEXT:    or i32 [[HI]], %.shl.outer.merge.hi
; CHECK-NOT:     lshr i64
; CHECK:         ret void
;
entry:
  %a = and i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %x
  %s = shl i64 %a, %n
  %b = or i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %s
  store i64 %b, ptr addrspace(1) %out
  ret void
}

define void @test_sibling(i64 %x, i1 %c, ptr addrspace(1) %out) {
; CHECK-LABEL: @test_sibling(
; CHECK:       {{^}}entry:
; CHECK:         [[SH:%.*]] = lshr i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), 32
; CHECK-NEXT:    [[HI:%.*]] = trunc i64 [[SH]] to i32
; CHECK:       {{^}}a:
; CHECK-NEXT:    and i32 trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32),
; CHECK-NEXT:    and i32 [[HI]],
; CHECK:       {{^}}b:
; CHECK-NEXT:    or i32 trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32),
; CHECK-NEXT:    or i32 [[HI]],
; CHECK-NOT:     lshr
; CHECK:       {{^}}end:
;
entry:
  br i1 %c, label %a, label %b
a:
  %r1 = and i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %x
  store i64 %r1, ptr addrspace(1) %out
  br label %end
b:
  %r2 = or i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %x
  store i64 %r2, ptr addrspace(1) %out
  br label %end
end:
  ret void
}

define void @test_phi(i1 %c, ptr addrspace(1) %out) {
; CHECK-LABEL: @test_phi(
; CHECK:       {{^}}entry:
; CHECK-DAG:     [[SH8:%.*]] = lshr i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), 32
; CHECK-DAG:     [[HI8:%.*]] = trunc i64 [[SH8]] to i32
; CHECK-DAG:     [[SH16:%.*]] = lshr i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 16), 32
; CHECK-DAG:     [[HI16:%.*]] = trunc i64 [[SH16]] to i32
; CHECK:         br i1 %c
; CHECK-NOT:     lshr
; CHECK:       {{^}}end:
; CHECK-NEXT:    phi i32 [ trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32), %a ], [ trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 16) to i32), %b ]
; CHECK-NEXT:    phi i32 [ [[HI8]], %a ], [ [[HI16]], %b ]
;
entry:
  br i1 %c, label %a, label %b
a:
  br label %end
b:
  br label %end
end:
  %p = phi i64 [ add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %a ], [ add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 16), %b ]
  store i64 %p, ptr addrspace(1) %out
  ret void
}

define void @test_phi_same_pred(i32 %s, ptr addrspace(1) %out) {
; CHECK-LABEL: @test_phi_same_pred(
; CHECK:       {{^}}entry:
; CHECK:         [[SH:%.*]] = lshr i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), 32
; CHECK-NEXT:    [[HI:%.*]] = trunc i64 [[SH]] to i32
; CHECK-NEXT:    switch i32 %s
; CHECK-NOT:     lshr
; CHECK:       {{^}}end:
; CHECK-NEXT:    phi i32 [ trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32), %entry ], [ trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32), %entry ], [ trunc (i64 add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8) to i32), %entry ]
; CHECK-NEXT:    phi i32 [ [[HI]], %entry ], [ [[HI]], %entry ], [ [[HI]], %entry ]
;
entry:
  switch i32 %s, label %end [
    i32 0, label %end
    i32 1, label %end
  ]
end:
  %p = phi i64 [ add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %entry ], [ add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %entry ], [ add (i64 ptrtoint (ptr addrspace(1) @g to i64), i64 8), %entry ]
  store i64 %p, ptr addrspace(1) %out
  ret void
}

!igc.functions = !{!0, !3, !4, !5, !6, !7}

!0 = !{ptr @test_sibling, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @test_phi, !1}
!4 = !{ptr @test_same_block, !1}
!5 = !{ptr @test_phi_same_pred, !1}
!6 = !{ptr @test_dominated, !1}
!7 = !{ptr @test_split_block, !1}
