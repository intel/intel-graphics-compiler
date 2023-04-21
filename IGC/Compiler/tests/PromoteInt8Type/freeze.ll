;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-promoteint8type -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type : Freeze
; ------------------------------------------------

; REQUIRES: llvm-10-plus

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; Please note that freeze instruction is only expected to be promoted to i16 type
; if its result is used by arithmetic operation.

define i8 @test_freeze(i8 %src1) {
; CHECK-LABEL: @test_freeze(
; CHECK-NOT: freeze i16
  %1 = freeze i8 %src1
  ret i8 %1
}

define i8 @test_freeze_used_by_arith_inst(i8 %src1) {
; CHECK-LABEL: @test_freeze_used_by_arith_inst(
; CHECK:    [[B2S:%.*]] = sext i8 %src1 to i16
; CHECK:    [[SHR:%.*]] = add i16 [[B2S]], 1
; CHECK:    [[FREEZE:%.*]] = freeze i16 [[SHR]]
; CHECK:    [[ADD:%.*]] = add i16 [[FREEZE]], 1
; CHECK:    [[TRUNC:%.*]] = trunc i16 [[ADD]] to i8
; CHECK:    ret i8 [[TRUNC]]
;
  %1 = add i8 %src1, 1
  %2 = freeze i8 %1
  %3 = add i8 %2, 1
  ret i8 %3
}

define i8 @test_freeze_not_used_by_arith_inst(i8 %src1) {
; CHECK-LABEL: @test_freeze_not_used_by_arith_inst(
; CHECK:    [[B2S:%.*]] = sext i8 %src1 to i16
; CHECK:    [[SHR:%.*]] = add i16 [[B2S]], 1
; CHECK:    [[TRUNC:%.*]] = trunc i16 [[SHR]] to i8
; CHECK:    [[FREEZE:%.*]] = freeze i8 [[TRUNC]]
; CHECK:    ret i8 [[FREEZE]]
;
  %1 = add i8 %src1, 1
  %2 = freeze i8 %1
  ret i8 %2
}
