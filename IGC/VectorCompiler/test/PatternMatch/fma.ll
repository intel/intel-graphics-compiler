;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true --fp-contract=fast  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true --fp-contract=off  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s |  FileCheck  --check-prefix=CHECK-NOFMA %s

; CHECK-LABEL: @test_mul_add_f32
; CHECK-NOFMA-LABEL: @test_mul_add_f32
define <16 x float> @test_mul_add_f32(<16 x float> %op0, <16 x float> %op1, <16 x float> %op2) {
; CHECK: [[FMA:%[^ ]+]] = call <16 x float> @llvm.fma.v16f32(<16 x float> %op0, <16 x float> %op1, <16 x float> %op2)
; CHECK-NOFMA: %mul = fmul <16 x float> %op0, %op1
; CHECK-NOFMA-NEXT: %add = fadd <16 x float> %mul, %op2
  %mul = fmul <16 x float> %op0, %op1
  %add = fadd <16 x float> %mul, %op2
  ret <16 x float> %add
}

; CHECK-LABEL: @test_mul_addzero_f32
; CHECK-NOFMA-LABEL: @test_mul_addzero_f32
define <16 x float> @test_mul_addzero_f32(<16 x float> %op0, <16 x float> %op1) {
; CHECK: %mul = fmul <16 x float> %op0, %op1
; CHECK-NEXT: %add = fadd <16 x float> %mul, zeroinitializer
; CHECK-NOFMA: %mul = fmul <16 x float> %op0, %op1
; CHECK-NOFMA-NEXT: %add = fadd <16 x float> %mul, zeroinitializer
  %mul = fmul <16 x float> %op0, %op1
  %add = fadd <16 x float> %mul, zeroinitializer
  ret <16 x float> %add
}

; CHECK-LABEL: @test_mul_radd_f32
; CHECK-NOFMA-LABEL: @test_mul_radd_f32
define <16 x float> @test_mul_radd_f32(<16 x float> %op0, <16 x float> %op1, <16 x float> %op2) {
; CHECK: [[FMA:%[^ ]+]] = call <16 x float> @llvm.fma.v16f32(<16 x float> %op0, <16 x float> %op1, <16 x float> %op2)
; CHECK-NOFMA: %mul = fmul <16 x float> %op0, %op1
; CHECK-NOFMA-NEXT: %add = fadd <16 x float> %op2, %mul
  %mul = fmul <16 x float> %op0, %op1
  %add = fadd <16 x float> %op2, %mul
  ret <16 x float> %add
}

; CHECK-LABEL: @test_mul_sub_f32
; CHECK-NOFMA-LABEL: @test_mul_sub_f32
define <16 x float> @test_mul_sub_f32(<16 x float> %op0, <16 x float> %op1, <16 x float> %op2) {
; CHECK: [[NEG:%[^ ]+]] = fneg <16 x float> %op2
; CHECK: [[FMA:%[^ ]+]] = call <16 x float> @llvm.fma.v16f32(<16 x float> %op0, <16 x float> %op1, <16 x float> [[NEG]])
; CHECK-NOFMA: %mul = fmul <16 x float> %op0, %op1
; CHECK-NOFMA-NEXT: %add = fsub <16 x float> %mul, %op2
  %mul = fmul <16 x float> %op0, %op1
  %add = fsub <16 x float> %mul, %op2
  ret <16 x float> %add
}

; CHECK-LABEL: @test_mul_rsub_f32
; CHECK-NOFMA-LABEL: @test_mul_rsub_f32
define <16 x float> @test_mul_rsub_f32(<16 x float> %op0, <16 x float> %op1, <16 x float> %op2) {
; CHECK: [[NEG:%[^ ]+]] = fneg <16 x float> %op1
; CHECK: [[FMA:%[^ ]+]] = call <16 x float> @llvm.fma.v16f32(<16 x float> %op0, <16 x float> [[NEG]], <16 x float> %op2)
; CHECK-NOFMA: %mul = fmul <16 x float> %op0, %op1
; CHECK-NOFMA-NEXT: %add = fsub <16 x float> %op2, %mul
  %mul = fmul <16 x float> %op0, %op1
  %add = fsub <16 x float> %op2, %mul
  ret <16 x float> %add
}

declare <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float>, i32, i32, i32, i16, i32)

; CHECK-LABEL: @test_mul_add_f32_indirect_op0
define <16 x float> @test_mul_add_f32_indirect_op0(<32 x float> %op0, i16 %idx, <16 x float> %op1, <16 x float> %op2) {
; CHECK: [[FMA:%[^ ]+]] = call <16 x float> @llvm.fma.v16f32(<16 x float> %rdregion, <16 x float> %op1, <16 x float> %op2)
  %rdregion = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 1, i32 1, i32 0, i16 %idx, i32 32)
  %mul = fmul <16 x float> %rdregion, %op1
  %add = fadd <16 x float> %op2, %mul
  ret <16 x float> %add
}

; CHECK-LABEL: @test_mul_add_f32_indirect_op0_op1
define <16 x float> @test_mul_add_f32_indirect_op0_op1(<32 x float> %op0, i16 %idx0, <32 x float> %op1, i16 %idx1, <16 x float> %op2) {
; CHECK: %mul = fmul <16 x float> %rdregion0, %rdregion1
; CHECK-NEXT: %add = fadd <16 x float> %mul, %op2
  %rdregion0 = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 1, i32 1, i32 0, i16 %idx0, i32 32)
  %rdregion1 = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op1, i32 1, i32 1, i32 0, i16 %idx1, i32 32)
  %mul = fmul <16 x float> %rdregion0, %rdregion1
  %add = fadd <16 x float> %mul, %op2
  ret <16 x float> %add
}

; CHECK-LABEL: @test_mulnonprofit_mul_add_f32
define <16 x float> @test_mulnonprofit_mul_add_f32(<32 x float> %op0, i16 %idx0, <32 x float> %op1, i16 %idx1, <16 x float> %op2) {
; CHECK: %mulnonprofit = fmul <16 x float> %rdregion0, %rdregion1
; CHECK-NEXT: [[FMA:%[^ ]+]] = call <16 x float> @llvm.fma.v16f32(<16 x float> %op2, <16 x float> %rdregion0, <16 x float> %mulnonprofit)
  %rdregion0 = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 1, i32 1, i32 0, i16 %idx0, i32 32)
  %rdregion1 = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op1, i32 1, i32 1, i32 0, i16 %idx1, i32 32)
  %mulnonprofit = fmul <16 x float> %rdregion0, %rdregion1
  %mul = fmul <16 x float> %op2, %rdregion0
  %add = fadd <16 x float> %mulnonprofit, %mul
  ret <16 x float> %add
}

declare <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float>, <16 x float>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @test_mul_add_f32_indirect_res
define <32 x float> @test_mul_add_f32_indirect_res(<32 x float> %dst, i16 %idx, <16 x float> %op0, <16 x float> %op1, <16 x float> %op2) {
; CHECK: [[FMA:%[^ ]+]] = call <16 x float> @llvm.fma.v16f32(<16 x float> %op0, <16 x float> %op1, <16 x float> %op2)
  %mul = fmul <16 x float> %op0, %op1
  %add = fadd <16 x float> %op2, %mul
  %res = call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> %dst, <16 x float> %add, i32 1, i32 1, i32 0, i16 %idx, i32 32, i1 true)
  ret <32 x float> %res
}

; CHECK-LABEL: @test_mul_add_f32_indirect_op0_res
define <32 x float> @test_mul_add_f32_indirect_op0_res(<32 x float> %dst, i16 %idx, <32 x float> %op0, i16 %idx0, <16 x float> %op1, <16 x float> %op2) {
; CHECK: %mul = fmul <16 x float> %rdregion0, %op1
; CHECK-NEXT: %add = fadd <16 x float> %mul, %op2
  %rdregion0 = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 1, i32 1, i32 0, i16 %idx0, i32 32)
  %mul = fmul <16 x float> %rdregion0, %op1
  %add = fadd <16 x float> %mul, %op2
  %res = call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> %dst, <16 x float> %add, i32 1, i32 1, i32 0, i16 %idx, i32 32, i1 true)
  ret <32 x float> %res
}
