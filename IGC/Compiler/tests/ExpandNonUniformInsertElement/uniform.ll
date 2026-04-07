;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --regkey ExpandNonUniformInsertElementThreshold=4 --igc-wi-analysis --ExpandNonUniformInsertElement -S %s | FileCheck %s

; Demonstrate a vector of 4 x i32 with a uniform insertion index.

; CHECK: insertelement <4 x i32> %vec, i32 0, i32 %uniformIdx

define void @test_uniform(<4 x i32> %vec, i32 %idx) {
entry:
  %uniformIdx = call i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32 %idx, i32 0, i32 0)
  %result = insertelement <4 x i32> %vec, i32 0, i32 %uniformIdx
  ret void
}

declare i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32, i32, i32)

!igc.functions = !{!358}

!358 = !{void (<4 x i32>, i32)* @test_uniform, !359}
!359 = !{!360}
!360 = !{!"function_type", i32 2}