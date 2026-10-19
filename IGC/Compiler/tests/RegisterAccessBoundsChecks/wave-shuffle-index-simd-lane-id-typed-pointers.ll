;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the pass did not modify the shader due to the presence of a pattern
; RUN: igc_opt %s -S  --igc-register-access-bounds-checks | FileCheck %s
;

define i32 @f0(i32 %data, i32 %idx) #1 {
Label-0:
  %landId16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %landId32 = zext i16 %landId16 to i32
  %ret = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %data, i32 %landId32, i32 2)
  ret i32 %ret
}
; CHECK-LABEL: define i32 @f0
; CHECK:        [[IDW:%[A-z0-9]*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK-NEXT:   [[IDDW:%[A-z0-9]*]] = zext i16 [[IDW]] to i32
; CHECK-NEXT:   [[RET:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %data, i32 [[IDDW]], i32 2)
; CHECK-NEXT:   ret i32 [[RET]]

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()



