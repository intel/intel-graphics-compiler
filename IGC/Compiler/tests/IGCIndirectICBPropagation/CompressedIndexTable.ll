;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
; RUN: igc_opt  -regkey MinCompressionThreshold=30  --opaque-pointers --platformNvl -IGCIndirectICBPropagaion -S  < %s | FileCheck %s
; ------------------------------------------------
; IGCIndirectICBPropagaion
; ------------------------------------------------

define i32 @test_load_i32(i32 %src) {
; CHECK-LABEL: define i32 @test_load_i32
; CHECK:    inttoptr i32 %src to ptr addrspace(65549)
; CHECK:    [[IDX_BASE:%.*]] = lshr i32 %src, 2
; CHECK:    [[IDX_SHIFTED:%.*]] = shl i32 [[IDX_BASE]], 0
; CHECK:    [[BITFIELD:%.*]] = lshr i32 3, [[IDX_SHIFTED]]
; CHECK:    [[IDX:%.*]] = and i32 [[BITFIELD]], 1
; CHECK:    [[VAL:%.*]] = extractelement <2 x i32> <i32 0, i32 1065353216>, i32 [[IDX]]
; CHECK:    ret i32 [[VAL]]
  %1 = inttoptr i32 %src to i32 addrspace(65549)*
  %2 = load i32, i32 addrspace(65549)* %1, align 4
  ret i32 %2
}

define float @test_load_f32(i32 %src) {
; CHECK-LABEL: define float @test_load_f32
; CHECK:    inttoptr i32 %src to ptr addrspace(65549)
; CHECK:    [[IDX_BASE:%.*]] = lshr i32 %src, 2
; CHECK:    [[IDX_SHIFTED:%.*]] = shl i32 [[IDX_BASE]], 0
; CHECK:    [[BITFIELD:%.*]] = lshr i32 3, [[IDX_SHIFTED]]
; CHECK:    [[IDX:%.*]] = and i32 [[BITFIELD]], 1
; CHECK:    [[VAL:%.*]] = extractelement <2 x float> <float 0.000000e+00, float 1.000000e+00>, i32 [[IDX]]
; CHECK:    ret float [[VAL]]
  %1 = inttoptr i32 %src to float addrspace(65549)*
  %2 = load float, float addrspace(65549)* %1, align 4
  ret float %2
}

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1, !26}
!1 = !{!"immConstant", !2, !23}
!2 = !{!"data", !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22}
!3 = !{!"dataVec[0]", i8 0}
!4 = !{!"dataVec[1]", i8 0}
!5 = !{!"dataVec[2]", i8 -128}
!6 = !{!"dataVec[3]", i8 63}
!7 = !{!"dataVec[4]", i8 0}
!8 = !{!"dataVec[5]", i8 0}
!9 = !{!"dataVec[6]", i8 -128}
!10 = !{!"dataVec[7]", i8 63}
!11 = !{!"dataVec[8]", i8 0}
!12 = !{!"dataVec[9]", i8 0}
!13 = !{!"dataVec[10]", i8 0}
!14 = !{!"dataVec[11]", i8 0}
!15 = !{!"dataVec[12]", i8 0}
!16 = !{!"dataVec[13]", i8 0}
!17 = !{!"dataVec[14]", i8 0}
!18 = !{!"dataVec[15]", i8 0}
!19 = !{!"dataVec[16]", i8 0}
!20 = !{!"dataVec[17]", i8 0}
!21 = !{!"dataVec[18]", i8 0}
!22 = !{!"dataVec[19]", i8 0}
!23 = !{!"sizes", !24, !25}
!24 = !{!"sizesMap[0]", i32 0}
!25 = !{!"sizesValue[0]", i32 20}
!26 = !{!"pushInfo", !27, !28}
!27 = !{!"inlineConstantBufferOffset", i32 -1}
!28 = !{!"inlineConstantBufferSlot", i32 13}
