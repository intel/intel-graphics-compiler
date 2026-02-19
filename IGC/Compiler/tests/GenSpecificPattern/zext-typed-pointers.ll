;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-gen-specific-pattern -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: zext, trunc patterns
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_zext_i1(i32 %src1) {
; CHECK-LABEL: @test_zext_i1(
; CHECK:    [[TMP1:%[A-z0-9]*]] = icmp slt i32 [[SRC1:%[A-z0-9]*]], 2
; CHECK:    [[TMP2:%[A-z0-9]*]] = sext i1 [[TMP1]] to i32
; CHECK:    [[TMP3:%[A-z0-9]*]] = sub i32 0, [[TMP2]]
; CHECK:    call void @use.i32(i32 [[TMP3]])
; CHECK:    ret void
;
  %1 = icmp slt i32 %src1, 2
  %2 = zext i1 %1 to i32
  call void @use.i32(i32 %2)
  ret void
}

define spir_kernel void @test_zext_ptr(i32 %src1) {
; CHECK-LABEL: @test_zext_ptr(
; CHECK:    [[TMP1:%[A-z0-9]*]] = inttoptr i32 [[SRC1:%[A-z0-9]*]] to i64*
; CHECK:    call void @use.p64(i64* [[TMP1]])
; CHECK:    ret void
;
  %1 = zext i32 %src1 to i64
  %2 = inttoptr i64 %1 to i64*
  call void @use.p64(i64* %2)
  ret void
}

define spir_kernel void @test_trunc(i64 %src1) {
; CHECK-LABEL: @test_trunc(
; CHECK:    [[TMP1:%[A-z0-9]*]] = bitcast i64 [[SRC1:%[A-z0-9]*]] to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP3:%[A-z0-9]*]] = lshr i32 [[TMP2]], 17
; CHECK:    call void @use.i32(i32 [[TMP3]])
; CHECK:    ret void
;
  %1 = lshr i64 %src1, 49
  %2 = trunc i64 %1 to i32
  call void @use.i32(i32 %2)
  ret void
}

declare void @use.i32(i32)
declare void @use.p64(i64*)
