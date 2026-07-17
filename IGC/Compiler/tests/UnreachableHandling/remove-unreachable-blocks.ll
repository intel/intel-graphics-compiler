;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -igc-unreachable-handling -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; UnreachableHandling: drop blocks unreachable from the entry
; ------------------------------------------------

; Blocks unreachable from the entry (a dead predecessor chain) must be dropped so
; later passes and codegen only see reachable CFG.

; A dead cycle with no path from the entry is removed entirely.
define void @test_drop_unreachable_cycle() {
; CHECK-LABEL: @test_drop_unreachable_cycle(
; CHECK-NEXT:    ret void
; CHECK-NOT:     br
  ret void

dead:                                 ; preds = %unreach -> unreachable from entry
  br label %unreach

unreach:                              ; preds = %dead
  br label %dead
}

; A value defined in a reachable block but used only in an unreachable-from-entry
; block: the dead block (and its use) is removed, the reachable code is kept.
define void @test_drop_unreachable_use_block(i64 %v) {
; CHECK-LABEL: @test_drop_unreachable_use_block(
; CHECK-NEXT:    [[P:%.*]] = inttoptr i64 [[V:%.*]] to ptr addrspace(1)
; CHECK-NEXT:    ret void
; CHECK-NOT:     dead.exit
  %p = inttoptr i64 %v to ptr addrspace(1)
  ret void

dead.exit:                            ; No predecessors -> unreachable from entry
  br label %outflush

outflush:                             ; preds = %dead.exit
  store i64 %v, ptr addrspace(1) %p, align 8
  br label %dead.exit
}
