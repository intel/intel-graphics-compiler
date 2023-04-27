;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXIMadLegalization -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXIMadPostLegalization
; ------------------------------------------------
; This test checks that GenXIMadPostLegalization pass doesn't move global volatile users to a positon where they get
; potentially clobbered by a corresponding global volatile store.
; It covers fixMadChain routine.
; -----------------------------------------------------------------------------------
; TODO: need to add mad legalization tests covering the whole optimization logic.
; -----------------------------------------------------------------------------------
;
target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@_ZL1A = internal global <32 x double> zeroinitializer, align 256, !spirv.Decorations !0 #0

define dllexport spir_kernel void @MadGVLoadClobberTest_chkFixMadChainRoutine() #6 {
entry:
  %call.i.i.i123.esimd69 = load volatile <32 x double>, <32 x double>* @_ZL1A
  %vecext.i1.regioncollapsed = tail call double @llvm.genx.rdregionf.f64.v32f64.i16(<32 x double> %call.i.i.i123.esimd69, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %vecext.i1372.regioncollapsed = tail call double @llvm.genx.rdregionf.f64.v32f64.i16(<32 x double> %call.i.i.i123.esimd69, i32 0, i32 1, i32 1, i16 16, i32 undef)
  %vecext.i1503.regioncollapsed = tail call double @llvm.genx.rdregionf.f64.v32f64.i16(<32 x double> %call.i.i.i123.esimd69, i32 0, i32 1, i32 1, i16 8, i32 undef)
  %sub = fsub double %vecext.i1503.regioncollapsed, %vecext.i1.regioncollapsed
  %mul = fmul double %vecext.i1372.regioncollapsed, 2.000000e+00
  %div = fdiv double %sub, %mul
  %constant = call double @llvm.genx.constantf.f64(double 1.000000e+00)
  %mad = call double @llvm.fma.f64(double %div, double %div, double %constant)
  %splat.splatinsert.i.i.i13 = bitcast double %mad to <1 x double>
  %call1.i.i.esimd = tail call <1 x double> @llvm.genx.ieee.sqrt.v1f64(<1 x double> %splat.splatinsert.i.i.i13)
  %vecext.i.i1214 = bitcast <1 x double> %call1.i.i.esimd to double
  %cmp.ordered.inversed = fcmp oge double %div, 0.000000e+00
  %cmp.ordered.inversed.not = xor i1 %cmp.ordered.inversed, true
  %0 = fsub double -0.000000e+00, %vecext.i.i1214
  %add.pn.p = select i1 %cmp.ordered.inversed.not, double %0, double %vecext.i.i1214
  %add.pn = fadd double %div, %add.pn.p
  %cond = fdiv double 1.000000e+00, %add.pn
  %mad16 = call double @llvm.fma.f64(double %cond, double %cond, double %constant)
  %splat.splatinsert.i.i.i10511 = bitcast double %mad16 to <1 x double>
  %call1.i.i110.esimd = tail call <1 x double> @llvm.genx.ieee.sqrt.v1f64(<1 x double> %splat.splatinsert.i.i.i10511)
  %vecext.i.i1161015 = bitcast <1 x double> %call1.i.i110.esimd to double
  %div13 = fdiv double 1.000000e+00, %vecext.i.i1161015
  %mul14 = fmul double %div13, %cond
  %mul15 = fmul double %div13, %div13
  %mul17 = fmul double %mul14, %mul14
  %mul18 = fmul double %vecext.i1503.regioncollapsed, %mul17
  %mad17 = call double @llvm.fma.f64(double %vecext.i1.regioncollapsed, double %mul15, double %mul18)
  %mul19 = fmul double %div13, 2.000000e+00
  %mul20 = fmul double %mul19, %mul14
; COM: @CAN_ALSO_CHECK:  %mul20 = fmul double %mul19, %mul14
; COM: @CAN_ALSO_CHECK-NEXT:  %mul25 = fmul double %vecext.i1.regioncollapsed[[POST_BALING_CLONE_IDX0:[0-9]+]], %mul17
; COM: @CAN_ALSO_CHECK-NEXT:   %mad19 = call double @llvm.fma.f64(double %vecext.i1503.regioncollapsed[[POST_BALING_CLONE_IDX1:[0-9]+]], double %mul15, double %mul25)
; COM: @CAN_ALSO_CHECK-NEXT:   %mad20 = call double @llvm.fma.f64(double %vecext.i1372.regioncollapsed[[POST_BALING_CLONE_IDX2:[0-9]+]], double %mul20, double %mad19)
  %fneg = fneg double %mul20
  %mad18 = call double @llvm.fma.f64(double %vecext.i1372.regioncollapsed, double %fneg, double %mad17)
  %mul25 = fmul double %vecext.i1.regioncollapsed, %mul17
  %mad19 = call double @llvm.fma.f64(double %vecext.i1503.regioncollapsed, double %mul15, double %mul25)
  %mad20 = call double @llvm.fma.f64(double %vecext.i1372.regioncollapsed, double %mul20, double %mad19)
  %call4.i.i.i.esimd = call <32 x double> @llvm.genx.wrregionf.v32f64.f64.i16.i1(<32 x double> %call.i.i.i123.esimd69, double %mad18, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  store volatile <32 x double> %call4.i.i.i.esimd, <32 x double>* @_ZL1A
  %call.i.i.i.i170.esimd73 = load volatile <32 x double>, <32 x double>* @_ZL1A
; CHECK: store volatile <32 x double> %call4.i.i.i.esimd, <32 x double>* @_ZL1A
; CHECK-NEXT: %call.i.i.i.i170.esimd73 = load volatile <32 x double>, <32 x double>* @_ZL1A
; CHECK-NOT:  %vecext.i1.regioncollapsed[[POST_BALING_CLONE_IDX0:[0-9]+]] = tail call double @llvm.genx.rdregionf.f64.v32f64.i16(<32 x double> %call.i.i.i123.esimd69, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NOT:  %mul25 = fmul double %vecext.i1.regioncollapsed[[POST_BALING_CLONE_IDX0:[0-9]+]], %mul17
; CHECK-NOT:  %vecext.i1503.regioncollapsed[[POST_BALING_CLONE_IDX1:[0-9]+]] = tail call double @llvm.genx.rdregionf.f64.v32f64.i16(<32 x double> %call.i.i.i123.esimd69, i32 0, i32 1, i32 1, i16 8, i32 undef)
; CHECK-NOT:  %mad19 = call double @llvm.fma.f64(double %vecext.i1503.regioncollapsed[[POST_BALING_CLONE_IDX1:[0-9]+]], double %mul15, double %mul25)
; CHECK-NOT:  %vecext.i1372.regioncollapsed[[POST_BALING_CLONE_IDX2:[0-9]+]] = tail call double @llvm.genx.rdregionf.f64.v32f64.i16(<32 x double> %call.i.i.i123.esimd69, i32 0, i32 1, i32 1, i16 16, i32 undef)
; CHECK-NOT:  %mad20 = call double @llvm.fma.f64(double %vecext.i1372.regioncollapsed[[POST_BALING_CLONE_IDX2:[0-9]+]], double %mul20, double %mad19)
; CHECK: %call4.i.i.i174.esimd = call <32 x double> @llvm.genx.wrregionf.v32f64.f64.i16.i1(<32 x double> %call.i.i.i.i170.esimd73, double %mad20, i32 0, i32
  %call4.i.i.i174.esimd = call <32 x double> @llvm.genx.wrregionf.v32f64.f64.i16.i1(<32 x double> %call.i.i.i.i170.esimd73, double %mad20, i32 0, i32 1, i32 1, i16 8, i32 undef, i1 true)
  store volatile <32 x double> %call4.i.i.i174.esimd, <32 x double>* @_ZL1A
  ret void
}

declare double @llvm.genx.constantf.f64(double) #2
declare double @llvm.fma.f64(double, double, double) #9
declare <1 x double> @llvm.genx.ieee.sqrt.v1f64(<1 x double>) #2
declare <2 x double> @llvm.fma.v2f64(<2 x double>, <2 x double>, <2 x double>) #9
declare <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32>, <8 x i32>, <8 x i32>) #2
declare <1 x i16> @llvm.genx.rdregioni.v1i16.v4i16.i16(<4 x i16>, i32, i32, i32, i16, i32) #2
declare double @llvm.genx.rdregionf.f64.v32f64.i16(<32 x double>, i32, i32, i32, i16, i32) #2
declare <2 x double> @llvm.genx.rdregionf.v2f64.v1f64.i16(<1 x double>, i32, i32, i32, i16, i32) #2
declare <32 x double> @llvm.genx.wrregionf.v32f64.f64.i16.i1(<32 x double>, double, i32, i32, i32, i16, i32, i1) #2

attributes #0 = { "VCByteOffset"="1024" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="1024" "genx_volatile" }
attributes #2 = { nounwind readnone }
attributes #6 = { nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #9 = { nounwind readnone speculatable willreturn }

!0 = !{!1, !2, !3, !4}
!1 = !{i32 21}
!2 = !{i32 44, i32 256}
!3 = !{i32 5624}
!4 = !{i32 5628, i32 1024}
