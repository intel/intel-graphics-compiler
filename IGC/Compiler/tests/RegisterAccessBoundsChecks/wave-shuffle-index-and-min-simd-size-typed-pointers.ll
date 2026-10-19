;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the pass did not modify the shader due to the presence of the and pattern
; RUN: igc_opt %s -S  --igc-register-access-bounds-checks | FileCheck %s
;

define i32 @f0(i32 %data, i32 %idx) #1 {
Label-0:
  %idxClamped = and i32 %idx, 7
  %ret = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %data, i32 %idxClamped, i32 2)
  ret i32 %ret
}
; CHECK-LABEL: define i32 @f0
; CHECK:        [[CLAMPED_INDEX:%[A-z0-9]*]] = and i32 %idx, 7
; CHECK-NEXT:   [[RET:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %data, i32 [[CLAMPED_INDEX]], i32 2)
; CHECK-NEXT:   ret i32 [[RET]]
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare i32 @llvm.genx.GenISA.simdSize()



