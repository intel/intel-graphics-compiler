;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s

; COM: these tests just check that there is no compilation/asserts failures

; Function Attrs: nounwind readnone
declare <2 x i64> @llvm.genx.uuadd.sat.v2i64(<2 x i64>, <2 x i64>)
declare i64 @llvm.genx.uuadd.sat.si64(i64, i64)

declare <2 x i64> @llvm.genx.ssadd.sat.v2i64(<2 x i64>, <2 x i64>)
declare i64 @llvm.genx.ssadd.sat.si64(i64, i64)

declare <2 x i64> @llvm.genx.absi.v2i64(<2 x i64>)
declare i64 @llvm.genx.absi.si64(i64)

declare <2 x i64> @llvm.genx.fptosi.sat.v2i64.v2f32(<2 x float>)
declare i64 @llvm.genx.fptosi.sat.i64.f32(float)

declare <2 x i64> @llvm.genx.fptoui.sat.v2i64.v2f32(<2 x float>)
declare i64 @llvm.genx.fptoui.sat.i64.f32(float)

; CHECK: @test_genx_absi_sanity
define dllexport spir_kernel void @test_genx_absi_sanity(i64 %sop, <2 x i64> %vop) {

  %sval0 = call i64 @llvm.genx.absi.si64(i64 1)
  %sval = call i64 @llvm.genx.absi.si64(i64 %sop)
  %val = call <2 x i64> @llvm.genx.absi.v2i64(<2 x i64> %vop)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  %usval0 = bitcast i64 %sval0 to i64
  %usval = bitcast i64 %sval to i64
  ret void
}

; CHECK: @test_ssaddsat_sanity
define dllexport spir_kernel void @test_ssaddsat_sanity(i64 %sop, <2 x i64> %vop) {

  %sval0 = call i64 @llvm.genx.ssadd.sat.si64(i64 1, i64 1)
  %sval = call i64 @llvm.genx.ssadd.sat.si64(i64 %sop, i64 %sop)
  %val = call <2 x i64> @llvm.genx.ssadd.sat.v2i64(<2 x i64> %vop, <2 x i64> %vop)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  %usval0 = bitcast i64 %sval0 to i64
  %usval = bitcast i64 %sval to i64
  ret void
}

; CHECK: @test_uuaddsat_sanity
define dllexport spir_kernel void @test_uuaddsat_sanity(i64 %sop, <2 x i64> %vop) {

  %sval0 = call i64 @llvm.genx.uuadd.sat.si64(i64 1, i64 1)
  %sval = call i64 @llvm.genx.uuadd.sat.si64(i64 %sop, i64 %sop)
  %val = call <2 x i64> @llvm.genx.uuadd.sat.v2i64(<2 x i64> %vop, <2 x i64> %vop)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  %usval0 = bitcast i64 %sval0 to i64
  %usval = bitcast i64 %sval to i64
  ret void
}
; CHECK: test_shl_sanity
define dllexport spir_kernel void @test_shl_sanity(i64 %sop1, i64 %sop2,
                                                   <2 x i64> %vop) {
  %v0 = shl <2 x i64> %vop, <i64 2, i64 0>
  %v1 = shl <2 x i64> %vop, <i64 2, i64 31>
  %v2 = shl <2 x i64> %vop, <i64 32, i64 32>
  %v3 = shl <2 x i64> %vop, <i64 63, i64 50>
  %v4 = shl <2 x i64> %vop, <i64 63, i64 2>
  %v_ind = shl <2 x i64> %vop, %vop
  %s1 = shl i64 %sop1, 3
  %s2 = shl i64 %sop1, 32
  %s3 = shl i64 %sop1, 50
  %s_ind = shl i64 %sop1, %sop2
  ret void
}
; CHECK: test_lshr_sanity
define dllexport spir_kernel void @test_lshr_sanity(i64 %sop1, i64 %sop2,
                                                   <2 x i64> %vop) {
  %v0 = lshr <2 x i64> %vop, <i64 2, i64 0>
  %v1 = lshr <2 x i64> %vop, <i64 2, i64 31>
  %v2 = lshr <2 x i64> %vop, <i64 32, i64 32>
  %v3 = lshr <2 x i64> %vop, <i64 63, i64 50>
  %v4 = lshr <2 x i64> %vop, <i64 63, i64 2>
  %v_ind = lshr <2 x i64> %vop, %vop
  %s1 = lshr i64 %sop1, 3
  %s2 = lshr i64 %sop1, 32
  %s3 = lshr i64 %sop1, 50
  %s_ind = lshr i64 %sop1, %sop2

  ret void
}
; CHECK: test_ashr_sanity
define dllexport spir_kernel void @test_ashr_sanity(i64 %sop1, i64 %sop2,
                                                   <2 x i64> %vop) {

  %v0 = ashr <2 x i64> %vop, <i64 2, i64 0>
  %v1 = ashr <2 x i64> %vop, <i64 2, i64 31>
  %v2 = ashr <2 x i64> %vop, <i64 32, i64 32>
  %v3 = ashr <2 x i64> %vop, <i64 63, i64 50>
  %v4 = ashr <2 x i64> %vop, <i64 63, i64 2>
  %v_ind = ashr <2 x i64> %vop, %vop
  %s1 = ashr i64 %sop1, 3
  %s2 = ashr i64 %sop1, 32
  %s3 = ashr i64 %sop1, 50
  %s_ind = ashr i64 %sop1, %sop2
  ret void
}

; CHECK: @test_fp2ui_sanity
define dllexport spir_kernel void @test_fp2ui_sanity(float %sop, <2 x float> %vop) {

  %im = fptoui float 123.0 to i64
  %vi = fptoui <2 x float> <float 123.0 , float 124.0 > to <2 x i64>
  %sv = fptoui float %sop to i64
  %vv = fptoui <2 x float> %vop to <2 x i64>

  ; COM: users of the values (to ensure that we dont't break IR)
  %iu = bitcast i64 %im to i64
  %viu = bitcast <2 x i64> %vi to <2 x i64>
  %su = bitcast i64 %sv to i64
  %vu = bitcast <2 x i64> %vv to <2 x i64>
  ret void
}
; CHECK: @test_fp2si_sanity
define dllexport spir_kernel void @test_fp2si_sanity(float %sop, <2 x float> %vop) {

  %im = fptosi float 123.0 to i64
  %vi = fptosi <2 x float> <float 123.0, float 124.0> to <2 x i64>
  %sv = fptosi float %sop to i64
  %vv = fptosi <2 x float> %vop to <2 x i64>

  ; COM: users of the values (to ensure that we dont't break IR)
  %iu = bitcast i64 %im to i64
  %viu = bitcast <2 x i64> %vi to <2 x i64>
  %su = bitcast i64 %sv to i64
  %vu = bitcast <2 x i64> %vv to <2 x i64>
  ret void
}

; CHECK: @test_genx_fptosi_sanity
define dllexport spir_kernel void @test_genx_fptosi_sanity(float %sop, <2 x float> %vop) {
  %im = call i64 @llvm.genx.fptosi.sat.i64.f32(float -111.0)
  %vi = call <2 x i64> @llvm.genx.fptosi.sat.v2i64.v2f32(<2 x float> <float -111.0, float 111.0>)
  %sv = call i64 @llvm.genx.fptosi.sat.i64.f32(float %sop)
  %vv = call <2 x i64> @llvm.genx.fptosi.sat.v2i64.v2f32(<2 x float> %vop)

  ; COM: users of the values (to ensure that we dont't break IR)
  %iu = bitcast i64 %im to i64
  %viu = bitcast <2 x i64> %vi to <2 x i64>
  %su = bitcast i64 %sv to i64
  %vu = bitcast <2 x i64> %vv to <2 x i64>
  ret void
}
; CHECK: @test_genx_fptoui_sanity
define dllexport spir_kernel void @test_genx_fptoui_sanity(float %sop, <2 x float> %vop) {
  %im = call i64 @llvm.genx.fptoui.sat.i64.f32(float 111.0)
  %vi = call <2 x i64> @llvm.genx.fptoui.sat.v2i64.v2f32(<2 x float> <float 111.0, float 112.0>)
  %sv = call i64 @llvm.genx.fptoui.sat.i64.f32(float %sop)
  %vv = call <2 x i64> @llvm.genx.fptoui.sat.v2i64.v2f32(<2 x float> %vop)

  ; COM: users of the values (to ensure that we dont't break IR)
  %iu = bitcast i64 %im to i64
  %viu = bitcast <2 x i64> %vi to <2 x i64>
  %su = bitcast i64 %sv to i64
  %vu = bitcast <2 x i64> %vv to <2 x i64>
  ret void
}

; CHECK: @test_ui2fp_sanity
define dllexport spir_kernel void @test_ui2fp_sanity(i64 %sop, <2 x i64> %vop) {
  %si = uitofp i64 1 to float
  %vi = uitofp <2 x i64> <i64 1, i64 2> to <2 x float>
  %sv = uitofp i64 %sop to float
  %vv = uitofp <2 x i64> %vop to <2 x float>

  ; COM: users of the values (to ensure that we dont't break IR)
  %iu = bitcast float %si to float
  %viu = bitcast <2 x float> %vi to <2 x float>
  %su = bitcast float %sv to float
  %vu = bitcast <2 x float> %vv to <2 x float>
  ret void
}
; CHECK: @test_si2fp_sanity
define dllexport spir_kernel void @test_si2fp_sanity(i64 %sop, <2 x i64> %vop) {
  %si = sitofp i64 1 to float
  %vi = sitofp <2 x i64> <i64 1, i64 2> to <2 x float>
  %sv = sitofp i64 %sop to float
  %vv = sitofp <2 x i64> %vop to <2 x float>

  ; COM: users of the values (to ensure that we dont't break IR)
  %iu = bitcast float %si to float
  %viu = bitcast <2 x float> %vi to <2 x float>
  %su = bitcast float %sv to float
  %vu = bitcast <2 x float> %vv to <2 x float>
  ret void
}

; COM: The presence of these __cm_intrinsic_* funcitions is a HACK to trick VC
; COM: backend into thinking that we have emulation routines
define <2 x i64> @__cm_intrinsic_impl_fp2ui(<2 x float>) #0 {
  ret <2 x i64> zeroinitializer
}
define <2 x i64> @__cm_intrinsic_impl_fp2si(<2 x float>) #0 {
  ret <2 x i64> zeroinitializer
}
define <2 x float> @__cm_intrinsic_impl_ui2fp(<2 x i64>) #0 {
  ret <2 x float> zeroinitializer
}
define <2 x float> @__cm_intrinsic_impl_si2fp(<2 x i64>) #0 {
  ret <2 x float> zeroinitializer
}

define i64 @__cm_intrinsic_impl_fp2ui_base(float) #0 {
  ret i64 zeroinitializer
}
define i64 @__cm_intrinsic_impl_fp2si_base(float) #0 {
  ret i64 zeroinitializer
}
define float @__cm_intrinsic_impl_ui2fp_base(i64) #0 {
  ret float zeroinitializer
}
define float @__cm_intrinsic_impl_si2fp_base(i64) #0 {
  ret float zeroinitializer
}
attributes #0 = { "VC.Emulation.Routine" }
