;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PartialEmuI64Ops : i64 ConstantExpr operands
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

!igc.functions = !{!0, !3, !4}

!0 = !{ptr @test_sibling, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @test_same_block, !1}
!4 = !{ptr @test_dominated, !1}
