;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-const-prop -S < %s | FileCheck %s
; ------------------------------------------------
; IGCConstProp
; ------------------------------------------------

define spir_kernel void @test_igcconst(i32 %a, i32* %b, i1 %c, <2 x i32>* %d) {
; CHECK-LABEL: @test_igcconst(
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> <i32 12, i32 13, i32 13, i32 42>, i32 [[A:%.*]]
; CHECK:    [[TMP2:%.*]] = select i1 false, i32 0, i32 [[A]]
; CHECK:    store i32 [[TMP2]], i32* [[B:%.*]], align 4
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i32> <i32 12, i32 13, i32 13, i32 42>, i32 55, i32 3
; CHECK:    store i32 55, i32* [[B]], align 4
; CHECK:    [[TMP4:%.*]] = select i1 [[C:%.*]], <4 x i32> <i32 12, i32 13, i32 13, i32 42>, <4 x i32> <i32 21, i32 31, i32 32, i32 42>
; CHECK:    store i32 42, i32* [[B]], align 4

; replaceShaderConstant checks
; CHECK:    [[TMP5:%.*]] = inttoptr i32 42 to i32 addrspace(65549)*
; CHECK:    store i32 0, i32* [[B]], align 4
; CHECK:    [[TMP6:%.*]] = inttoptr i32 4 to i32 addrspace(65549)*
; CHECK:    store i32 134678021, i32* [[B]], align 4
; CHECK:    [[TMP7:%.*]] = inttoptr i32 0 to <2 x i32> addrspace(65549)*
; CHECK:    store <2 x i32> <i32 67305985, i32 134678021>, <2 x i32>* [[D:%.*]], align 4
; CHECK:    [[TMP8:%.*]] = inttoptr i32 8 to <2 x i32> addrspace(65549)*
; CHECK:    store <2 x i32> zeroinitializer, <2 x i32>* [[D]], align 4
; CHECK:    ret void
;
  %1 = extractelement <4 x i32> <i32 12, i32 13, i32 13, i32 42>, i32 %a
  %2 = icmp eq i32 %1, 0
  %3 = select i1 %2, i32 0, i32 %a
  store i32 %3, i32* %b, align 4
  %4 = insertelement <4 x i32> <i32 12, i32 13, i32 13, i32 42>, i32 55, i32 3
  %5 = extractelement <4 x i32> %4, i32 3
  store i32 %5, i32* %b, align 4
  %6 = select i1 %c, <4 x i32> <i32 12, i32 13, i32 13, i32 42>, <4 x i32> <i32 21, i32 31, i32 32, i32 42>
  %7 = extractelement <4 x i32> %6, i32 3
  store i32 %7, i32* %b, align 4

; replaceShaderConstant IR
  %8 = inttoptr i32 42 to i32 addrspace(65549)*
  %9 = load i32, i32 addrspace(65549)* %8, align 4
  store i32 %9, i32* %b, align 4
  %10 = inttoptr i32 4 to i32 addrspace(65549)*
  %11 = load i32, i32 addrspace(65549)* %10, align 4
  store i32 %11, i32* %b, align 4
  %12 = inttoptr i32 0 to <2 x i32> addrspace(65549)*
  %13 = load <2 x i32>, <2 x i32> addrspace(65549)* %12, align 4
  store <2 x i32> %13, <2 x i32>* %d, align 4
  %14 = inttoptr i32 8 to <2 x i32> addrspace(65549)*
  %15 = load <2 x i32>, <2 x i32> addrspace(65549)* %14, align 4
  store <2 x i32> %15, <2 x i32>* %d, align 4
  ret void
}

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
