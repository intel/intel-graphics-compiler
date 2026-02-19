;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------

define float @test(i64 %b) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = alloca <12 x float>
; CHECK:    [[TMP2:%.*]] = trunc i64 %b to i32
; CHECK:    [[TMP3:%.*]] = add i32 0, [[TMP2]]
; CHECK:    [[TMP4:%.*]] = mul i32 [[TMP3]], 1
; CHECK:    [[TMP5:%.*]] = add i32 [[TMP4]], 0
; CHECK:    [[TMP6:%.*]] = add i32 -1, [[TMP5]]
; CHECK:    [[TMP7:%.*]] = load <12 x float>, <12 x float>* [[TMP1]]
; CHECK:    [[TMP8:%.*]] = extractelement <12 x float> [[TMP7]], i32 [[TMP6]]
; CHECK:    ret float [[TMP8]]

  %a = alloca [12 x float], align 4
  %1 = getelementptr [12 x float], [12 x float]* %a, i64 0, i64 %b
  %2 = getelementptr float, float* %1, i64 -1
  %3 = load float, float* %2, align 4
  ret float %3
}

!igc.functions = !{!0}

!0 = !{float (i64)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
