;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --enable-debugify --igc-runtimevalue-vector-extract-pass -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; RuntimeValueVectorExtractPass
; ------------------------------------------------

; Test checks that extractelement from vector RuntimeValue call
; is replaced by scalar RuntimeValue call.

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
;

define void @test_runtime_extract32() {
; CHECK-LABEL: @test_runtime_extract32(
; CHECK:    [[TMP2:%.*]] = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
; CHECK:    call void @use.i32(i32 [[TMP2]])
;
  %1 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 1)
  %2 = extractelement <4 x i32> %1, i32 3
  call void @use.i32(i32 %2)
  ret void
}

define void @test_runtime_extract64() {
; CHECK-LABEL: @test_runtime_extract64(
; CHECK:    [[TMP2:%.*]] = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 3)
; CHECK:    call void @use.i64(i64 [[TMP2]])
;
  %1 = call <4 x i64> @llvm.genx.GenISA.RuntimeValue.v4i64(i32 1)
  %2 = extractelement <4 x i64> %1, i32 1
  call void @use.i64(i64 %2)
  ret void
}

declare <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32)
declare <4 x i64> @llvm.genx.GenISA.RuntimeValue.v4i64(i32)
declare void @use.i32(i32)
declare void @use.i64(i64)
