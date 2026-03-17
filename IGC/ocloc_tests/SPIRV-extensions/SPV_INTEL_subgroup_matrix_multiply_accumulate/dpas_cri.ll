;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported, typed-pointers

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_matrix_multiply_accumulate -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options " -igc_opts 'PrintToConsole=1 PrintAfter=ArithmeticFuncsTranslation'" 2>&1 | FileCheck %s --check-prefix=CHECK-GENISA
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options " -igc_opts 'DumpVISAASMToConsole=1'"                                 2>&1 | FileCheck %s --check-prefix=CHECK-VISAASM

target triple = "spir64-unknown-unknown"

declare spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_ifi(i32, i16 signext, <8 x i32>, float, i32)
declare spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_fi(i32, <2 x i16>, <8 x i32>, <2 x float>, i32)
declare spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_fi(i32, <4 x i16>, <8 x i32>, <4 x float>, i32)
declare spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_fi(i32, <8 x i16>, <8 x i32>, <8 x float>, i32)

declare spir_func signext i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32, i16 signext, <8 x i32>, i16 signext, i32)
declare spir_func <2 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iS_i(i32, <2 x i16>, <8 x i32>, <2 x i16>, i32)
declare spir_func <4 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iS_i(i32, <4 x i16>, <8 x i32>, <4 x i16>, i32)
declare spir_func <8 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS_i(i32, <8 x i16>, <8 x i32>, <8 x i16>, i32)

; fp8 matrix sources (hf8 and bf8), fp32 accumulator:
define spir_kernel void @test_v1(float* %res1, <2 x float>* %res2, <4 x float>* %res4, <8 x float>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 float %c1, <2 x float> %c2, <4 x float> %c4, <8 x float> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v1(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i16.v8i32(float %c1, i16 %a1, <8 x i32> %b, i32 7, i32 7, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store float [[DPAS]], float* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.v2i16.v8i32(<2 x float> %c2, <2 x i16> %a2, <8 x i32> %b, i32 7, i32 7, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x float> [[DPAS1]], <2 x float>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %c4, <4 x i16> %a4, <8 x i32> %b, i32 7, i32 7, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x float> [[DPAS2]], <4 x float>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %c8, <8 x i16> %a8, <8 x i32> %b, i32 7, i32 7, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x float> [[DPAS3]], <8 x float>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v1"
; CHECK-VISAASM-DAG: dpas.bf8.bf8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.bf8.bf8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.bf8.bf8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.bf8.bf8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call0 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_ifi(i32 32, i16 %a1, <8 x i32> %b, float %c1, i32 196608)
  store float %call0, float* %res1
  %call1 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_fi(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x float> %c2, i32 196608)
  store <2 x float> %call1, <2 x float>* %res2
  %call2 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_fi(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x float> %c4, i32 196608)
  store <4 x float> %call2, <4 x float>* %res4
  %call3 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_fi(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x float> %c8, i32 196608)
  store <8 x float> %call3, <8 x float>* %res8

  ret void
}

define spir_kernel void @test_v2(float* %res1, <2 x float>* %res2, <4 x float>* %res4, <8 x float>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 float %c1, <2 x float> %c2, <4 x float> %c4, <8 x float> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v2(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i16.v8i32(float %c1, i16 %a1, <8 x i32> %b, i32 7, i32 8, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store float [[DPAS]], float* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.v2i16.v8i32(<2 x float> %c2, <2 x i16> %a2, <8 x i32> %b, i32 7, i32 8, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x float> [[DPAS1]], <2 x float>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %c4, <4 x i16> %a4, <8 x i32> %b, i32 7, i32 8, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x float> [[DPAS2]], <4 x float>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %c8, <8 x i16> %a8, <8 x i32> %b, i32 7, i32 8, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x float> [[DPAS3]], <8 x float>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v2"
; CHECK-VISAASM-DAG: dpas.hf8.bf8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.hf8.bf8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.hf8.bf8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.hf8.bf8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call4 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_ifi(i32 32, i16 %a1, <8 x i32> %b, float %c1, i32 98304)
  store float %call4, float* %res1
  %call5 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_fi(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x float> %c2, i32 98304)
  store <2 x float> %call5, <2 x float>* %res2
  %call6 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_fi(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x float> %c4, i32 98304)
  store <4 x float> %call6, <4 x float>* %res4
  %call7 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_fi(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x float> %c8, i32 98304)
  store <8 x float> %call7, <8 x float>* %res8

  ret void
}

define spir_kernel void @test_v3(float* %res1, <2 x float>* %res2, <4 x float>* %res4, <8 x float>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 float %c1, <2 x float> %c2, <4 x float> %c4, <8 x float> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v3(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i16.v8i32(float %c1, i16 %a1, <8 x i32> %b, i32 8, i32 7, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store float [[DPAS]], float* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.v2i16.v8i32(<2 x float> %c2, <2 x i16> %a2, <8 x i32> %b, i32 8, i32 7, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x float> [[DPAS1]], <2 x float>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %c4, <4 x i16> %a4, <8 x i32> %b, i32 8, i32 7, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x float> [[DPAS2]], <4 x float>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %c8, <8 x i16> %a8, <8 x i32> %b, i32 8, i32 7, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x float> [[DPAS3]], <8 x float>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v3"
; CHECK-VISAASM-DAG: dpas.bf8.hf8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.bf8.hf8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.bf8.hf8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.bf8.hf8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call8 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_ifi(i32 32, i16 %a1, <8 x i32> %b, float %c1, i32 147456)
  store float %call8, float* %res1
  %call9 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_fi(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x float> %c2, i32 147456)
  store <2 x float> %call9, <2 x float>* %res2
  %call10 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_fi(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x float> %c4, i32 147456)
  store <4 x float> %call10, <4 x float>* %res4
  %call11 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_fi(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x float> %c8, i32 147456)
  store <8 x float> %call11, <8 x float>* %res8

  ret void
}

define spir_kernel void @test_v4(float* %res1, <2 x float>* %res2, <4 x float>* %res4, <8 x float>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 float %c1, <2 x float> %c2, <4 x float> %c4, <8 x float> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v4(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i16.v8i32(float %c1, i16 %a1, <8 x i32> %b, i32 8, i32 8, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store float [[DPAS]], float* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.v2i16.v8i32(<2 x float> %c2, <2 x i16> %a2, <8 x i32> %b, i32 8, i32 8, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x float> [[DPAS1]], <2 x float>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %c4, <4 x i16> %a4, <8 x i32> %b, i32 8, i32 8, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x float> [[DPAS2]], <4 x float>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %c8, <8 x i16> %a8, <8 x i32> %b, i32 8, i32 8, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x float> [[DPAS3]], <8 x float>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v4"
; CHECK-VISAASM-DAG: dpas.hf8.hf8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.hf8.hf8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.hf8.hf8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.hf8.hf8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call12 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_ifi(i32 32, i16 %a1, <8 x i32> %b, float %c1, i32 49152)
  store float %call12, float* %res1
  %call13 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_fi(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x float> %c2, i32 49152)
  store <2 x float> %call13, <2 x float>* %res2
  %call14 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_fi(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x float> %c4, i32 49152)
  store <4 x float> %call14, <4 x float>* %res4
  %call15 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_fi(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x float> %c8, i32 49152)
  store <8 x float> %call15, <8 x float>* %res8

  ret void
}

; fp8 matrix sources (hf8 and bf8), bf16 accumulator:
define spir_kernel void @test_v5(i16* %res1, <2 x i16>* %res2, <4 x i16>* %res4, <8 x i16>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i16 %c1, <2 x i16> %c2, <4 x i16> %c4, <8 x i16> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v5(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %c1, i16 %a1, <8 x i32> %b, i32 7, i32 7, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i16 [[DPAS]], i16* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i16> @llvm.genx.GenISA.sub.group.dpas.v2i16.v2i16.v2i16.v8i32(<2 x i16> %c2, <2 x i16> %a2, <8 x i32> %b, i32 7, i32 7, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i16> [[DPAS1]], <2 x i16>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i16> @llvm.genx.GenISA.sub.group.dpas.v4i16.v4i16.v4i16.v8i32(<4 x i16> %c4, <4 x i16> %a4, <8 x i32> %b, i32 7, i32 7, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i16> [[DPAS2]], <4 x i16>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16> %c8, <8 x i16> %a8, <8 x i32> %b, i32 7, i32 7, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i16> [[DPAS3]], <8 x i16>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v5"
; CHECK-VISAASM-DAG: dpas.bf8.bf8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.bf8.bf8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.bf8.bf8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.bf8.bf8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call16 = call spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32 32, i16 %a1, <8 x i32> %b, i16 %c1, i32 196620)
  store i16 %call16, i16* %res1
  %call17 = call spir_func <2 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iS_i(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x i16> %c2, i32 196620)
  store <2 x i16> %call17, <2 x i16>* %res2
  %call18 = call spir_func <4 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iS_i(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x i16> %c4, i32 196620)
  store <4 x i16> %call18, <4 x i16>* %res4
  %call19 = call spir_func <8 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS_i(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x i16> %c8, i32 196620)
  store <8 x i16> %call19, <8 x i16>* %res8

  ret void
}

define spir_kernel void @test_v6(i16* %res1, <2 x i16>* %res2, <4 x i16>* %res4, <8 x i16>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i16 %c1, <2 x i16> %c2, <4 x i16> %c4, <8 x i16> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v6(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %c1, i16 %a1, <8 x i32> %b, i32 8, i32 7, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i16 [[DPAS]], i16* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i16> @llvm.genx.GenISA.sub.group.dpas.v2i16.v2i16.v2i16.v8i32(<2 x i16> %c2, <2 x i16> %a2, <8 x i32> %b, i32 8, i32 7, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i16> [[DPAS1]], <2 x i16>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i16> @llvm.genx.GenISA.sub.group.dpas.v4i16.v4i16.v4i16.v8i32(<4 x i16> %c4, <4 x i16> %a4, <8 x i32> %b, i32 8, i32 7, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i16> [[DPAS2]], <4 x i16>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16> %c8, <8 x i16> %a8, <8 x i32> %b, i32 8, i32 7, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i16> [[DPAS3]], <8 x i16>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v6"
; CHECK-VISAASM-DAG: dpas.bf8.hf8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.bf8.hf8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.bf8.hf8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.bf8.hf8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call20 = call spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32 32, i16 %a1, <8 x i32> %b, i16 %c1, i32 147468)
  store i16 %call20, i16* %res1
  %call21 = call spir_func <2 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iS_i(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x i16> %c2, i32 147468)
  store <2 x i16> %call21, <2 x i16>* %res2
  %call22 = call spir_func <4 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iS_i(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x i16> %c4, i32 147468)
  store <4 x i16> %call22, <4 x i16>* %res4
  %call23 = call spir_func <8 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS_i(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x i16> %c8, i32 147468)
  store <8 x i16> %call23, <8 x i16>* %res8

  ret void
}

define spir_kernel void @test_v7(i16* %res1, <2 x i16>* %res2, <4 x i16>* %res4, <8 x i16>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i16 %c1, <2 x i16> %c2, <4 x i16> %c4, <8 x i16> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v7(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %c1, i16 %a1, <8 x i32> %b, i32 7, i32 8, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i16 [[DPAS]], i16* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i16> @llvm.genx.GenISA.sub.group.dpas.v2i16.v2i16.v2i16.v8i32(<2 x i16> %c2, <2 x i16> %a2, <8 x i32> %b, i32 7, i32 8, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i16> [[DPAS1]], <2 x i16>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i16> @llvm.genx.GenISA.sub.group.dpas.v4i16.v4i16.v4i16.v8i32(<4 x i16> %c4, <4 x i16> %a4, <8 x i32> %b, i32 7, i32 8, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i16> [[DPAS2]], <4 x i16>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16> %c8, <8 x i16> %a8, <8 x i32> %b, i32 7, i32 8, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i16> [[DPAS3]], <8 x i16>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v7"
; CHECK-VISAASM-DAG: dpas.hf8.bf8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.hf8.bf8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.hf8.bf8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.hf8.bf8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call24 = call spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32 32, i16 %a1, <8 x i32> %b, i16 %c1, i32 98316)
  store i16 %call24, i16* %res1
  %call25 = call spir_func <2 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iS_i(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x i16> %c2, i32 98316)
  store <2 x i16> %call25, <2 x i16>* %res2
  %call26 = call spir_func <4 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iS_i(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x i16> %c4, i32 98316)
  store <4 x i16> %call26, <4 x i16>* %res4
  %call27 = call spir_func <8 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS_i(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x i16> %c8, i32 98316)
  store <8 x i16> %call27, <8 x i16>* %res8

  ret void
}

define spir_kernel void @test_v8(i16* %res1, <2 x i16>* %res2, <4 x i16>* %res4, <8 x i16>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i16 %c1, <2 x i16> %c2, <4 x i16> %c4, <8 x i16> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v8(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %c1, i16 %a1, <8 x i32> %b, i32 8, i32 8, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i16 [[DPAS]], i16* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i16> @llvm.genx.GenISA.sub.group.dpas.v2i16.v2i16.v2i16.v8i32(<2 x i16> %c2, <2 x i16> %a2, <8 x i32> %b, i32 8, i32 8, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i16> [[DPAS1]], <2 x i16>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i16> @llvm.genx.GenISA.sub.group.dpas.v4i16.v4i16.v4i16.v8i32(<4 x i16> %c4, <4 x i16> %a4, <8 x i32> %b, i32 8, i32 8, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i16> [[DPAS2]], <4 x i16>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16> %c8, <8 x i16> %a8, <8 x i32> %b, i32 8, i32 8, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i16> [[DPAS3]], <8 x i16>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v8"
; CHECK-VISAASM-DAG: dpas.hf8.hf8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.hf8.hf8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.hf8.hf8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.hf8.hf8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call28 = call spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32 32, i16 %a1, <8 x i32> %b, i16 %c1, i32 49164)
  store i16 %call28, i16* %res1
  %call29 = call spir_func <2 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iS_i(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x i16> %c2, i32 49164)
  store <2 x i16> %call29, <2 x i16>* %res2
  %call30 = call spir_func <4 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iS_i(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x i16> %c4, i32 49164)
  store <4 x i16> %call30, <4 x i16>* %res4
  %call31 = call spir_func <8 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS_i(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x i16> %c8, i32 49164)
  store <8 x i16> %call31, <8 x i16>* %res8

  ret void
}

define spir_kernel void @test_v12(float* %res1, <2 x float>* %res2, <4 x float>* %res4, <8 x float>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 float %c1, <2 x float> %c2, <4 x float> %c4, <8 x float> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v12(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i16.v8i32(float %c1, i16 %a1, <8 x i32> %b, i32 13, i32 13, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store float [[DPAS]], float* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.v2i16.v8i32(<2 x float> %c2, <2 x i16> %a2, <8 x i32> %b, i32 13, i32 13, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x float> [[DPAS1]], <2 x float>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %c4, <4 x i16> %a4, <8 x i32> %b, i32 13, i32 13, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x float> [[DPAS2]], <4 x float>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %c8, <8 x i16> %a8, <8 x i32> %b, i32 13, i32 13, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x float> [[DPAS3]], <8 x float>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v12"
; CHECK-VISAASM-DAG: dpas.e2m1.e2m1.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8
; CHECK-VISAASM-DAG: dpas.e2m1.e2m1.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16
; CHECK-VISAASM-DAG: dpas.e2m1.e2m1.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32
; CHECK-VISAASM-DAG: dpas.e2m1.e2m1.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64

  %call12 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_ifi(i32 64, i16 %a1, <8 x i32> %b, float %c1, i32 786432)
  store float %call12, float* %res1
  %call13 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_fi(i32 64, <2 x i16> %a2, <8 x i32> %b, <2 x float> %c2, i32 786432)
  store <2 x float> %call13, <2 x float>* %res2
  %call14 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_fi(i32 64, <4 x i16> %a4, <8 x i32> %b, <4 x float> %c4, i32 786432)
  store <4 x float> %call14, <4 x float>* %res4
  %call15 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_fi(i32 64, <8 x i16> %a8, <8 x i32> %b, <8 x float> %c8, i32 786432)
  store <8 x float> %call15, <8 x float>* %res8

  ret void
}

define spir_kernel void @test_v16(i16* %res1, <2 x i16>* %res2, <4 x i16>* %res4, <8 x i16>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i16 %c1, <2 x i16> %c2, <4 x i16> %c4, <8 x i16> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v16(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %c1, i16 %a1, <8 x i32> %b, i32 13, i32 13, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i16 [[DPAS]], i16* %res1
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i16> @llvm.genx.GenISA.sub.group.dpas.v2i16.v2i16.v2i16.v8i32(<2 x i16> %c2, <2 x i16> %a2, <8 x i32> %b, i32 13, i32 13, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i16> [[DPAS1]], <2 x i16>* %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i16> @llvm.genx.GenISA.sub.group.dpas.v4i16.v4i16.v4i16.v8i32(<4 x i16> %c4, <4 x i16> %a4, <8 x i32> %b, i32 13, i32 13, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i16> [[DPAS2]], <4 x i16>* %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16> %c8, <8 x i16> %a8, <8 x i32> %b, i32 13, i32 13, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i16> [[DPAS3]], <8 x i16>* %res8

; CHECK-VISAASM-LABEL: .kernel "test_v16"
; CHECK-VISAASM-DAG: dpas.e2m1.e2m1.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.e2m1.e2m1.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.e2m1.e2m1.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.e2m1.e2m1.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call28 = call spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32 64, i16 %a1, <8 x i32> %b, i16 %c1, i32 786444)
  store i16 %call28, i16* %res1
  %call29 = call spir_func <2 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iS_i(i32 64, <2 x i16> %a2, <8 x i32> %b, <2 x i16> %c2, i32 786444)
  store <2 x i16> %call29, <2 x i16>* %res2
  %call30 = call spir_func <4 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iS_i(i32 64, <4 x i16> %a4, <8 x i32> %b, <4 x i16> %c4, i32 786444)
  store <4 x i16> %call30, <4 x i16>* %res4
  %call31 = call spir_func <8 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS_i(i32 64, <8 x i16> %a8, <8 x i32> %b, <8 x i16> %c8, i32 786444)
  store <8 x i16> %call31, <8 x i16>* %res8

  ret void
}

!100 = !{i32 16}
