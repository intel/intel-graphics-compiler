;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -IGCIndirectICBPropagaion -S < %s | FileCheck %s
; ------------------------------------------------
; IGCIndirectICBPropagaion
; ------------------------------------------------

define void @test_load_i32(i32 %src) {
; CHECK-LABEL: @test_load_i32(
; CHECK:    [[TMP1:%.*]] = lshr i32 %src, 2
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> <i32 67305985, i32 134678021>, i32 [[TMP1]]
; CHECK:    call void @use.i32(i32 [[TMP2]])
; CHECK:    ret void
;
  %1 = inttoptr i32 %src to i32 addrspace(65549)*
  %2 = load i32, i32 addrspace(65549)* %1, align 4
  call void @use.i32(i32 %2)
  ret void
}

define void @test_load_f32(i32 %src) {
; CHECK-LABEL: @test_load_f32(
; CHECK:    [[TMP1:%.*]] = lshr i32 %src, 2
; CHECK:    [[TMP2:%.*]] = extractelement <2 x float> <float 0x3880604020000000, float 0x3900E0C0A0000000>, i32 [[TMP1]]
; CHECK:    call void @use.f32(float [[TMP2]])
; CHECK:    ret void
;
  %1 = inttoptr i32 %src to float addrspace(65549)*
  %2 = load float, float addrspace(65549)* %1, align 4
  call void @use.f32(float %2)
  ret void
}

declare void @use.i32(i32)
declare void @use.f32(float)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1, !11}
!1 = !{!"immConstant", !2}
!2 = !{!"data", !3, !4, !5, !6, !7, !8, !9, !10}
!3 = !{!"dataVec[0]", i8 1}
!4 = !{!"dataVec[1]", i8 2}
!5 = !{!"dataVec[2]", i8 3}
!6 = !{!"dataVec[3]", i8 4}
!7 = !{!"dataVec[4]", i8 5}
!8 = !{!"dataVec[5]", i8 6}
!9 = !{!"dataVec[6]", i8 7}
!10 = !{!"dataVec[7]", i8 8}
!11 = !{!"pushInfo", !12, !13}
!12 = !{!"inlineConstantBufferOffset", i32 -1}
!13 = !{!"inlineConstantBufferSlot", i32 13}
