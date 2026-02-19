;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-genrotate -platformdg1 -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenRotate
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_rotate(i32 %src, i32* %dst) {
; CHECK-LABEL: @test_rotate(
; CHECK:    [[TMP1:%.*]] = shl i32 [[SRC:%.*]], 14
; CHECK:    [[TMP2:%.*]] = lshr i32 [[SRC]], 18
; CHECK:    [[ROTATE:%.*]] = call i32 @llvm.fshl.i32(i32 [[SRC]], i32 [[SRC]], i32 14)
; CHECK:    store i32 [[ROTATE]], i32* [[DST:%.*]]
; CHECK:    ret void
;
  %1 = shl i32 %src, 14
  %2 = lshr i32 %src, 18
  %3 = or i32 %1, %2
  store i32 %3, i32* %dst
  ret void
}
