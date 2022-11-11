;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-lower-discard -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DiscardLowering
; ------------------------------------------------

; Test checks discard intrinsic lowering

define void @test_discardlow(i1 %a) {
; CHECK-LABEL: @test_discardlow(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = call i1 @llvm.genx.GenISA.InitDiscardMask()
; CHECK:    [[TMP1:%[A-z0-9]*]] = call i1 @llvm.genx.GenISA.GetPixelMask(i1 [[TMP0]])
; CHECK:    [[TMP2:%[A-z0-9]*]] = xor i1 [[TMP1]], true
; CHECK:    [[TMP3:%[A-z0-9]*]] = call i1 @llvm.genx.GenISA.UpdateDiscardMask(i1 [[TMP0]], i1 [[TMP2]])
; CHECK:    br i1 [[TMP3]], label [[DISCARDRET:%[A-z0-9]*]], label [[POSTDISCARD:%[A-z0-9]*]]
; CHECK:  postDiscard:
; CHECK:    br label [[DISCARDRET]]
; CHECK:  DiscardRet:
; CHECK:    ret void
;
entry:
  %0 = call i1 @llvm.genx.GenISA.IsHelperInvocation()
  call void @llvm.genx.GenISA.discard(i1 %0)
  ret void
}

declare void @llvm.genx.GenISA.discard(i1)

declare i1 @llvm.genx.GenISA.IsHelperInvocation()

!igc.functions = !{!0}

!0 = !{void (i1)* @test_discardlow, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
