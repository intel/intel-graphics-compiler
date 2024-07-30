;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN:  igc_opt  -igc-custom-unsafe-opt-pass  -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare float @llvm.fma.f32(float, float, float)

define void @sample_test1(float %x, float %y) #0 {
entry:
  %0 = call fast float @llvm.fma.f32(float %x, float %y, float 0.000000e+00)
  ret void
}
; CHECK-LABEL: define void @sample_test1
; CHECK: entry:
; CHECK:  %0 = fmul fast float %y, %x

define void @sample_test2(float %x, float %y) #0 {
entry:
  %0 = call fast float @llvm.fma.f32(float %x, float %y, float 0.000000e-00)
  ret void
}
; CHECK-LABEL: define void @sample_test2
; CHECK: entry:
; CHECK:  %0 = fmul fast float %y, %x

define void @sample_test3(float %x, float %y) #0 {
entry:
  %0 = call fast float @llvm.fma.f32(float %x, float 2.000000e+00, float 0.000000e-00)
  ret void
}
; CHECK-LABEL: define void @sample_test3
; CHECK: entry:
; CHECK:  %0 = fmul fast float 2.000000e+00, %x

define void @sample_test4(float %x, float %y) #0 {
entry:
  %0 = call fast float @llvm.fma.f32(float 2.000000e+00, float %y, float 0.000000e-00)
  ret void
}
; CHECK-LABEL: define void @sample_test4
; CHECK: entry:
; CHECK:  %0 = fmul fast float %y, 2.000000e+00

define void @sample_test5(float %x, float %y) #0 {
entry:
  %0 = call fast float @llvm.fma.f32(float %x, float 0.000000e+00, float %y)
  %1 = fmul fast float %0, 5.000000e-01
  ret void
}
; CHECK-LABEL: define void @sample_test5
; CHECK: entry:
; CHECK:  %0 = fmul fast float %y, 5.000000e-01

define void @sample_test6(float %x, float %y) #0 {
entry:
  %0 = call fast float @llvm.fma.f32(float %x, float 0.000000e-00, float %y)
  %1 = fmul fast float %0, 5.000000e-01
  ret void
}
; CHECK-LABEL: define void @sample_test6
; CHECK: entry:
; CHECK:  %0 = fmul fast float %y, 5.000000e-01

define void @sample_test7(float %x, float %y) #0 {
entry:
  %0 = call fast float @llvm.fma.f32(float 0.000000e+00, float %x, float %y)
  %1 = fmul fast float %0, 5.000000e-01
  ret void
}
; CHECK-LABEL: define void @sample_test7
; CHECK: entry:
; CHECK:  %0 = fmul fast float %y, 5.000000e-01

define void @sample_test8(float %x, float %y) #0 {
entry:
  %0 = call fast float @llvm.fma.f32(float 0.000000e-00, float %x, float %y)
  %1 = fmul fast float %0, 5.000000e-01
  ret void
}
; CHECK-LABEL: define void @sample_test8
; CHECK: entry:
; CHECK:  %0 = fmul fast float %y, 5.000000e-01

