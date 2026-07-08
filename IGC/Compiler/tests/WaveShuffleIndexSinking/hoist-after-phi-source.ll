;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-wave-shuffle-index-sinking -S < %s | FileCheck %s
; ------------------------------------------------
; WaveShuffleIndexSinking
;
; If shuffle's source is a PHI node defined in the basic block selected for
; hoisting, the moved (non-PHI) instruction must be placed after all PHI nodes
; of that block.

define i32 @test(i1 %p, i32 %a, i32 %b) {
entry:
  br i1 %p, label %bb.a, label %bb.b

bb.a:
  br label %merge

bb.b:
  br label %merge

; CHECK-LABEL: merge:
; CHECK-NEXT:   %src = phi i32 [ %a, %bb.a ], [ %b, %bb.b ]
; CHECK-NEXT:   %other = phi i32 [ 0, %bb.a ], [ 1, %bb.b ]
; CHECK-NEXT:   [[HOISTED:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %src, i32 0, i32 0)
; CHECK-NEXT:   %cond = icmp eq i32 %other, 0
; CHECK-NEXT:   br i1 %cond, label %bb.true, label %bb.false
merge:
  %src = phi i32 [ %a, %bb.a ], [ %b, %bb.b ]
  %other = phi i32 [ 0, %bb.a ], [ 1, %bb.b ]
  %cond = icmp eq i32 %other, 0
  br i1 %cond, label %bb.true, label %bb.false

; CHECK-LABEL: bb.true:
; CHECK-NEXT:    br label %bb.exit
bb.true:
  %bar = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %src, i32 0, i32 0)
  br label %bb.exit

; CHECK-LABEL: bb.false:
; CHECK-NEXT:    br label %bb.exit
bb.false:
  %foo = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %src, i32 0, i32 0)
  br label %bb.exit

; CHECK-LABEL: bb.exit:
; CHECK-NEXT:    [[RET:%.*]] = phi i32 [ [[HOISTED]], %bb.true ], [ [[HOISTED]], %bb.false ]
; CHECK-NEXT:    ret i32 [[RET]]
bb.exit:
  %ret = phi i32 [ %bar, %bb.true ], [ %foo, %bb.false ]
  ret i32 %ret
}

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
