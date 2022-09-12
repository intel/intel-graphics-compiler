;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify -igc-divrem-combine -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; IntDivRemCombine
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test(i32 %src1, i32 %src2) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%[A-z0-9]*]] = sdiv i32 [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = mul i32 [[TMP1]], [[SRC2]]
; CHECK:    [[TMP3:%[A-z0-9]*]] = sub i32 [[SRC1]], [[TMP2]]
; CHECK:    [[TMP4:%[A-z0-9]*]] = add i32 [[TMP3]], [[TMP3]]
; CHECK:    call void @use.i32(i32 [[TMP4]])
; CHECK:    call void @use.i32(i32 [[TMP1]])
; CHECK:    [[TMP5:%[A-z0-9]*]] = udiv i32 [[SRC1]], [[SRC2]]
; CHECK:    [[TMP6:%[A-z0-9]*]] = mul i32 [[TMP5]], [[SRC2]]
; CHECK:    [[TMP7:%[A-z0-9]*]] = sub i32 [[SRC1]], [[TMP6]]
; CHECK:    call void @use.i32(i32 [[TMP7]])
; CHECK:    ret void
;
  %1 = srem i32 %src1, %src2
  %2 = add i32 %1, %1
  call void @use.i32(i32 %2)
  %3 = sdiv i32 %src1, %src2
  call void @use.i32(i32 %3)
  %4 = udiv i32 %src1, %src2
  %5 = urem i32 %src1, %src2
  call void @use.i32(i32 %5)
  ret void
}

declare void @use.i32(i32)
