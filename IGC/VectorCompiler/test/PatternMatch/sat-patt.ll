;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_sat
; CHECK: (<4 x i16> [[ARG:%[A-Za-z0-9_.]+]])
; CHECK-NEXT: [[ZUU:%[A-Za-z0-9_.]+]] = call <4 x i32> @llvm.genx.uutrunc.sat.v4i32.v4i16(<4 x i16> [[ARG]])
; CHECK-NEXT: [[SUU:%[A-Za-z0-9_.]+]] = call <4 x i32> @llvm.genx.ustrunc.sat.v4i32.v4i16(<4 x i16> [[ARG]])
; CHECK-NEXT: [[ADD_UU:%[A-Za-z0-9_.]+]] = add <4 x i32> [[ZUU]], [[SUU]]
; CHECK-NEXT: [[ZSU:%[A-Za-z0-9_.]+]] = call <4 x i32> @llvm.genx.sutrunc.sat.v4i32.v4i16(<4 x i16> [[ARG]])
; CHECK-NEXT: [[SSU:%[A-Za-z0-9_.]+]] = call <4 x i32> @llvm.genx.sstrunc.sat.v4i32.v4i16(<4 x i16> [[ARG]])
; CHECK-NEXT: [[ADD_SU:%[A-Za-z0-9_.]+]] = add <4 x i32> [[ZSU]], [[SSU]]
; CHECK-NEXT: [[ZUS:%[A-Za-z0-9_.]+]] = call <4 x i32> @llvm.genx.uutrunc.sat.v4i32.v4i16(<4 x i16> [[ARG]])
; CHECK-NEXT: [[SUS:%[A-Za-z0-9_.]+]] = call <4 x i32> @llvm.genx.ustrunc.sat.v4i32.v4i16(<4 x i16> [[ARG]])
; CHECK-NEXT: [[ADD_US:%[A-Za-z0-9_.]+]] = add <4 x i32> [[ZUS]], [[SUS]]
; CHECK-NEXT: [[ZSS:%[A-Za-z0-9_.]+]] = call <4 x i32> @llvm.genx.sutrunc.sat.v4i32.v4i16(<4 x i16> [[ARG]])
; CHECK-NEXT: [[SSS:%[A-Za-z0-9_.]+]] = call <4 x i32> @llvm.genx.sstrunc.sat.v4i32.v4i16(<4 x i16> [[ARG]])
; CHECK-NEXT: [[ADD_SS:%[A-Za-z0-9_.]+]] = add <4 x i32> [[ZSS]], [[SSS]]
; CHECK-NEXT: [[ADD1:%[A-Za-z0-9_.]+]] = add <4 x i32> [[ADD_UU]], [[ADD_SU]]
; CHECK-NEXT: [[ADD2:%[A-Za-z0-9_.]+]] = add <4 x i32> [[ADD_US]], [[ADD1]]
; CHECK-NEXT: [[ADD3:%[A-Za-z0-9_.]+]] = add <4 x i32> [[ADD_SS]], [[ADD2]]
; CHECK-NEXT: tail call <4 x i32> @llvm.genx.wrregionf.v4i32.v1i32.i16.i1(<4 x i32> [[ADD3]], <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)

define spir_kernel void @test_sat(<4 x i16> %arg) {
  %zin = zext <4 x i16> %arg to <4 x i32>
  %sin = sext <4 x i16> %arg to <4 x i32>
  %zuu = call <4 x i32> @llvm.genx.uutrunc.sat.v4i32.v4i32(<4 x i32> %zin)
  %suu = call <4 x i32> @llvm.genx.uutrunc.sat.v4i32.v4i32(<4 x i32> %sin)
  %add_uu = add <4 x i32> %zuu, %suu
  %zsu = call <4 x i32> @llvm.genx.sutrunc.sat.v4i32.v4i32(<4 x i32> %zin)
  %ssu = call <4 x i32> @llvm.genx.sutrunc.sat.v4i32.v4i32(<4 x i32> %sin)
  %add_su = add <4 x i32> %zsu, %ssu
  %zus = call <4 x i32> @llvm.genx.ustrunc.sat.v4i32.v4i32(<4 x i32> %zin)
  %sus = call <4 x i32> @llvm.genx.ustrunc.sat.v4i32.v4i32(<4 x i32> %sin)
  %add_us = add <4 x i32> %zus, %sus
  %zss = call <4 x i32> @llvm.genx.sstrunc.sat.v4i32.v4i32(<4 x i32> %zin)
  %sss = call <4 x i32> @llvm.genx.sstrunc.sat.v4i32.v4i32(<4 x i32> %sin)
  %add_ss = add <4 x i32> %zss, %sss
  %add1 = add <4 x i32> %add_uu, %add_su
  %add2 = add <4 x i32> %add_us, %add1
  %add3 = add <4 x i32> %add_ss, %add2
  %wrreg = tail call <4 x i32> @llvm.genx.wrregionf.v4i32.v1i32.i16.i1(<4 x i32> %add3, <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  ret void
}

declare <4 x i32> @llvm.genx.uutrunc.sat.v4i32.v4i32(<4 x i32>)
declare <4 x i32> @llvm.genx.sutrunc.sat.v4i32.v4i32(<4 x i32>)
declare <4 x i32> @llvm.genx.ustrunc.sat.v4i32.v4i32(<4 x i32>)
declare <4 x i32> @llvm.genx.sstrunc.sat.v4i32.v4i32(<4 x i32>)
declare <4 x i32> @llvm.genx.wrregionf.v4i32.v1i32.i16.i1(<4 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)