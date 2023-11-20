;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_mul
; CHECK: (<16 x i32> [[HEX:%[A-Za-z0-9_.]+]], <8 x i32> [[OCT:%[A-Za-z0-9_.]+]], i32 [[SCL:%[A-Za-z0-9_.]+]], <16 x i1> [[FLAG:%[A-Za-z0-9_.]+]], <8 x float> [[FP:%[A-Za-z0-9_.]+]], <16 x float> [[FP_BIG:%[A-Za-z0-9_.]+]])
; CHECK-NEXT: [[MUL_HEX:%[A-Za-z0-9_.]+]] = mul <16 x i32> [[HEX]], [[HEX]]
; CHECK-NEXT: [[MUL_OCT:%[A-Za-z0-9_.]+]] = mul <8 x i32> [[OCT]], [[OCT]]
; CHECK-NEXT: [[MUL_SCALAR:%[A-Za-z0-9_.]+]] = mul i32 [[SCL]], [[SCL]]
; CHECK-NEXT: [[MUL_FP:%[A-Za-z0-9_.]+]] = fmul <8 x float> [[FP]], [[FP]]

; CHECK-LABEL: case1:
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.i1(<16 x i32> [[HEX]], <8 x i32> [[MUL_OCT]], i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16.i1(<16 x i32> [[HEX]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: call <8 x i32> @llvm.genx.uuadd.sat.v8i32.v8i32(<8 x i32> [[MUL_OCT]], <8 x i32> [[OCT]])
; CHECK-NEXT: call <8 x i32> @llvm.genx.suadd.sat.v8i32.v8i32(<8 x i32> [[MUL_OCT]], <8 x i32> [[OCT]])
; CHECK-NEXT: call <8 x i32> @llvm.genx.usadd.sat.v8i32.v8i32(<8 x i32> [[MUL_OCT]], <8 x i32> [[OCT]])
; CHECK-NEXT: call <8 x i32> @llvm.genx.ssadd.sat.v8i32.v8i32(<8 x i32> [[MUL_OCT]], <8 x i32> [[OCT]])
; CHECK-NEXT: [[ADD:%[A-Za-z0-9_.]+]] = add <8 x i32> [[MUL_OCT]], <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
; CHECK-NEXT: tail call <8 x i32> @llvm.genx.wrregionf.v8i32.v8i32.v8i32.i16.i1(<8 x i32> [[ADD]], <8 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

; CHECK-LABEL: case2:
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> [[HEX]], i32 [[MUL_SCALAR]], i32 16, i32 0, i32 0, i16 0, i32 0, i1 true)
; CHECK-NEXT: [[WRREG2:%[A-Za-z0-9_.]+]] = tail call <1 x i32> @llvm.genx.wrregionf.v1i32.v1i32.v16i32.i16.i1(<1 x i32> zeroinitializer, <16 x i32> [[HEX]], i32 1, i32 1, i32 1, i16 128, i32 0, i1 true)
; CHECK-NEXT: tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v1i32.i16.i1(<1 x i32> [[WRREG2]], i32 0, i32 0, i32 1, i16 0, i32 undef, i1 true)

; CHECK-LABEL: case3:
; CHECK-NEXT: tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.v8f32.i16.i1(<16 x float> [[FP_BIG]], <8 x float> [[MUL_FP]], i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: tail call <8 x float> @llvm.genx.rdregioni.v8f32.v16f32.i16.i1(<16 x float> [[FP_BIG]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[FMA1:%[A-Za-z0-9_.]+]] = call <8 x float> @llvm.fma.v8f32(<8 x float> [[FP]], <8 x float> [[FP]], <8 x float> [[FP]])
; CHECK-NEXT: [[FNEG:%[A-Za-z0-9_.]+]] = fneg <8 x float> [[FMA1]]
; CHECK-NEXT: [[FMA2:%[A-Za-z0-9_.]+]] = call <8 x float> @llvm.fma.v8f32(<8 x float> [[FP]], <8 x float> [[FP]], <8 x float> [[FNEG]])
; CHECK-NEXT: %out3 = tail call <8 x float> @llvm.genx.wrregionf.v8f32.v8f32.v8f32.i16.i1(<8 x float> [[FMA2]], <8 x float> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

; CHECK-LABEL: case4:
; CHECK-NEXT: [[WRREG4:%[A-Za-z0-9_.]+]] = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.i1(<16 x i32> [[HEX]], <8 x i32> [[MUL_OCT]], i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: [[RDREG4:%[A-Za-z0-9_.]+]] = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16.i1(<16 x i32> [[WRREG4]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: call <16 x i32> @llvm.genx.uuadd.sat.v16i32.v16i32(<16 x i32> [[RDREG4]], <16 x i32> [[HEX]])

; CHECK-LABEL: case5:
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16.i1(<16 x i32> [[MUL_HEX]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)

; CHECK-LABEL: case6:
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.i1(<16 x i32> [[MUL_HEX]], <8 x i32> zeroinitializer, i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)

; CHECK-LABEL: case7:
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.v16i1(<16 x i32> [[HEX]], <8 x i32> [[MUL_OCT]], i32 1, i32 1, i32 1, i16 0, i32 0, <16 x i1> %flag)

; CHECK-LABEL: case8:
; CHECK-NEXT: [[WRREG8:%[A-Za-z0-9_.]+]] = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> [[HEX]], i32 [[MUL_SCALAR]], i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> [[WRREG8]], i32 0, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

; CHECK-LABEL: case9:
; CHECK-NEXT: [[WRREG9:%[A-Za-z0-9_.]+]] = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> [[HEX]], i32 [[MUL_SCALAR]], i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> [[WRREG9]], i32 [[SCL]], i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

; CHECK-LABEL: case10:
; CHECK-NEXT: [[WRREG10:%[A-Za-z0-9_.]+]] = tail call <8 x i32> @llvm.genx.wrregionf.v8i32.v8i32.v16i32.i16.i1(<8 x i32> [[OCT]], <16 x i32> [[MUL_HEX]], i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: tail call <8 x i32> @llvm.genx.wrregionf.v8i32.v8i32.v8i32.i16.i1(<8 x i32> [[OCT]], <8 x i32> [[WRREG10]], i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

; CHECK-LABEL: case11:
; CHECK-NEXT: tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.v8f32.i16.i1(<16 x float> [[FP_BIG]], <8 x float> [[MUL_FP]], i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: tail call <8 x float> @llvm.genx.rdregioni.v8f32.v16f32.i16.i1(<16 x float> [[FP_BIG]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[FMA3:%[A-Za-z0-9_.]+]] = call <8 x float> @llvm.fma.v8f32(<8 x float> [[FP]], <8 x float> [[FP]], <8 x float> [[FP]])
; CHECK-NEXT: %out11 = tail call <8 x float> @llvm.genx.wrregionf.v8f32.v8f32.v8f32.i16.i1(<8 x float> [[FMA3]], <8 x float> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

define spir_kernel void @test_mul(<16 x i32> %hex, <8 x i32> %oct, i32 %scl, <16 x i1> %flag, <8 x float> %fp, <16 x float> %fp_big) {
  %mul_hex = mul <16 x i32> %hex, %hex
  %mul_oct = mul <8 x i32> %oct, %oct
  %mul_scalar = mul i32 %scl, %scl
  %mul_fp = fmul <8 x float> %fp, %fp
  br label %case1

case1:
  ; IntMulLike::isAdd
  %wrreg1 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.i1(<16 x i32> %hex, <8 x i32> %mul_oct, i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)
  %rdreg1 = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16.i1(<16 x i32> %wrreg1, i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %uuadd = call <8 x i32> @llvm.genx.uuadd.sat.v8i32.v8i32(<8 x i32> %rdreg1, <8 x i32> %oct)
  %suadd = call <8 x i32> @llvm.genx.suadd.sat.v8i32.v8i32(<8 x i32> %rdreg1, <8 x i32> %oct)
  %usadd = call <8 x i32> @llvm.genx.usadd.sat.v8i32.v8i32(<8 x i32> %rdreg1, <8 x i32> %oct)
  %ssadd = call <8 x i32> @llvm.genx.ssadd.sat.v8i32.v8i32(<8 x i32> %rdreg1, <8 x i32> %oct)
  %add = add <8 x i32> %rdreg1, <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
  %out1 = tail call <8 x i32> @llvm.genx.wrregionf.v8i32.v8i32.v8i32.i16.i1(<8 x i32> %add, <8 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case2

case2:
  ; WrRegion -> RdRegion
  %wrreg2 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> %hex, i32 %mul_scalar, i32 16, i32 0, i32 0, i16 0, i32 0, i1 true)
  %wrreg2_pl = tail call <1 x i32> @llvm.genx.wrregionf.v1i32.v1i32.v16i32.i16.i1(<1 x i32> zeroinitializer, <16 x i32> %wrreg2, i32 1, i32 1, i32 1, i16 128, i32 0, i1 true)
  %rdreg2 = tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v1i32.i16.i1(<1 x i32> %wrreg2_pl, i32 0, i32 0, i32 1, i16 0, i32 undef, i1 true)
  br label %case3

case3:
  ; FPMulLike::isAdd
  %wrreg3 = tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.v8f32.i16.i1(<16 x float> %fp_big, <8 x float> %mul_fp, i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)
  %rdreg3 = tail call <8 x float> @llvm.genx.rdregioni.v8f32.v16f32.i16.i1(<16 x float> %wrreg3, i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %fadd = fadd <8 x float> %rdreg3, %fp
  %fsub = fsub <8 x float> %rdreg3, %fadd
  %out3 = tail call <8 x float> @llvm.genx.wrregionf.v8f32.v8f32.v8f32.i16.i1(<8 x float> %fsub, <8 x float> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case4

case4:
  ; R.overlap(W)
  %wrreg4 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.i1(<16 x i32> %hex, <8 x i32> %mul_oct, i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)
  %rdreg4 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16.i1(<16 x i32> %wrreg4, i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %uutrunc4 = call <16 x i32> @llvm.genx.uuadd.sat.v16i32.v16i32(<16 x i32> %rdreg4, <16 x i32> %hex)
  br label %case5

case5:
  ; Mul->User != WrRegion
  %rdreg5 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16.i1(<16 x i32> %mul_hex, i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
  br label %case6

case6:
  ; WII->getOperand(1) != Mul
  %wrreg6 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.i1(<16 x i32> %mul_hex, <8 x i32> zeroinitializer, i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)
  br label %case7

case7:
  ; !W.isStrictlySimilar(V)
  %wrreg7 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.v16i1(<16 x i32> %hex, <8 x i32> %mul_oct, i32 1, i32 1, i32 1, i16 0, i32 0, <16 x i1> %flag)
  br label %case8

case8:
  ; !RdRegion && !WrRegion
  %wrreg8 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> %hex, i32 %mul_scalar, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  %add8 = add <16 x i32> %wrreg8, zeroinitializer
  %out8 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> %add8, i32 zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case9

case9:
  ; W == W2
  %wrreg9 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> %hex, i32 %mul_scalar, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  %wrreg9_1 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32> %wrreg9, i32 %scl, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case10

case10:
  ; W2.overlap
  %wrreg10 = tail call <8 x i32> @llvm.genx.wrregionf.v8i32.v8i32.v16i32.i16.i1(<8 x i32> %oct, <16 x i32> %mul_hex, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  %wrreg10_2 = tail call <8 x i32> @llvm.genx.wrregionf.v8i32.v8i32.v8i32.i16.i1(<8 x i32> %oct, <8 x i32> %wrreg10, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case11

case11:
  ; not FPMulLike::isAdd
  %wrreg11 = tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.v8f32.i16.i1(<16 x float> %fp_big, <8 x float> %mul_fp, i32 8, i32 8, i32 1, i16 0, i32 0, i1 true)
  %rdreg11 = tail call <8 x float> @llvm.genx.rdregioni.v8f32.v16f32.i16.i1(<16 x float> %wrreg11, i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %fdiv = fadd <8 x float> %rdreg11, %fp
  %out11 = tail call <8 x float> @llvm.genx.wrregionf.v8f32.v8f32.v8f32.i16.i1(<8 x float> %fdiv, <8 x float> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

  ret void
}

declare <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.v8f32.i16.i1(<16 x float>, <8 x float>, i32, i32, i32, i16, i32, i1)
declare <8 x float> @llvm.genx.wrregionf.v8f32.v8f32.v8f32.i16.i1(<8 x float>, <8 x float>, i32, i32, i32, i16, i32, i1)
declare <8 x float> @llvm.genx.rdregioni.v8f32.v16f32.i16.i1(<16 x float>, i32, i32, i32, i16, i32, i1)

declare <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.i1(<16 x i32>, <8 x i32>, i32, i32, i32, i16, i32, i1)
declare <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.v8i32.i16.v16i1(<16 x i32>, <8 x i32>, i32, i32, i32, i16, i32, <16 x i1>)
declare <16 x i32> @llvm.genx.wrregionf.v16i32.v16i32.i32.i16.i1(<16 x i32>, i32, i32, i32, i32, i16, i32, i1)
declare <8 x i32> @llvm.genx.wrregionf.v8i32.v8i32.v8i32.i16.i1(<8 x i32>, <8 x i32>, i32, i32, i32, i16, i32, i1)
declare <1 x i32> @llvm.genx.wrregionf.v1i32.v1i32.v16i32.i16.i1(<1 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)
declare <8 x i32> @llvm.genx.wrregionf.v8i32.v8i32.v16i32.i16.i1(<8 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16.i1(<16 x i32>, i32, i32, i32, i16, i32, i1)
declare <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16.i1(<16 x i32>, i32, i32, i32, i16, i32, i1)
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v1i32.i16.i1(<1 x i32>, i32, i32, i32, i16, i32, i1)

declare <16 x i32> @llvm.genx.uuadd.sat.v16i32.v16i32(<16 x i32>, <16 x i32>)
declare <8 x i32> @llvm.genx.uuadd.sat.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.suadd.sat.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.usadd.sat.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.ssadd.sat.v8i32.v8i32(<8 x i32>, <8 x i32>)