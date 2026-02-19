;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-vectorprocess -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; VectorProcess : load
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_vectorpro(<2 x i16>** %src1, <2 x i16*>* %src2) {
; CHECK-LABEL: @test_vectorpro(
; CHECK:    [[VPTRCAST:%.*]] = bitcast <2 x i16>** %src1 to <2 x i32>*
; CHECK:    [[VCASTLOAD:%.*]] = load <2 x i32>, <2 x i32>* [[VPTRCAST]], align 4
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i32> [[VCASTLOAD]] to i64
; CHECK:    [[TMP2:%.*]] = inttoptr i64 [[TMP1]] to <2 x i16>*
; CHECK:    [[VPTRCAST1:%.*]] = bitcast <2 x i16*>* %src2 to <4 x i32>*
; CHECK:    [[VCASTLOAD2:%.*]] = load <4 x i32>, <4 x i32>* [[VPTRCAST1]], align 1
; CHECK:    [[TMP3:%.*]] = bitcast <4 x i32> [[VCASTLOAD2]] to <2 x i64>
; CHECK:    [[TMP4:%.*]] = extractelement <2 x i64> [[TMP3]], i64 0
; CHECK:    [[TMP5:%.*]] = inttoptr i64 [[TMP4]] to i16*
; CHECK:    [[TMP6:%.*]] = insertelement <2 x i16*> undef, i16* [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <2 x i64> [[TMP3]], i64 1
; CHECK:    [[TMP8:%.*]] = inttoptr i64 [[TMP7]] to i16*
; CHECK:    [[TMP9:%.*]] = insertelement <2 x i16*> [[TMP6]], i16* [[TMP8]], i64 1
; CHECK:    [[INTTOPTR2:%.*]] = inttoptr i64 [[TMP1]] to i32*
; CHECK:    [[VCASTLOAD3:%.*]] = load i32, i32* [[INTTOPTR2]], align 2
; CHECK:    [[TMP10:%.*]] = bitcast i32 [[VCASTLOAD3]] to <2 x i16>
; CHECK:    [[TMP11:%.*]] = extractelement <2 x i16> [[TMP10]], i32 1
; CHECK:    [[TMP12:%.*]] = extractelement <2 x i16*> [[TMP9]], i32 0
; CHECK:    store i16 [[TMP11]], i16* [[TMP12]], align 2
; CHECK:    ret void
;
  %1 = load <2 x i16>*, <2 x i16>** %src1, align 4
  %2 = load <2 x i16*>, <2 x i16*>* %src2, align 1
  %3 = load <2 x i16>, <2 x i16>* %1, align 2
  %4 = extractelement <2 x i16> %3, i32 1
  %5 = extractelement <2 x i16*> %2, i32 0
  store i16 %4, i16* %5, align 2
  ret void
}
