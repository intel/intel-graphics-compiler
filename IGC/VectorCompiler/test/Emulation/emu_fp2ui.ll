;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulationImport \
; RUN: -vc-emulation-bif-path=%VC_EMULATION_BIF_XeLP% \
; RUN: -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeLP -S < %s | FileCheck %s

; RUN: opt %use_old_pass_manager% -GenXEmulationImport \
; RUN: -vc-emulation-bif-path=%VC_EMULATION_BIF_Gen9% \
; RUN: -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -S < %s | FileCheck %s --check-prefix=CHECK_NOEMU

; CHECK: @test_vector
; CHECK-NEXT: [[FP1:%[^ ]+]] = uitofp <16 x i32>   %r to <16 x float>
; CHECK-NEXT: [[IN1:%[^ ]+]] = fptosi <16 x float> [[FP1]] to <16 x i32>
; CHECK-NEXT: [[FP2:%[^ ]+]] = sitofp <16 x i32>   [[IN1]] to <16 x float>
; CHECK-NEXT: [[IN2:%[^ ]+]] = fptoui <16 x float> [[FP2]] to <16 x i32>
; CHECK-NEXT: [[FP3:%[^ ]+]] = call <16 x float> @__cm_intrinsic_impl_ui2fp_16_(<16 x i64> %r64)
; CHECK-NEXT: [[IN3:%[^ ]+]] = call <16 x i64>   @__cm_intrinsic_impl_fp2si_16_(<16 x float> [[FP3]])
; CHECK-NEXT: [[FP4:%[^ ]+]] = call <16 x float> @__cm_intrinsic_impl_si2fp_16_(<16 x i64> [[IN3]])
; CHECK-NEXT: [[IN4:%[^ ]+]] = call <16 x i64>   @__cm_intrinsic_impl_fp2ui_16_(<16 x float> [[FP4]])
; CHECK-NEXT: ret void

; CHECK_NOEMU: @test_vector
; CHECK_NOEMU-NEXT: [[FP1:%[^ ]+]] = uitofp <16 x i32>   %r to <16 x float>
; CHECK_NOEMU-NEXT: [[IN1:%[^ ]+]] = fptosi <16 x float> [[FP1]] to <16 x i32>
; CHECK_NOEMU-NEXT: [[FP2:%[^ ]+]] = sitofp <16 x i32>   [[IN1]] to <16 x float>
; CHECK_NOEMU-NEXT: [[IN2:%[^ ]+]] = fptoui <16 x float> [[FP2]] to <16 x i32>
; CHECK_NOEMU-NEXT: [[FP3:%[^ ]+]] = uitofp <16 x i64>   %r64 to <16 x float>
; CHECK_NOEMU-NEXT: [[IN3:%[^ ]+]] = fptosi <16 x float> [[FP3]] to <16 x i64>
; CHECK_NOEMU-NEXT: [[FP4:%[^ ]+]] = sitofp <16 x i64>   [[IN3]] to <16 x float>
; CHECK_NOEMU-NEXT: [[IN4:%[^ ]+]] = fptoui <16 x float> [[FP4]] to <16 x i64>
; CHECK_NOEMU-NEXT: ret void

define dllexport spir_kernel void @test_vector(<16 x i32> %r, <16 x i64> %r64) {
  %fp1 =  uitofp <16 x i32> %r to <16 x float>
  %in1 =  fptosi <16 x float> %fp1 to <16 x i32>
  %fp2 =  sitofp <16 x i32> %in1 to <16 x float>
  %in2 =  fptoui <16 x float> %fp2 to <16 x i32>

  %fp3 =  uitofp <16 x i64> %r64 to <16 x float>
  %in3 =  fptosi <16 x float> %fp3 to <16 x i64>
  %fp4 =  sitofp <16 x i64> %in3 to <16 x float>
  %in4 =  fptoui <16 x float> %fp4 to <16 x i64>

  ret void
}

; CHECK: @test_scalar
; CHECK-NEXT: [[FP1:%[^ ]+]] = uitofp i32   %r to float
; CHECK-NEXT: [[IN1:%[^ ]+]] = fptosi float [[FP1]] to i32
; CHECK-NEXT: [[FP2:%[^ ]+]] = sitofp i32   [[IN1]] to float
; CHECK-NEXT: [[IN2:%[^ ]+]] = fptoui float [[FP2]] to i32
; CHECK-NEXT: [[FP3:%[^ ]+]] = call float @__cm_intrinsic_impl_ui2fp_1_base__(i64 %r64)
; CHECK-NEXT: [[IN3:%[^ ]+]] = call i64   @__cm_intrinsic_impl_fp2si_1_base__(float [[FP3]])
; CHECK-NEXT: [[FP4:%[^ ]+]] = call float @__cm_intrinsic_impl_si2fp_1_base__(i64 [[IN3]])
; CHECK-NEXT: [[IN4:%[^ ]+]] = call i64   @__cm_intrinsic_impl_fp2ui_1_base__(float [[FP4]])
; CHECK-NEXT: ret void

; CHECK_NOEMU: @test_scalar
; CHECK_NOEMU-NEXT: [[FP1:%[^ ]+]] = uitofp i32   %r to float
; CHECK_NOEMU-NEXT: [[IN1:%[^ ]+]] = fptosi float [[FP1]] to i32
; CHECK_NOEMU-NEXT: [[FP2:%[^ ]+]] = sitofp i32   [[IN1]] to float
; CHECK_NOEMU-NEXT: [[IN2:%[^ ]+]] = fptoui float [[FP2]] to i32
; CHECK_NOEMU-NEXT: [[FP3:%[^ ]+]] = uitofp i64   %r64    to float
; CHECK_NOEMU-NEXT: [[IN3:%[^ ]+]] = fptosi float [[FP3]] to i64
; CHECK_NOEMU-NEXT: [[FP4:%[^ ]+]] = sitofp i64   [[IN3]] to float
; CHECK_NOEMU-NEXT: [[IN4:%[^ ]+]] = fptoui float [[FP4]] to i64
; CHECK_NOEMU-NEXT: ret void
define dllexport spir_kernel void @test_scalar(i32 %r, i64 %r64) {
  %fp1 =  uitofp i32 %r to float
  %in1 =  fptosi float %fp1 to i32
  %fp2 =  sitofp i32 %in1 to float
  %in2 =  fptoui float %fp2 to i32

  %fp3 =  uitofp i64 %r64 to float
  %in3 =  fptosi float %fp3 to i64
  %fp4 =  sitofp i64 %in3 to float
  %in4 =  fptoui float %fp4 to i64

  ret void
}

