;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info=true -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck --match-full-lines %s
; ------------------------------------------------
; GenXBaling
; ------------------------------------------------
; This test checks that GenXBaling doesn't bale vload user to position where it potentially gets clobbered by vstore.

attributes #0 = { "genx_volatile" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

@_ZL1A = external dso_local global <32 x double> #0
@_ZL1Z = external dso_local global <32 x double> #0

define internal spir_func void @vload_user_not_baled_across_vstore1(<32 x double> %call.i.i.i.i170.esimd73, <32 x double> %call.i.i.i.i297.esimd80) {
entry:
  %call.i.i.i123.esimd69 = load volatile <32 x double>, <32 x double>* @_ZL1A, align 256
  %vecext.i1372.regioncollapsed37 = tail call double @llvm.genx.rdregionf.f64.v32f64.i16(<32 x double> %call.i.i.i123.esimd69, i32 0, i32 0, i32 0, i16 0, i32 0)
  %mad20 = call double @llvm.fma.f64(double %vecext.i1372.regioncollapsed37, double 0.000000e+00, double 0.000000e+00)
  store volatile <32 x double> zeroinitializer, <32 x double>* @_ZL1A, align 256
  %call.i.i.i.i170.esimd731 = load volatile <32 x double>, <32 x double>* null, align 256
  %call4.i.i.i174.esimd = call <32 x double> @llvm.genx.wrregionf.v32f64.f64.i16.i1(<32 x double> %call.i.i.i.i170.esimd73, double %mad20, i32 0, i32 1, i32 1, i16 8, i32 undef, i1 true)
; COM: operand #1 must not be baled.
; CHECK: %call4.i.i.i174.esimd = {{.+}}: wrregion
; CHECK-NOT: %call4.i.i.i174.esimd = {{.+}}: wrregion 1
  store volatile <32 x double> %call4.i.i.i174.esimd, <32 x double>* null, align 256
  %call.i.i.i191.esimd75 = load volatile <32 x double>, <32 x double>* @_ZL1Z, align 256
  %call2.i.i.i196.esimd = tail call <2 x double> @llvm.genx.rdregionf.v2f64.v32f64.i16(<32 x double> %call.i.i.i191.esimd75, i32 0, i32 0, i32 0, i16 0, i32 0)
  %mad22 = call <2 x double> @llvm.fma.v2f64(<2 x double> zeroinitializer, <2 x double> %call2.i.i.i196.esimd, <2 x double> zeroinitializer)
  store volatile <32 x double> zeroinitializer, <32 x double>* @_ZL1Z, align 256
  %call.i.i.i.i297.esimd802 = load volatile <32 x double>, <32 x double>* null, align 256
; COM: operand #1 must not be baled.
  %call4.i.i.i301.esimd = tail call <32 x double> @llvm.genx.wrregionf.v32f64.v2f64.i16.v2i1(<32 x double> %call.i.i.i.i297.esimd80, <2 x double> %mad22, i32 0, i32 2, i32 1, i16 0, i32 0, <2 x i1> <i1 true, i1 true>)
; CHECK: %call4.i.i.i301.esimd = {{.+}}: wrregion
; CHECK-NOT: %call4.i.i.i301.esimd = {{.+}}: wrregion 1
  store volatile <32 x double> %call4.i.i.i301.esimd, <32 x double>* null, align 256
  ret void
}

declare <32 x double> @llvm.genx.wrregionf.v32f64.v2f64.i16.v2i1(<32 x double>, <2 x double>, i32, i32, i32, i16, i32, <2 x i1>)
declare <32 x double> @llvm.genx.wrregionf.v32f64.f64.i16.i1(<32 x double>, double, i32, i32, i32, i16, i32, i1)
declare <2 x double> @llvm.genx.rdregionf.v2f64.v32f64.i16(<32 x double>, i32, i32, i32, i16, i32)
declare double @llvm.genx.rdregionf.f64.v32f64.i16(<32 x double>, i32, i32, i32, i16, i32)
declare double @llvm.fma.f64(double, double, double) #1
declare <2 x double> @llvm.fma.v2f64(<2 x double>, <2 x double>, <2 x double>) #1

;---------------------------------------------------------------------------

@_ZL8g_global = external dso_local global <4 x i32> #0

define spir_kernel void @vload_user_not_baled_across_vstore2(<1 x i64> %.splatinsert13) {
entry:
  %call.i.i.i8.i.esimd8 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global, align 16
  %vecext.i.i1.regioncollapsed = call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd8, i32 0, i32 0, i32 0, i16 0, i32 0)
  store volatile <4 x i32> zeroinitializer, <4 x i32>* @_ZL8g_global, align 16
  %.splatinsert131 = bitcast i64 0 to <1 x i64>
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert13, <1 x i32> %vecext.i.i1.regioncollapsed)
; COM: operand #3 must not be baled-in due to potentially clobbering store.
; CHECK-NOT: call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert13, <1 x i32> %vecext.i.i1.regioncollapsed): maininst 3
  ret void
}

declare <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32>, <1 x i32>, i32, i32, i32, i16, i32, <1 x i1>)
declare <1 x i32> @llvm.genx.svm.gather.v1i32.v1i1.v1i64(<1 x i1>, i32, <1 x i64>, <1 x i32>)
declare void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1>, i32, <1 x i64>, <1 x i32>)
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
