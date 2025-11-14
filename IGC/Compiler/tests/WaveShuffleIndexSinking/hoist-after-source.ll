;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-wave-shuffle-index-sinking -S < %s | FileCheck %s
; ------------------------------------------------
; WaveShuffleIndexSinking
;
; If shuffle's source is defined in the same basic block as the one selected
; for hoisting, hoisted instruction should be placed after the source.

define void @_ZTS10sycl_subgrIJZ4mainE18KernelName_TovsKTkZ5checkIS0_lEvRN4sycl3_V15queueEmmE25KernelName_cNsJzXxSBQfEKYEE(i32 %a, i32 %b) {
; CHECK-LABEL: entry:
; CHECK-NEXT:   %c = add i32 %a, %b
; CHECK-NEXT:   call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %c, i32 0, i32 0)
; CHECK-NEXT:   %cond = icmp eq i32 %a, 0
; CHECK-NEXT:   br i1 %cond, label %bb.true, label %bb.false
entry:
  %c = add i32 %a, %b
  %cond = icmp eq i32 %a, 0
  br i1 %cond, label %bb.true, label %bb.false

; CHECK-LABEL: bb.true:
; CHECK-NEXT:    br label %bb.exit
bb.true:
  %bar = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %c, i32 0, i32 0)
  br label %bb.exit

; CHECK-LABEL: bb.false:
; CHECK-NEXT:    br label %bb.exit
bb.false:
  %foo = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %c, i32 0, i32 0)
  br label %bb.exit

; CHECK-LABEL: bb.exit:
; CHECK-NEXT:    ret void
bb.exit:
  ret void
}

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
