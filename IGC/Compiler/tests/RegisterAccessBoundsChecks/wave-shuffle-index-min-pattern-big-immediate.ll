;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the pass added runtime checks for potentially OOB subgroup shuffle
; RUN: igc_opt --opaque-pointers %s -S  --igc-register-access-bounds-checks | FileCheck %s
;

define i32 @f0(i32 %data, i32 %idx) #1 {
Label-0:
  %isInBound = icmp ule i32 %idx, 63
  %idxClamped = select i1 %isInBound, i32 %idx, i32 63
  %ret = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %data, i32 %idxClamped, i32 2)
  ret i32 %ret
}
; CHECK-LABEL: define i32 @f0
; CHECK:        [[IS_IN_BOUNDS:%[A-z0-9]*]] = icmp ule i32 %idx, 63
; CHECK-NEXT:   [[CLAMPED_INDEX:%[A-z0-9]*]] = select i1 [[IS_IN_BOUNDS]], i32 %idx, i32 63
; CHECK-NEXT:   [[SIMD_SIZE:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK-NEXT:   [[SIMD_MASK:%[A-z0-9]*]] = sub i32 [[SIMD_SIZE:%[A-z0-9]*]], 1
; CHECK-NEXT:   [[INDEX:%[A-z0-9]*]] = and i32 [[CLAMPED_INDEX]], [[SIMD_MASK]]
; CHECK-NEXT:   [[RET:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %data, i32 [[INDEX]], i32 2)
; CHECK-NEXT:   ret i32 [[RET]]
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)



