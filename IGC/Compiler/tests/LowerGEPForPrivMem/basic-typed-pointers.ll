;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------

; Debug-info related check
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 6
; CHECK: CheckModuleDebugify: PASS


define void @test(<4 x i32> %a, <4 x i32>* %b) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = alloca <4 x i32>
; CHECK:    [[TMP2:%.*]] = load <4 x i32>, <4 x i32>* [[TMP1]]
; CHECK:    [[TMP3:%.*]] = extractelement <4 x i32> [[A:%.*]], i32 0
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP2]], i32 [[TMP3]], i32 0
; CHECK:    [[TMP5:%.*]] = extractelement <4 x i32> [[A]], i32 1
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i32> [[TMP4]], i32 [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <4 x i32> [[A]], i32 2
; CHECK:    [[TMP8:%.*]] = insertelement <4 x i32> [[TMP6]], i32 [[TMP7]], i32 2
; CHECK:    [[TMP9:%.*]] = extractelement <4 x i32> [[A]], i32 3
; CHECK:    [[TMP10:%.*]] = insertelement <4 x i32> [[TMP8]], i32 [[TMP9]], i32 3
; CHECK:    store <4 x i32> [[TMP10]], <4 x i32>* [[TMP1]]
; CHECK:    [[TMP11:%.*]] = load <4 x i32>, <4 x i32>* [[TMP1]]
; CHECK:    [[TMP12:%.*]] = extractelement <4 x i32> [[TMP11]], i32 0
; CHECK:    [[TMP13:%.*]] = insertelement <4 x i32> poison, i32 [[TMP12]], i32 0
; CHECK:    [[TMP14:%.*]] = extractelement <4 x i32> [[TMP11]], i32 1
; CHECK:    [[TMP15:%.*]] = insertelement <4 x i32> [[TMP13]], i32 [[TMP14]], i32 1
; CHECK:    [[TMP16:%.*]] = extractelement <4 x i32> [[TMP11]], i32 2
; CHECK:    [[TMP17:%.*]] = insertelement <4 x i32> [[TMP15]], i32 [[TMP16]], i32 2
; CHECK:    [[TMP18:%.*]] = extractelement <4 x i32> [[TMP11]], i32 3
; CHECK:    [[TMP19:%.*]] = insertelement <4 x i32> [[TMP17]], i32 [[TMP18]], i32 3
; CHECK:    store <4 x i32> [[TMP19]], <4 x i32>* [[B:%.*]], align 16
; CHECK:    [[TMP20:%.*]] = alloca <4 x i32>
; CHECK:    [[TMP21:%.*]] = call <4 x i32> @llvm.genx.GenISA.vectorUniform.v4i32()
; CHECK:    store <4 x i32> [[TMP21]], <4 x i32>* [[TMP20]]
; CHECK:    [[TMP22:%.*]] = load <4 x i32>, <4 x i32>* [[TMP20]]
; CHECK:    [[TMP23:%.*]] = insertelement <4 x i32> [[TMP22]], i32 13, i32 3
; CHECK:    store <4 x i32> [[TMP23]], <4 x i32>* [[TMP20]]
; CHECK:    ret void
;
  %1 = alloca <4 x i32>, align 4, !uniform !5
  store <4 x i32> %a, <4 x i32>* %1, align 16
  %2 = load <4 x i32>, <4 x i32>* %1, align 16
  store <4 x i32> %2, <4 x i32>* %b, align 16
  %3 = alloca [4 x i32], align 4, !uniform !5
  %4 = getelementptr [4 x i32], [4 x i32]* %3, i32 0, i32 3
  store i32 13, i32* %4, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (<4 x i32>, <4 x i32>*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i32 8}
!5 = !{i1 true}
