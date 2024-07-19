;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info=true -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=XeLP -S < %s | FileCheck --match-full-lines %s
; ------------------------------------------------
; GenXBaling
; ------------------------------------------------

declare float @llvm.fabs.f32(float)
declare i32 @llvm.genx.absi.i32(i32)

define float @fneg_fabs(float %a) {
; Invalid case: abs(fneg(%a)) cannot be baled
; CHECK-LABEL: bales in function: fneg_fabs:
; CHECK: %1 = fneg float %a: negmod{{$}}
; CHECK: %2 = call float @llvm.fabs.f32(float %1): absmod{{$}}
  %1 = fneg float %a
  %2 = call float @llvm.fabs.f32(float %1)
  ret float %2
}

define float @fabs_fneg(float %a) {
; Valid case: fneg(abs(%a)) is supported by the hardware
; CHECK-LABEL: bales in function: fabs_fneg:
; CHECK: %1 = call float @llvm.fabs.f32(float %a): absmod{{$}}
; CHECK: %2 = fneg float %1: negmod 0
  %1 = call float @llvm.fabs.f32(float %a)
  %2 = fneg float %1
  ret float %2
}

define float @fsub_fabs(float %a) {
; Invalid case: abs(0.0f - %a) cannot be baled
; CHECK-LABEL: bales in function: fsub_fabs:
; CHECK: %1 = fsub float 0.000000e+00, %a: negmod{{$}}
; CHECK: %2 = call float @llvm.fabs.f32(float %1): absmod{{$}}
  %1 = fsub float 0.0, %a
  %2 = call float @llvm.fabs.f32(float %1)
  ret float %2
}

define float @fabs_fsub(float %a) {
; Valid case: 0.0f - abs(%a) is supported by the hardware
; CHECK-LABEL: bales in function: fabs_fsub:
; CHECK: %1 = call float @llvm.fabs.f32(float %a): absmod{{$}}
; CHECK: %2 = fsub float 0.000000e+00, %1: negmod 1{{$}}
  %1 = call float @llvm.fabs.f32(float %a)
  %2 = fsub float 0.0, %1
  ret float %2
}

define i32 @sub_abs(i32 %a) {
; Invalid case: abs(0.0f - %a) cannot be baled
; CHECK-LABEL: bales in function: sub_abs:
; CHECK: %1 = sub i32 0, %a: negmod{{$}}
; CHECK: %2 = call i32 @llvm.genx.absi.i32(i32 %1): absmod{{$}}
  %1 = sub i32 0, %a
  %2 = call i32 @llvm.genx.absi.i32(i32 %1)
  ret i32 %2
}

define i32 @abs_sub(i32 %a) {
; Valid case: 0.0f - abs(%a) is supported by the hardware
; CHECK-LABEL: bales in function: abs_sub:
; CHECK: %1 = call i32 @llvm.genx.absi.i32(i32 %a): absmod{{$}}
; CHECK: %2 = sub i32 0, %1: negmod 1{{$}}
  %1 = call i32 @llvm.genx.absi.i32(i32 %a)
  %2 = sub i32 0, %1
  ret i32 %2
}
