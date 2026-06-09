;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s

; ------------------------------------------------
; WIAnalysis
; ------------------------------------------------

; This test checks whether the 'alloca' instruction is correctly identified as non-uniform when non-uniform
; value stored in it goes through a freeze instruction.

; CHECK: N: {{.*}} %a = alloca i16, align 2

; Function Attrs: convergent nounwind
define spir_kernel void @test_uniform(<8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i32 %bufferOffset) {
entry:
  %a = alloca i16
  %freezed_lid = freeze i16 %localIdX
  store i16 %freezed_lid, i16* %a, align 2
  ret void
}

!igc.functions = !{!358}

!358 = !{void (<8 x i32>, <8 x i32>, i32)* @test_uniform, !359}
!359 = !{!360}
!360 = !{!"function_type", i32 0}
!367 = !{!"argId", i32 0}
!368 = !{!"implicitArgInfoListVec[0]", !367}
!369 = !{!"argId", i32 1}
!370 = !{!"implicitArgInfoListVec[1]", !369}
!371 = !{!"argId", i32 8}
!372 = !{!"implicitArgInfoListVec[2]", !371}
!373 = !{!"argId", i32 15}
!374 = !{!"explicitArgNum", i32 0}
!375 = !{!"implicitArgInfoListVec[3]", !373, !374}
!376 = !{!"implicitArgInfoList", !368, !370, !372, !375}
!377 = !{!"FuncMDMap[0]", void (<8 x i32>, <8 x i32>, i32)* @test_uniform}
!378 = !{!"FuncMDValue[0]", !376}
!379 = !{!"FuncMD", !377, !378}
!380 = !{!"ModuleMD", !379}
!IGCMetadata = !{!380}
