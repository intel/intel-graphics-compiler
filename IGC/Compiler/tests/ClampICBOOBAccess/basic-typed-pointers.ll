;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -ClampICBOOBAccess  -S < %s 2>&1 | FileCheck %s

; Test checks that ICB index is clamped

define void @test_clamp(i32 %src) {
; CHECK-LABEL: @test_clamp(
; CHECK:    [[TMP1:%.*]] = call float @llvm.genx.GenISA.RuntimeValue.f32(i32 16)
; CHECK:    [[TMP2:%.*]] = bitcast float [[TMP1]] to i32
; CHECK:    [[TMP3:%.*]] = inttoptr i32 [[TMP2]] to [3 x i32]*
; CHECK:    [[TMP4:%.*]] = icmp uge i32 [[SRC:%.*]], 3
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i32 0, i32 [[SRC]]
; CHECK:    [[TMP6:%.*]] = getelementptr [3 x i32], [3 x i32]* [[TMP3]], i32 0, i32 [[TMP5]]
; CHECK:    [[TMP7:%.*]] = load i32, i32* [[TMP6]]
; CHECK:    [[TMP8:%.*]] = add i32 [[TMP7]], [[SRC]]
; CHECK:    call void @use.i32(i32 [[TMP8]])
; CHECK:    ret void
;
  %1 = call float @llvm.genx.GenISA.RuntimeValue.f32(i32 16)
  %2 = bitcast float %1 to i32
  %3 = inttoptr i32 %2 to [3 x i32]*
  %4 = getelementptr [3 x i32], [3 x i32]* %3, i32 0, i32 %src
  %5 = load i32, i32* %4
  %6 = add i32 %5, %src
  call void @use.i32(i32 %6)
  ret void
}

declare void @use.i32(i32)
declare float @llvm.genx.GenISA.RuntimeValue.f32(i32)

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1}
!1 = !{!"pushInfo", !2}
!2 = !{!"inlineConstantBufferOffset", i32 16}
