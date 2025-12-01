;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
; RUN: igc_opt -regkey MinCompressionThreshold=30  --opaque-pointers -IGCIndirectICBPropagaion -S  < %s | FileCheck %s
; ------------------------------------------------
; IGCIndirectICBPropagaion
; ------------------------------------------------

define i32 @test_load_i32(i32 %src) {
; CHECK-LABEL: @test_load_i32
; CHECK:       inttoptr i32 %src to ptr addrspace(65549)
; CHECK:       [[IDX_DIV4:%[0-9]+]] = lshr i32 %src, 2
; CHECK:       [[TABLE_IDX:%[0-9]+]] = lshr i32 [[IDX_DIV4]], 3
; CHECK:       [[TABLE_VAL:%[0-9]+]] = extractelement <2 x i32> <i32 1985229328, i32 1732584210>, i32 [[TABLE_IDX]]
; CHECK:       [[SUB_IDX:%[0-9]+]] = and i32 [[IDX_DIV4]], 7
; CHECK:       [[SHIFT_VAL:%[0-9]+]] = shl i32 [[SUB_IDX]], 2
; CHECK:       [[SHIFTED_TABLE_VAL:%[0-9]+]] = lshr i32 [[TABLE_VAL]], [[SHIFT_VAL]]
; CHECK:       [[FINAL_IDX:%[0-9]+]] = and i32 [[SHIFTED_TABLE_VAL]], 15
; CHECK:       [[VAL:%[0-9]+]] = extractelement <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>, i32 [[FINAL_IDX]]
; CHECK:       ret i32 [[VAL]]
  %1 = inttoptr i32 %src to i32 addrspace(65549)*
  %2 = load i32, i32 addrspace(65549)* %1, align 4
  ret i32 %2
}

define float @test_load_f32(i32 %src) {
; CHECK-LABEL: @test_load_f32
; CHECK:       inttoptr i32 %src to ptr addrspace(65549)
; CHECK:       [[IDX_DIV4:%.*]] = lshr i32 %src, 2
; CHECK:       [[TABLE_IDX:%.*]] = lshr i32 [[IDX_DIV4]], 3
; CHECK:       [[TABLE_VAL:%.*]] = extractelement <2 x i32> <i32 1985229328, i32 1732584210>, i32 [[TABLE_IDX]]
; CHECK:       [[SUB_IDX:%.*]] = and i32 [[IDX_DIV4]], 7
; CHECK:       [[SHIFT_VAL:%.*]] = shl i32 [[SUB_IDX]], 2
; CHECK:       [[SHIFTED_TABLE_VAL:%.*]] = lshr i32 [[TABLE_VAL]], [[SHIFT_VAL]]
; CHECK:       [[FINAL_IDX:%.*]] = and i32 [[SHIFTED_TABLE_VAL]], 15
; CHECK:       [[VAL:%.*]] = extractelement <8 x float> <float 0x36A0000000000000, float 0x36B0000000000000, float 0x36B8000000000000, float 0x36C0000000000000, float 0x36C4000000000000, float 0x36C8000000000000, float 0x36CC000000000000, float 0x36D0000000000000>, i32 [[FINAL_IDX]]
; CHECK:       ret float [[VAL]]
  %1 = inttoptr i32 %src to float addrspace(65549)*
  %2 = load float, float addrspace(65549)* %1, align 4
  ret float %2
}

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1, !131}
!1 = !{!"immConstant", !2, !128}
!2 = !{!"data", !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66}
!3 = !{!"dataVec[0]", i8 1}
!4 = !{!"dataVec[1]", i8 0}
!5 = !{!"dataVec[2]", i8 0}
!6 = !{!"dataVec[3]", i8 0}
!7 = !{!"dataVec[4]", i8 2}
!8 = !{!"dataVec[5]", i8 0}
!9 = !{!"dataVec[6]", i8 0}
!10 = !{!"dataVec[7]", i8 0}
!11 = !{!"dataVec[8]", i8 3}
!12 = !{!"dataVec[9]", i8 0}
!13 = !{!"dataVec[10]", i8 0}
!14 = !{!"dataVec[11]", i8 0}
!15 = !{!"dataVec[12]", i8 4}
!16 = !{!"dataVec[13]", i8 0}
!17 = !{!"dataVec[14]", i8 0}
!18 = !{!"dataVec[15]", i8 0}
!19 = !{!"dataVec[16]", i8 5}
!20 = !{!"dataVec[17]", i8 0}
!21 = !{!"dataVec[18]", i8 0}
!22 = !{!"dataVec[19]", i8 0}
!23 = !{!"dataVec[20]", i8 6}
!24 = !{!"dataVec[21]", i8 0}
!25 = !{!"dataVec[22]", i8 0}
!26 = !{!"dataVec[23]", i8 0}
!27 = !{!"dataVec[24]", i8 7}
!28 = !{!"dataVec[25]", i8 0}
!29 = !{!"dataVec[26]", i8 0}
!30 = !{!"dataVec[27]", i8 0}
!31 = !{!"dataVec[28]", i8 8}
!32 = !{!"dataVec[29]", i8 0}
!33 = !{!"dataVec[30]", i8 0}
!34 = !{!"dataVec[31]", i8 0}
!35 = !{!"dataVec[32]", i8 3}
!36 = !{!"dataVec[33]", i8 0}
!37 = !{!"dataVec[34]", i8 0}
!38 = !{!"dataVec[35]", i8 0}
!39 = !{!"dataVec[36]", i8 2}
!40 = !{!"dataVec[37]", i8 0}
!41 = !{!"dataVec[38]", i8 0}
!42 = !{!"dataVec[39]", i8 0}
!43 = !{!"dataVec[40]", i8 4}
!44 = !{!"dataVec[41]", i8 0}
!45 = !{!"dataVec[42]", i8 0}
!46 = !{!"dataVec[43]", i8 0}
!47 = !{!"dataVec[44]", i8 3}
!48 = !{!"dataVec[45]", i8 0}
!49 = !{!"dataVec[46]", i8 0}
!50 = !{!"dataVec[47]", i8 0}
!51 = !{!"dataVec[48]", i8 6}
!52 = !{!"dataVec[49]", i8 0}
!53 = !{!"dataVec[50]", i8 0}
!54 = !{!"dataVec[51]", i8 0}
!55 = !{!"dataVec[52]", i8 5}
!56 = !{!"dataVec[53]", i8 0}
!57 = !{!"dataVec[54]", i8 0}
!58 = !{!"dataVec[55]", i8 0}
!59 = !{!"dataVec[56]", i8 8}
!60 = !{!"dataVec[57]", i8 0}
!61 = !{!"dataVec[58]", i8 0}
!62 = !{!"dataVec[59]", i8 0}
!63 = !{!"dataVec[60]", i8 7}
!64 = !{!"dataVec[61]", i8 0}
!65 = !{!"dataVec[62]", i8 0}
!66 = !{!"dataVec[63]", i8 0}
!128 = !{!"sizes", !129, !130}
!129 = !{!"sizesMap[0]", i32 0}
!130 = !{!"sizesValue[0]", i32 64}
!131 = !{!"pushInfo", !132, !133}
!132 = !{!"inlineConstantBufferOffset", i32 -1}
!133 = !{!"inlineConstantBufferSlot", i32 13}
