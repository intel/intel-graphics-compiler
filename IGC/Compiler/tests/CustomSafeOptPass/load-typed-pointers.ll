;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: load
; ------------------------------------------------

; Transforms select of pointer to select
; of loaded result

define i32 @test_load_gep1(i32* %src, i1 %cc) {
; CHECK-LABEL: define i32 @test_load_gep1(
; CHECK-SAME: i32* [[SRC:%.*]], i1 [[CC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = getelementptr i32, i32* [[SRC]], i32 1
; CHECK:    [[TMP2:%.*]] = getelementptr i32, i32* [[SRC]], i32 13
; CHECK:    [[TMP3:%.*]] = load i32, i32* [[TMP1]]
; CHECK:    [[TMP4:%.*]] = load i32, i32* [[TMP2]]
; CHECK:    [[TMP5:%.*]] = select i1 [[CC]], i32 [[TMP3]], i32 [[TMP4]]
; CHECK:    ret i32 [[TMP5]]
;
  %1 = select i1 %cc, i32 1, i32 13
  %2 = getelementptr i32, i32* %src, i32 %1
  %3 = load i32, i32* %2
  ret i32 %3
}

define i32 @test_load_gep3([40 x [4 x i32]]* %src, i1 %cc) {
; CHECK-LABEL: define i32 @test_load_gep3(
; CHECK-SAME: [40 x [4 x i32]]* [[SRC:%.*]], i1 [[CC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = getelementptr [40 x [4 x i32]], [40 x [4 x i32]]* [[SRC]], i32 0, i32 1, i32 3
; CHECK:    [[TMP2:%.*]] = getelementptr [40 x [4 x i32]], [40 x [4 x i32]]* [[SRC]], i32 0, i32 13, i32 3
; CHECK:    [[TMP3:%.*]] = load i32, i32* [[TMP1]]
; CHECK:    [[TMP4:%.*]] = load i32, i32* [[TMP2]]
; CHECK:    [[TMP5:%.*]] = select i1 [[CC]], i32 [[TMP3]], i32 [[TMP4]]
; CHECK:    ret i32 [[TMP5]]
;
  %1 = select i1 %cc, i32 1, i32 13
  %2 = getelementptr [40 x [4 x i32]], [40 x [4 x i32]]* %src, i32 0, i32 %1, i32 3
  %3 = load i32, i32* %2
  ret i32 %3
}
