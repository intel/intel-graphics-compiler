;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt --platformrlt --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------

; Check that result is not split into hi and lo if op is supported

define void @test_partial(i32 %src1) {
; CHECK-LABEL: @test_partial(
; CHECK-NEXT:    [[TMP1:%.*]] = sext i32 [[SRC1:%.*]] to i64
; CHECK-NEXT:    [[TMP2:%.*]] = ashr i64 [[TMP1]], 13
; CHECK-NEXT:    [[TMP3:%.*]] = bitcast i64 [[TMP2]] to <2 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <2 x i32> [[TMP3]], i32 0
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP3]], i32 1
; CHECK-NEXT:    [[TMP6:%.*]] = insertelement <2 x i32> undef, i32 [[TMP4]], i32 0
; CHECK-NEXT:    [[TMP7:%.*]] = insertelement <2 x i32> [[TMP6]], i32 [[TMP5]], i32 1
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <2 x i32> [[TMP7]] to i64
; CHECK-NEXT:    call void @use.i64(i64 [[TMP8]])
; CHECK-NEXT:    ret void
;
  %1 = sext i32 %src1 to i64
  %2 = ashr i64 %1, 13
  call void @use.i64(i64 %2)
  ret void
}

declare void @use.i64(i64)

!igc.functions = !{!0}

!0 = !{void (i32)* @test_partial, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
