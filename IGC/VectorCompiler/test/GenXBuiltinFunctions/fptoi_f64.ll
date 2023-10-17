;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -vc-builtins-bif-path=%VC_BUILTINS_BIF_XeLPG% \
; RUN: -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeLPG -S < %s | FileCheck %s

; RUN: %opt %use_old_pass_manager% -vc-builtins-bif-path=%VC_BUILTINS_BIF_Gen9% \
; RUN: -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -S < %s | FileCheck %s --check-prefix=CHECK-NOEMU

; CHECK: @test_vector
; CHECK-NEXT: [[FP64_1:%[^ ]+]] = uitofp <16 x i32>    %r to <16 x double>
; CHECK-NEXT: [[IN1:%[^ ]+]]    = fptosi <16 x double> [[FP64_1]] to <16 x i32>
; CHECK-NEXT: [[FP64_2:%[^ ]+]] = sitofp <16 x i32>    [[IN1]] to <16 x double>
; CHECK-NEXT: [[IN2:%[^ ]+]]    = fptoui <16 x double> [[FP64_2]] to <16 x i32>
; CHECK-NEXT: [[FP64_3:%[^ ]+]] = call <16 x double> @__vc_builtin_uitofp_v16f64(<16 x i64> %r64)
; CHECK-NEXT: [[IN3:%[^ ]+]]    = call <16 x i64>    @__vc_builtin_fptosi_v16f64(<16 x double> [[FP64_3]])
; CHECK-NEXT: [[FP64_4:%[^ ]+]] = call <16 x double> @__vc_builtin_sitofp_v16f64(<16 x i64> [[IN3]])
; CHECK-NEXT: [[IN4:%[^ ]+]]    = call <16 x i64>    @__vc_builtin_fptoui_v16f64(<16 x double> [[FP64_4]])
; CHECK-NEXT: ret void

; CHECK-NOEMU: @test_vector
; CHECK-NOEMU-NEXT: [[FP64_1:%[^ ]+]] = uitofp <16 x i32>    %r to <16 x double>
; CHECK-NOEMU-NEXT: [[IN1:%[^ ]+]]    = fptosi <16 x double> [[FP64_1]] to <16 x i32>
; CHECK-NOEMU-NEXT: [[FP64_2:%[^ ]+]] = sitofp <16 x i32>    [[IN1]] to <16 x double>
; CHECK-NOEMU-NEXT: [[IN2:%[^ ]+]]    = fptoui <16 x double> [[FP64_2]] to <16 x i32>
; CHECK-NOEMU-NEXT: [[FP64_3:%[^ ]+]] = uitofp <16 x i64>    %r64 to <16 x double>
; CHECK-NOEMU-NEXT: [[IN3:%[^ ]+]]    = fptosi <16 x double> [[FP64_3]] to <16 x i64>
; CHECK-NOEMU-NEXT: [[FP64_4:%[^ ]+]] = sitofp <16 x i64>    [[IN3]] to <16 x double>
; CHECK-NOEMU-NEXT: [[IN4:%[^ ]+]]    = fptoui <16 x double> [[FP64_4]] to <16 x i64>
; CHECK-NOEMU-NEXT: ret void

define dllexport spir_kernel void @test_vector(<16 x i32> %r, <16 x i64> %r64) {
  %fp1 =  uitofp <16 x i32> %r to <16 x double>
  %in1 =  fptosi <16 x double> %fp1 to <16 x i32>
  %fp2 =  sitofp <16 x i32> %in1 to <16 x double>
  %in2 =  fptoui <16 x double> %fp2 to <16 x i32>

  %fp3 =  uitofp <16 x i64> %r64 to <16 x double>
  %in3 =  fptosi <16 x double> %fp3 to <16 x i64>
  %fp4 =  sitofp <16 x i64> %in3 to <16 x double>
  %in4 =  fptoui <16 x double> %fp4 to <16 x i64>

  ret void
}

; CHECK: @test_scalar
; CHECK-NEXT: [[FP64_1:%[^ ]+]] = uitofp i32   %r to double
; CHECK-NEXT: [[IN1:%[^ ]+]]    = fptosi double [[FP64_1]] to i32
; CHECK-NEXT: [[FP64_2:%[^ ]+]] = sitofp i32   [[IN1]] to double
; CHECK-NEXT: [[IN2:%[^ ]+]]    = fptoui double [[FP64_2]] to i32
; CHECK-NEXT: [[FP64_3:%[^ ]+]] = call double @__vc_builtin_uitofp_f64(i64 %r64)
; CHECK-NEXT: [[IN3:%[^ ]+]]    = call i64    @__vc_builtin_fptosi_f64(double [[FP64_3]])
; CHECK-NEXT: [[FP64_4:%[^ ]+]] = call double @__vc_builtin_sitofp_f64(i64 [[IN3]])
; CHECK-NEXT: [[IN4:%[^ ]+]]    = call i64    @__vc_builtin_fptoui_f64(double [[FP64_4]])
; CHECK-NEXT: ret void

; CHECK-NOEMU: @test_scalar
; CHECK-NOEMU-NEXT: [[FP64_1:%[^ ]+]] = uitofp i32    %r to double
; CHECK-NOEMU-NEXT: [[IN1:%[^ ]+]]    = fptosi double [[FP64_1]] to i32
; CHECK-NOEMU-NEXT: [[FP64_2:%[^ ]+]] = sitofp i32    [[IN1]] to double
; CHECK-NOEMU-NEXT: [[IN2:%[^ ]+]]    = fptoui double [[FP64_2]] to i32
; CHECK-NOEMU-NEXT: [[FP64_3:%[^ ]+]] = uitofp i64    %r64    to double
; CHECK-NOEMU-NEXT: [[IN3:%[^ ]+]]    = fptosi double [[FP64_3]] to i64
; CHECK-NOEMU-NEXT: [[FP64_4:%[^ ]+]] = sitofp i64    [[IN3]] to double
; CHECK-NOEMU-NEXT: [[IN4:%[^ ]+]]    = fptoui double [[FP64_4]] to i64
; CHECK-NOEMU-NEXT: ret void
define dllexport spir_kernel void @test_scalar(i32 %r, i64 %r64) {
  %fp1 =  uitofp i32 %r to double
  %in1 =  fptosi double %fp1 to i32
  %fp2 =  sitofp i32 %in1 to double
  %in2 =  fptoui double %fp2 to i32

  %fp3 =  uitofp i64 %r64 to double
  %in3 =  fptosi double %fp3 to i64
  %fp4 =  sitofp i64 %in3 to double
  %in4 =  fptoui double %fp4 to i64

  ret void
}
