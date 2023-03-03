;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-gen-specific-pattern -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: select pattern optimization
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_select1(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_select1(
; CHECK:    [[TMP1:%[A-z0-9]*]] = icmp slt i64 [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = sext i1 [[TMP1]] to i64
; CHECK:    [[TMP3:%[A-z0-9]*]] = and i64 [[SRC2]], [[TMP2]]
; CHECK:    call void @use.i64(i64 [[TMP3]])
; CHECK:    ret void
;
  %1 = icmp slt i64 %src1, %src2
  %2 = select i1 %1, i64 %src2, i64 0
  call void @use.i64(i64 %2)
  ret void
}

define spir_kernel void @test_select2(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_select2(
; CHECK:    [[TMP1:%[A-z0-9]*]] = sitofp i64 [[SRC1:%[A-z0-9]*]] to float
; CHECK:    [[TMP2:%[A-z0-9]*]] = sitofp i64 [[SRC2:%[A-z0-9]*]] to float
; CHECK:    [[TMP3:%[A-z0-9]*]] = fcmp olt float [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%[A-z0-9]*]] = sext i1 [[TMP3]] to i32
; CHECK:    [[TMP5:%[A-z0-9]*]] = bitcast float [[TMP2]] to i32
; CHECK:    [[TMP6:%[A-z0-9]*]] = and i32 [[TMP4]], [[TMP5]]
; CHECK:    [[TMP7:%[A-z0-9]*]] = bitcast i32 [[TMP6]] to float
; CHECK:    call void @use.f32(float [[TMP7]])
; CHECK:    ret void
;
  %1 = sitofp i64 %src1 to float
  %2 = sitofp i64 %src2 to float
  %3 = fcmp olt float %1, %2
  %4 = select i1 %3, float %2, float 0.000000e+00
  call void @use.f32(float %4)
  ret void
}

define spir_kernel void @test_select3(float %src1, float %src2) {
; CHECK-LABEL: @test_select3(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fcmp olt float [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = sext i1 [[TMP1]] to i32
; CHECK:    [[TMP3:%[A-z0-9]*]] = and i32 [[TMP2]], 1077936128
; CHECK:    [[TMP4:%[A-z0-9]*]] = bitcast i32 [[TMP3]] to float
; CHECK:    call void @use.f32(float [[TMP4]])
; CHECK:    ret void
;
  %1 = fcmp olt float %src1, %src2
  %2 = select i1 %1, float 3.000000e+00, float 0.000000e+00
  call void @use.f32(float %2)
  ret void
}

define spir_kernel void @test_select4(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_select4(
; CHECK:    [[TMP1:%[A-z0-9]*]] = sdiv i64 [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = icmp sgt i64 [[TMP1]], 10
; CHECK:    [[TMP3:%[A-z0-9]*]] = select i1 [[TMP2]], i64 [[TMP1]], i64 10
; CHECK:    [[TMP4:%[A-z0-9]*]] = trunc i64 [[TMP3]] to i32
; CHECK:    call void @use.i32(i32 [[TMP4]])
; CHECK:    ret void
;
  %1 = sdiv i64 %src1, %src2
  %2 = trunc i64 %1 to i32
  %3 = icmp sgt i64 %1, 10
  %4 = select i1 %3, i32 %2, i32 10
  call void @use.i32(i32 %4)
  ret void
}

define spir_kernel void @test_select5(i64 %src1, i64 %src2, i1 %srcb) {
; CHECK-LABEL: @test_select5(
; CHECK:    [[TMP1:%[A-z0-9]*]] = icmp slt i64 [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = and i1 [[TMP1]], %srcb
; CHECK:    call void @use.i1(i1 [[TMP2]])
; CHECK:    ret void
;
  %1 = icmp slt i64 %src1, %src2
  %2 = select i1 %1, i1 %srcb, i1 false
  call void @use.i1(i1 %2)
  ret void
}

define spir_kernel void @test_select6(i64 %src1, i64 %src2, i1 %srcb) {
; CHECK-LABEL: @test_select6(
; CHECK:    [[TMP1:%[A-z0-9]*]] = icmp slt i64 [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = or i1 [[TMP1]], %srcb
; CHECK:    call void @use.i1(i1 [[TMP2]])
; CHECK:    ret void
;
  %1 = icmp slt i64 %src1, %src2
  %2 = select i1 %1, i1 true, i1 %srcb
  call void @use.i1(i1 %2)
  ret void
}

declare void @use.i1(i1)
declare void @use.i32(i32)
declare void @use.i64(i64)
declare void @use.f32(float)
