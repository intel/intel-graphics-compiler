;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-promoteint8type -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define i8 @test_clustered_broadcast(i8 %src1, i32 %lane) {
; CHECK-LABEL: @test_clustered_broadcast(
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = call i16 @llvm.genx.GenISA.WaveClusteredBroadcast.i16(i16 [[B2S1]], i32 8, i32 %lane, i32 0)
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = call i8 @llvm.genx.GenISA.WaveClusteredBroadcast.i8(i8 %src1, i32 8, i32 %lane, i32 0)
  ret i8 %1
}

define i8 @test_clustered_scan_exclusive_add(i8 %src1) {
; CHECK-LABEL: @test_clustered_scan_exclusive_add(
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = call i16 @llvm.genx.GenISA.WaveClusteredPrefix.i16(i16 [[B2S1]], i8 0, i32 8, i32 0)
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = call i8 @llvm.genx.GenISA.WaveClusteredPrefix.i8(i8 %src1, i8 0, i32 8, i32 0)
  ret i8 %1
}

declare i8 @llvm.genx.GenISA.WaveClusteredBroadcast.i8(i8, i32, i32, i32)
declare i8 @llvm.genx.GenISA.WaveClusteredPrefix.i8(i8, i8, i32, i32)
