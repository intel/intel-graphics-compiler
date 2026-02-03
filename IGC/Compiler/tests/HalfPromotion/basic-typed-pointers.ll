;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --half-promotion  -S < %s | FileCheck %s
; ------------------------------------------------
; HalfPromotion
; ------------------------------------------------

; Binary operators

; CHECK: define void @test_fadd
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = fadd float [[F_S1]], [[F_S2]]
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_fadd(half %s1, half %s2) {
  %1 = fadd half %s1, %s2
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_fsub
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = fsub float [[F_S1]], [[F_S2]]
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_fsub(half %s1, half %s2) {
  %1 = fsub half %s1, %s2
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_fmul
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = fmul float [[F_S1]], [[F_S2]]
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_fmul(half %s1, half %s2) {
  %1 = fmul half %s1, %s2
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_fdiv
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = fdiv float [[F_S1]], [[F_S2]]
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_fdiv(half %s1, half %s2) {
  %1 = fdiv half %s1, %s2
  call void @use_half(half %1)
  ret void
}


; CHECK: define void @test_uitofp
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = uitofp i32 %s1 to float
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_uitofp(i32 %s1) {
  %1 = uitofp i32 %s1 to half
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_sitofp
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = sitofp i32 %s1 to float
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_sitofp(i32 %s1) {
  %1 = sitofp i32 %s1 to half
  call void @use_half(half %1)
  ret void
}


; CHECK: define void @test_fptoui
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[I32_RES:%[0-9A-z]*]] = fptoui float [[F_S1]] to i32
; CHECK: call void @use_i32(i32 [[I32_RES]])

define void @test_fptoui(half %s1) {
  %1 = fptoui half %s1 to i32
  call void @use_i32(i32 %1)
  ret void
}

; CHECK: define void @test_fptosi
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[I32_RES:%[0-9A-z]*]] = fptosi float [[F_S1]] to i32
; CHECK: call void @use_i32(i32 [[I32_RES]])

define void @test_fptosi(half %s1) {
  %1 = fptosi half %s1 to i32
  call void @use_i32(i32 %1)
  ret void
}

; CHECK: define void @test_genx_waveall
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.genx.GenISA.WaveAll.f32(float [[F_S1]], i8 1, i1 true, i32 0)
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_genx_waveall(half %s1) {
  %1 = call half @llvm.genx.GenISA.WaveAll.f16(half %s1, i8 1, i1 true, i32 0)
  call void @use_half(half %1)
  ret void
}


; CHECK: define void @test_genx_waveprefix
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.genx.GenISA.WavePrefix.f32(float [[F_S1]], i8 1, i1 false, i1 true, i32 13)
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_genx_waveprefix(half %s1) {
  %1 = call half @llvm.genx.GenISA.WavePrefix.f16(half %s1, i8 1, i1 0, i1 1, i32 13)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_genx_waveclustered
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.genx.GenISA.WaveClustered.f32(float [[F_S1]], i8 1, i32 13, i32 14)
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_genx_waveclustered(half %s1) {
  %1 = call half @llvm.genx.GenISA.WaveClustered.f16(half %s1, i8 1, i32 13, i32 14)
  call void @use_half(half %1)
  ret void
}


; CHECK: define void @test_llvm_cos
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.cos.f32(float [[F_S1]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_cos(half %s1) {
  %1 = call half @llvm.cos.f16(half %s1)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_sin
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.sin.f32(float [[F_S1]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_sin(half %s1) {
  %1 = call half @llvm.sin.f16(half %s1)
  call void @use_half(half %1)
  ret void

}


; CHECK: define void @test_llvm_log2
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.log2.f32(float [[F_S1]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_log2(half %s1) {
  %1 = call half @llvm.log2.f16(half %s1)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_exp2
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.exp2.f32(float [[F_S1]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_exp2(half %s1) {
  %1 = call half @llvm.exp2.f16(half %s1)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_sqrt
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.sqrt.f32(float [[F_S1]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_sqrt(half %s1) {
  %1 = call half @llvm.sqrt.f16(half %s1)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_floor
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.floor.f32(float [[F_S1]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_floor(half %s1) {
  %1 = call half @llvm.floor.f16(half %s1)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_ceil
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.ceil.f32(float [[F_S1]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_ceil(half %s1) {
  %1 = call half @llvm.ceil.f16(half %s1)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_fabs
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.fabs.f32(float [[F_S1]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_fabs(half %s1) {
  %1 = call half @llvm.fabs.f16(half %s1)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_pow
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.pow.f32(float [[F_S1]], float [[F_S2]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_pow(half %s1, half %s2) {
  %1 = call half @llvm.pow.f16(half %s1, half %s2)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_fma
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[F_S3:%[0-9A-z]*]] = fpext half %s3 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.fma.f32(float [[F_S1]], float [[F_S2]], float [[F_S3]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_fma(half %s1, half %s2, half %s3) {
  %1 = call half @llvm.fma.f16(half %s1, half %s2, half %s3)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_maxnum
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.maxnum.f32(float [[F_S1]], float [[F_S2]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_maxnum(half %s1, half %s2) {
  %1 = call half @llvm.maxnum.f16(half %s1, half %s2)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_llvm_minnum
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = call float @llvm.minnum.f32(float [[F_S1]], float [[F_S2]])
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_llvm_minnum(half %s1, half %s2) {
  %1 = call half @llvm.minnum.f16(half %s1, half %s2)
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_cmp
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[I1_RES:%[0-9A-z]*]] = fcmp one float [[F_S1]], [[F_S2]]
; CHECK: call void @use_i1(i1 [[I1_RES]])

define void @test_cmp(half %s1, half %s2) {
  %1 = fcmp one half %s1, %s2
  call void @use_i1(i1 %1)
  ret void
}

; CHECK: define void @test_select
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = select i1 %cond, float [[F_S1]], float [[F_S2]]
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_select(i1 %cond, half %s1, half %s2) {
  %1 = select i1 %cond, half %s1, half %s2
  call void @use_half(half %1)
  ret void
}

; CHECK: define void @test_phi
; CHECK: entry:
; CHECK-NEXT: [[F_S1:%[0-9A-z]*]] = fpext half %s1 to float
; CHECK: bb1:
; CHECK-NEXT: [[F_S2:%[0-9A-z]*]] = fpext half %s2 to float
; CHECK: bb2:
; CHECK-NEXT: [[F_RES:%[0-9A-z]*]] = phi float [ [[F_S1]], %entry ], [ [[F_S2]], %bb1 ]
; CHECK-NEXT: [[H_RES:%[0-9A-z]*]] = fptrunc float [[F_RES]] to half
; CHECK: call void @use_half(half [[H_RES]])

define void @test_phi(i1 %cond, half %s1, half %s2) {
entry:
  br i1 %cond, label %bb1, label %bb2

bb1:
  br label %bb2

bb2:
  %0 = phi half [ %s1, %entry ], [ %s2, %bb1 ]
  call void @use_half(half %0)
  ret void
}

declare void @use_half(half)
declare void @use_i32(i32)
declare void @use_i1(i1)

declare half @llvm.cos.f16(half)
declare half @llvm.sin.f16(half)
declare half @llvm.log2.f16(half)
declare half @llvm.exp2.f16(half)
declare half @llvm.sqrt.f16(half)
declare half @llvm.floor.f16(half)
declare half @llvm.ceil.f16(half)
declare half @llvm.fabs.f16(half)
declare half @llvm.pow.f16(half, half)
declare half @llvm.fma.f16(half,half,half)
declare half @llvm.maxnum.f16(half, half)
declare half @llvm.minnum.f16(half, half)

declare half @llvm.genx.GenISA.WaveAll.f16(half, i8, i1, i32)
declare half @llvm.genx.GenISA.WaveClustered.f16(half, i8, i32, i32)
declare half @llvm.genx.GenISA.WavePrefix.f16(half, i8, i1, i1, i32)

