;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-int-type-legalizer -S < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-n8:16:32"

define i32 @test_ctpop(i8 %a) {
; CHECK-LABEL: define i32 @test_ctpop
; CHECK-NOT:     {{i3[^0-9]}}
; CHECK:         [[AND:%.*]] = and i8 %a, 7
; CHECK:         [[CALL:%.*]] = call i8 @llvm.ctpop.i8(i8 [[AND]])
; CHECK:         [[RES:%.*]] = and i8 [[CALL]], 7
; CHECK:         [[EXT:%.*]] = zext i8 [[RES]] to i32
; CHECK:         ret i32 [[EXT]]
  %1 = trunc i8 %a to i3
  %2 = call i3 @llvm.ctpop.i3(i3 %1)
  %3 = zext i3 %2 to i32
  ret i32 %3
}

define i32 @test_umin(i8 %a) {
; CHECK-LABEL: define i32 @test_umin
; CHECK-NOT:     {{i3[^0-9]}}
; CHECK:         [[AND:%.*]] = and i8 %a, 7
; CHECK:         [[CALL:%.*]] = call i8 @llvm.umin.i8(i8 [[AND]], i8 5)
; CHECK:         [[RES:%.*]] = and i8 [[CALL]], 7
; CHECK:         zext i8 [[RES]] to i32
  %1 = trunc i8 %a to i3
  %2 = call i3 @llvm.umin.i3(i3 %1, i3 5)
  %3 = zext i3 %2 to i32
  ret i32 %3
}

define i32 @test_umax(i8 %a) {
; CHECK-LABEL: define i32 @test_umax
; CHECK-NOT:     {{i3[^0-9]}}
; CHECK:         [[AND:%.*]] = and i8 %a, 7
; CHECK:         [[CALL:%.*]] = call i8 @llvm.umax.i8(i8 [[AND]], i8 5)
; CHECK:         [[RES:%.*]] = and i8 [[CALL]], 7
; CHECK:         zext i8 [[RES]] to i32
  %1 = trunc i8 %a to i3
  %2 = call i3 @llvm.umax.i3(i3 %1, i3 5)
  %3 = zext i3 %2 to i32
  ret i32 %3
}

define i32 @test_smin(i8 %a) {
; CHECK-LABEL: define i32 @test_smin
; CHECK-NOT:     {{i3[^0-9]}}
; CHECK:         [[AND:%.*]] = and i8 %a, 7
; CHECK:         [[SHL:%.*]] = shl i8 [[AND]], 5
; CHECK:         [[SEXT:%.*]] = ashr i8 [[SHL]], 5
; CHECK:         [[CALL:%.*]] = call i8 @llvm.smin.i8(i8 [[SEXT]], i8 -3)
; CHECK:         [[RES:%.*]] = and i8 [[CALL]], 7
; CHECK:         zext i8 [[RES]] to i32
  %1 = trunc i8 %a to i3
  %2 = call i3 @llvm.smin.i3(i3 %1, i3 -3)
  %3 = zext i3 %2 to i32
  ret i32 %3
}

define i32 @test_smax(i8 %a) {
; CHECK-LABEL: define i32 @test_smax
; CHECK-NOT:     {{i3[^0-9]}}
; CHECK:         [[AND:%.*]] = and i8 %a, 7
; CHECK:         [[SHL:%.*]] = shl i8 [[AND]], 5
; CHECK:         [[SEXT:%.*]] = ashr i8 [[SHL]], 5
; CHECK:         [[CALL:%.*]] = call i8 @llvm.smax.i8(i8 [[SEXT]], i8 -3)
; CHECK:         [[RES:%.*]] = and i8 [[CALL]], 7
; CHECK:         zext i8 [[RES]] to i32
  %1 = trunc i8 %a to i3
  %2 = call i3 @llvm.smax.i3(i3 %1, i3 -3)
  %3 = zext i3 %2 to i32
  ret i32 %3
}

define i32 @test_ctpop_i12(i16 %a) {
; CHECK-LABEL: define i32 @test_ctpop_i12
; CHECK-NOT:     {{i12[^0-9]}}
; CHECK:         [[AND:%.*]] = and i16 %a, 4095
; CHECK:         [[CALL:%.*]] = call i16 @llvm.ctpop.i16(i16 [[AND]])
; CHECK:         [[RES:%.*]] = and i16 [[CALL]], 4095
; CHECK:         [[EXT:%.*]] = zext i16 [[RES]] to i32
; CHECK:         ret i32 [[EXT]]
  %1 = trunc i16 %a to i12
  %2 = call i12 @llvm.ctpop.i12(i12 %1)
  %3 = zext i12 %2 to i32
  ret i32 %3
}

declare i3 @llvm.ctpop.i3(i3)
declare i12 @llvm.ctpop.i12(i12)
declare i3 @llvm.umin.i3(i3, i3)
declare i3 @llvm.umax.i3(i3, i3)
declare i3 @llvm.smin.i3(i3, i3)
declare i3 @llvm.smax.i3(i3, i3)