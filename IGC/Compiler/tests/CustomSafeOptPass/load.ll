;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: load
; ------------------------------------------------

; Transforms select of pointer to select
; of loaded result

define i32 @test_load_gep1(ptr %src, i1 %cc) {
; CHECK-LABEL: define i32 @test_load_gep1(
; CHECK-SAME: ptr [[SRC:%.*]], i1 [[CC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = getelementptr i32, ptr [[SRC]], i32 1
; CHECK:    [[TMP2:%.*]] = getelementptr i32, ptr [[SRC]], i32 13
; CHECK:    [[TMP3:%.*]] = load i32, ptr [[TMP1]], align 4
; CHECK:    [[TMP4:%.*]] = load i32, ptr [[TMP2]], align 4
; CHECK:    [[TMP5:%.*]] = select i1 [[CC]], i32 [[TMP3]], i32 [[TMP4]]
; CHECK:    ret i32 [[TMP5]]
;
  %1 = select i1 %cc, i32 1, i32 13
  %2 = getelementptr i32, ptr %src, i32 %1
  %3 = load i32, ptr %2, align 4
  ret i32 %3
}

define i32 @test_load_gep3(ptr %src, i1 %cc) {
; CHECK-LABEL: define i32 @test_load_gep3(
; CHECK-SAME: ptr [[SRC:%.*]], i1 [[CC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = getelementptr [40 x [4 x i32]], ptr [[SRC]], i32 0, i32 1, i32 3
; CHECK:    [[TMP2:%.*]] = getelementptr [40 x [4 x i32]], ptr [[SRC]], i32 0, i32 13, i32 3
; CHECK:    [[TMP3:%.*]] = load i32, ptr [[TMP1]], align 4
; CHECK:    [[TMP4:%.*]] = load i32, ptr [[TMP2]], align 4
; CHECK:    [[TMP5:%.*]] = select i1 [[CC]], i32 [[TMP3]], i32 [[TMP4]]
; CHECK:    ret i32 [[TMP5]]
;
  %1 = select i1 %cc, i32 1, i32 13
  %2 = getelementptr [40 x [4 x i32]], ptr %src, i32 0, i32 %1, i32 3
  %3 = load i32, ptr %2, align 4
  ret i32 %3
}

