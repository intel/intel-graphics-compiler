;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-ldshrink -dce -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LdShrink
; ------------------------------------------------

;
; Debug-info related check
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 2
; CHECK: CheckModuleDebugify: PASS

define void @test(<16 x i32>* %src, i32* %dst) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = bitcast <16 x i32>* [[SRC:%.*]] to i32*
; CHECK:    [[TMP2:%.*]] = getelementptr inbounds i32, i32* [[TMP1]], i32 13
; CHECK:    [[TMP3:%.*]] = load i32, i32* [[TMP2]], align 4
; CHECK:    store i32 [[TMP3]], i32* [[DST:%.*]], align 4
; CHECK:    ret void
;
  %1 = load <16 x i32>, <16 x i32>* %src, align 64
  %2 = extractelement <16 x i32> %1, i32 13
  store i32 %2, i32* %dst, align 4
  ret void
}
