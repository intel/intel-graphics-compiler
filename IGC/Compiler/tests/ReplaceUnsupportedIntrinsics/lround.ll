;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

define i32 @A(float) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[FCMP:%[a-zA-Z0-9]+]] = fcmp oge float %0, 0.000000e+00
; CHECK:  [[SEL:%[a-zA-Z0-9]+]] = select i1 [[FCMP]], double 5.000000e-01, double -5.000000e-01
; CHECK:  [[FPEXP:%[a-zA-Z0-9]+]] = fpext float %0 to double
; CHECK:  [[FADD:%[a-zA-Z0-9]+]] = fadd double [[FPEXP]], [[SEL]]
; CHECK:  [[FPTOSI:%[a-zA-Z0-9]+]] = fptosi double [[FADD]] to i32
; CHECK:  ret i32 [[FPTOSI]]
  %1 = call i32 @llvm.lround.i32.f32(float %0)
  ret i32 %1
}

define i64 @B(double) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[FCMP1:%[a-zA-Z0-9]+]] = fcmp oge double %0, 0.000000e+00
; CHECK:  [[SEL1:%[a-zA-Z0-9]+]] = select i1 [[FCMP1]], double 5.000000e-01, double -5.000000e-01
; CHECK:  [[FADD1:%[a-zA-Z0-9]+]] = fadd double %0, [[SEL1]]
; CHECK:  [[FPTOSI1:%[a-zA-Z0-9]+]] = fptosi double [[FADD1]] to i64
; CHECK:  ret i64 [[FPTOSI1]]
  %1 = call i64 @llvm.llround.i64.f64(double %0)
  ret i64 %1
}

declare i32 @llvm.lround.i32.f32(float)
declare i64 @llvm.llround.i64.f64(double)
