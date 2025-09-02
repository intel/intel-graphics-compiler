;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, pvc-supported, regkeys, llvm-15-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_subgroup_matrix_multiply_accumulate -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintAfter=ArithmeticFuncsTranslation'" 2>&1 | FileCheck %s --check-prefix=CHECK-GENISA
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'EnableOpaquePointersBackend=1 DumpVISAASMToConsole=1'"

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_matrix_multiply_accumulate -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'PrintToConsole=1 PrintAfter=ArithmeticFuncsTranslation'" 2>&1 | FileCheck %s --check-prefix=CHECK-GENISA
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'"                                 2>&1 | FileCheck %s --check-prefix=CHECK-VISAASM

target triple = "spir64-unknown-unknown"

declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iii(i32, i16 signext, <8 x i32>, i32, i32)
declare spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_ii(i32, <2 x i16>, <8 x i32>, <2 x i32>, i32)
declare spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_ii(i32, <4 x i16>, <8 x i32>, <4 x i32>, i32)
declare spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS0_i(i32, <8 x i16>, <8 x i32>, <8 x i32>, i32)

declare spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_ifi(i32, i16 signext, <8 x i32>, float, i32)
declare spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_fi(i32, <2 x i16>, <8 x i32>, <2 x float>, i32)
declare spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_fi(i32, <4 x i16>, <8 x i32>, <4 x float>, i32)
declare spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_fi(i32, <8 x i16>, <8 x i32>, <8 x float>, i32)

declare spir_func half @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iDhi(i32, i16 signext, <8 x i32>, half, i32)
declare spir_func <2 x half> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_Dhi(i32, <2 x i16>, <8 x i32>, <2 x half>, i32)
declare spir_func <4 x half> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_Dhi(i32, <4 x i16>, <8 x i32>, <4 x half>, i32)
declare spir_func <8 x half> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_Dhi(i32, <8 x i16>, <8 x i32>, <8 x half>, i32)

declare spir_func signext i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32, i16 signext, <8 x i32>, i16 signext, i32)
declare spir_func <2 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iS_i(i32, <2 x i16>, <8 x i32>, <2 x i16>, i32)
declare spir_func <4 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iS_i(i32, <4 x i16>, <8 x i32>, <4 x i16>, i32)
declare spir_func <8 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS_i(i32, <8 x i16>, <8 x i32>, <8 x i16>, i32)

declare spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELifDv8_ffi(i32, float, <8 x float>, float, i32)
declare spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_fDv8_fS_i(i32, float, <8 x float>, <2 x float>, i32)
declare spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_fDv8_fS_i(i32, <2 x float>, <8 x float>, <4 x float>, i32)
declare spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_fS_S_i(i32, <4 x float>, <8 x float>, <8 x float>, i32)

; 8-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
define spir_kernel void @test_v1(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v1(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32 %c1I32, i16 %a1, <8 x i32> %b, i32 4, i32 4, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i16.v8i32(<2 x i32> %c2I32, <2 x i16> %a2, <8 x i32> %b, i32 4, i32 4, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i16.v8i32(<4 x i32> %c4I32, <4 x i16> %a4, <8 x i32> %b, i32 4, i32 4, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %c8I32, <8 x i16> %a8, <8 x i32> %b, i32 4, i32 4, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

; CHECK-VISAASM-LABEL: .kernel "test_v1"
; CHECK-VISAASM-DAG: dpas.s8.s8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.s8.s8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.s8.s8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.s8.s8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call0 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iii(i32 32, i16 %a1, <8 x i32> %b, i32 %c1I32, i32 51)
  store i32 %call0, i32* %res1I32
  %call1 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_ii(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x i32> %c2I32, i32 51)
  store <2 x i32> %call1, <2 x i32>* %res2I32
  %call2 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_ii(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x i32> %c4I32, i32 51)
  store <4 x i32> %call2, <4 x i32>* %res4I32
  %call3 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS0_i(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x i32> %c8I32, i32 51)
  store <8 x i32> %call3, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v2(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v2(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32 %c1I32, i16 %a1, <8 x i32> %b, i32 4, i32 1, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i16.v8i32(<2 x i32> %c2I32, <2 x i16> %a2, <8 x i32> %b, i32 4, i32 1, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i16.v8i32(<4 x i32> %c4I32, <4 x i16> %a4, <8 x i32> %b, i32 4, i32 1, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %c8I32, <8 x i16> %a8, <8 x i32> %b, i32 4, i32 1, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

; CHECK-VISAASM-LABEL: .kernel "test_v2"
; CHECK-VISAASM-DAG: dpas.u8.s8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.u8.s8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.u8.s8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.u8.s8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call4 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iii(i32 32, i16 %a1, <8 x i32> %b, i32 %c1I32, i32 49)
  store i32 %call4, i32* %res1I32
  %call5 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_ii(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x i32> %c2I32, i32 49)
  store <2 x i32> %call5, <2 x i32>* %res2I32
  %call6 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_ii(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x i32> %c4I32, i32 49)
  store <4 x i32> %call6, <4 x i32>* %res4I32
  %call7 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS0_i(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x i32> %c8I32, i32 49)
  store <8 x i32> %call7, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v3(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v3(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32 %c1I32, i16 %a1, <8 x i32> %b, i32 1, i32 4, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i16.v8i32(<2 x i32> %c2I32, <2 x i16> %a2, <8 x i32> %b, i32 1, i32 4, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i16.v8i32(<4 x i32> %c4I32, <4 x i16> %a4, <8 x i32> %b, i32 1, i32 4, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %c8I32, <8 x i16> %a8, <8 x i32> %b, i32 1, i32 4, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

; CHECK-VISAASM-LABEL: .kernel "test_v3"
; CHECK-VISAASM-DAG: dpas.s8.u8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.s8.u8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.s8.u8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.s8.u8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call8 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iii(i32 32, i16 %a1, <8 x i32> %b, i32 %c1I32, i32 50)
  store i32 %call8, i32* %res1I32
  %call9 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_ii(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x i32> %c2I32, i32 50)
  store <2 x i32> %call9, <2 x i32>* %res2I32
  %call10 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_ii(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x i32> %c4I32, i32 50)
  store <4 x i32> %call10, <4 x i32>* %res4I32
  %call11 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS0_i(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x i32> %c8I32, i32 50)
  store <8 x i32> %call11, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v4(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v4(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32 %c1I32, i16 %a1, <8 x i32> %b, i32 1, i32 1, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i16.v8i32(<2 x i32> %c2I32, <2 x i16> %a2, <8 x i32> %b, i32 1, i32 1, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i16.v8i32(<4 x i32> %c4I32, <4 x i16> %a4, <8 x i32> %b, i32 1, i32 1, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %c8I32, <8 x i16> %a8, <8 x i32> %b, i32 1, i32 1, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

; CHECK-VISAASM-LABEL: .kernel "test_v4"
; CHECK-VISAASM-DAG: dpas.u8.u8.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.u8.u8.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.u8.u8.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.u8.u8.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call12 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iii(i32 32, i16 %a1, <8 x i32> %b, i32 %c1I32, i32 48)
  store i32 %call12, i32* %res1I32
  %call13 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_ii(i32 32, <2 x i16> %a2, <8 x i32> %b, <2 x i32> %c2I32, i32 48)
  store <2 x i32> %call13, <2 x i32>* %res2I32
  %call14 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_ii(i32 32, <4 x i16> %a4, <8 x i32> %b, <4 x i32> %c4I32, i32 48)
  store <4 x i32> %call14, <4 x i32>* %res4I32
  %call15 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS0_i(i32 32, <8 x i16> %a8, <8 x i32> %b, <8 x i32> %c8I32, i32 48)
  store <8 x i32> %call15, <8 x i32>* %res8I32

  ret void
}

; 4-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
define spir_kernel void @test_v5(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v5(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32 %c1I32, i16 %a1, <8 x i32> %b, i32 5, i32 5, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i16.v8i32(<2 x i32> %c2I32, <2 x i16> %a2, <8 x i32> %b, i32 5, i32 5, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i16.v8i32(<4 x i32> %c4I32, <4 x i16> %a4, <8 x i32> %b, i32 5, i32 5, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %c8I32, <8 x i16> %a8, <8 x i32> %b, i32 5, i32 5, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

; CHECK-VISAASM-LABEL: .kernel "test_v5"
; CHECK-VISAASM-DAG: dpas.s4.s4.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.s4.s4.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.s4.s4.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.s4.s4.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call16 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iii(i32 64, i16 %a1, <8 x i32> %b, i32 %c1I32, i32 195)
  store i32 %call16, i32* %res1I32
  %call17 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_ii(i32 64, <2 x i16> %a2, <8 x i32> %b, <2 x i32> %c2I32, i32 195)
  store <2 x i32> %call17, <2 x i32>* %res2I32
  %call18 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_ii(i32 64, <4 x i16> %a4, <8 x i32> %b, <4 x i32> %c4I32, i32 195)
  store <4 x i32> %call18, <4 x i32>* %res4I32
  %call19 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS0_i(i32 64, <8 x i16> %a8, <8 x i32> %b, <8 x i32> %c8I32, i32 195)
  store <8 x i32> %call19, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v6(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v6(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32 %c1I32, i16 %a1, <8 x i32> %b, i32 5, i32 2, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i16.v8i32(<2 x i32> %c2I32, <2 x i16> %a2, <8 x i32> %b, i32 5, i32 2, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i16.v8i32(<4 x i32> %c4I32, <4 x i16> %a4, <8 x i32> %b, i32 5, i32 2, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %c8I32, <8 x i16> %a8, <8 x i32> %b, i32 5, i32 2, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

; CHECK-VISAASM-LABEL: .kernel "test_v6"
; CHECK-VISAASM-DAG: dpas.u4.s4.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.u4.s4.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.u4.s4.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.u4.s4.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call20 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iii(i32 64, i16 %a1, <8 x i32> %b, i32 %c1I32, i32 193)
  store i32 %call20, i32* %res1I32
  %call21 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_ii(i32 64, <2 x i16> %a2, <8 x i32> %b, <2 x i32> %c2I32, i32 193)
  store <2 x i32> %call21, <2 x i32>* %res2I32
  %call22 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_ii(i32 64, <4 x i16> %a4, <8 x i32> %b, <4 x i32> %c4I32, i32 193)
  store <4 x i32> %call22, <4 x i32>* %res4I32
  %call23 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS0_i(i32 64, <8 x i16> %a8, <8 x i32> %b, <8 x i32> %c8I32, i32 193)
  store <8 x i32> %call23, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v7(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v7(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32 %c1I32, i16 %a1, <8 x i32> %b, i32 2, i32 5, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i16.v8i32(<2 x i32> %c2I32, <2 x i16> %a2, <8 x i32> %b, i32 2, i32 5, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i16.v8i32(<4 x i32> %c4I32, <4 x i16> %a4, <8 x i32> %b, i32 2, i32 5, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %c8I32, <8 x i16> %a8, <8 x i32> %b, i32 2, i32 5, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

; CHECK-VISAASM-LABEL: .kernel "test_v7"
; CHECK-VISAASM-DAG: dpas.s4.u4.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.s4.u4.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.s4.u4.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.s4.u4.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call24 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iii(i32 64, i16 %a1, <8 x i32> %b, i32 %c1I32, i32 194)
  store i32 %call24, i32* %res1I32
  %call25 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_ii(i32 64, <2 x i16> %a2, <8 x i32> %b, <2 x i32> %c2I32, i32 194)
  store <2 x i32> %call25, <2 x i32>* %res2I32
  %call26 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_ii(i32 64, <4 x i16> %a4, <8 x i32> %b, <4 x i32> %c4I32, i32 194)
  store <4 x i32> %call26, <4 x i32>* %res4I32
  %call27 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS0_i(i32 64, <8 x i16> %a8, <8 x i32> %b, <8 x i32> %c8I32, i32 194)
  store <8 x i32> %call27, <8 x i32>* %res8I32

  ret void
}

; int4 matrix sources, fp32 accumulator:
define spir_kernel void @test_v8(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v8(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i16.v8i32(i32 %c1I32, i16 %a1, <8 x i32> %b, i32 2, i32 2, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i16.v8i32(<2 x i32> %c2I32, <2 x i16> %a2, <8 x i32> %b, i32 2, i32 2, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i16.v8i32(<4 x i32> %c4I32, <4 x i16> %a4, <8 x i32> %b, i32 2, i32 2, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %c8I32, <8 x i16> %a8, <8 x i32> %b, i32 2, i32 2, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

; CHECK-VISAASM-LABEL: .kernel "test_v8"
; CHECK-VISAASM-DAG: dpas.u4.u4.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=d num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.u4.u4.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=d num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.u4.u4.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=d num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.u4.u4.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call28 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iii(i32 64, i16 %a1, <8 x i32> %b, i32 %c1I32, i32 192)
  store i32 %call28, i32* %res1I32
  %call29 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_ii(i32 64, <2 x i16> %a2, <8 x i32> %b, <2 x i32> %c2I32, i32 192)
  store <2 x i32> %call29, <2 x i32>* %res2I32
  %call30 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_ii(i32 64, <4 x i16> %a4, <8 x i32> %b, <4 x i32> %c4I32, i32 192)
  store <4 x i32> %call30, <4 x i32>* %res4I32
  %call31 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS0_i(i32 64, <8 x i16> %a8, <8 x i32> %b, <8 x i32> %c8I32, i32 192)
  store <8 x i32> %call31, <8 x i32>* %res8I32

  ret void
}

; fp16 matrix sources, fp32 accumulator:
define spir_kernel void @test_v9(float* %resF,  <2 x float>* %res2, <4 x float>* %res4, <8 x float>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 float %cF,  <2 x float> %c2F, <4 x float> %c4F, <8 x float> %c8F) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v9(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i16.v8i32(float %cF, i16 %a1, <8 x i32> %b, i32 12, i32 12, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store float [[DPAS]], {{float\*|ptr}} %resF
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.v2i16.v8i32(<2 x float> %c2F, <2 x i16> %a2, <8 x i32> %b, i32 12, i32 12, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x float> [[DPAS1]], {{<2 x float>\*|ptr}} %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %c4F, <4 x i16> %a4, <8 x i32> %b, i32 12, i32 12, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x float> [[DPAS2]], {{<4 x float>\*|ptr}} %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %c8F, <8 x i16> %a8, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x float> [[DPAS3]], {{<8 x float>\*|ptr}} %res8

; CHECK-VISAASM-LABEL: .kernel "test_v9"
; CHECK-VISAASM-DAG: dpas.hf.hf.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.hf.hf.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.hf.hf.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.hf.hf.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call32 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_ifi(i32 16, i16 %a1, <8 x i32> %b, float %cF, i32 3072)
  store float %call32, float* %resF
  %call33 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_fi(i32 16, <2 x i16> %a2, <8 x i32> %b, <2 x float> %c2F, i32 3072)
  store <2 x float> %call33, <2 x float>* %res2
  %call34 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_fi(i32 16, <4 x i16> %a4, <8 x i32> %b, <4 x float> %c4F, i32 3072)
  store <4 x float> %call34, <4 x float>* %res4
  %call35 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_fi(i32 16, <8 x i16> %a8, <8 x i32> %b, <8 x float> %c8F, i32 3072)
  store <8 x float> %call35, <8 x float>* %res8

  ret void
}

; bf16 matrix sources, fp32 accumulator:
define spir_kernel void @test_v10(float* %resF,  <2 x float>* %res2, <4 x float>* %res4, <8 x float>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 float %cF,  <2 x float> %c2F, <4 x float> %c4F, <8 x float> %c8F) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v10(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i16.v8i32(float %cF, i16 %a1, <8 x i32> %b, i32 11, i32 11, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store float [[DPAS]], {{float\*|ptr}} %resF
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.v2i16.v8i32(<2 x float> %c2F, <2 x i16> %a2, <8 x i32> %b, i32 11, i32 11, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x float> [[DPAS1]], {{<2 x float>\*|ptr}} %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %c4F, <4 x i16> %a4, <8 x i32> %b, i32 11, i32 11, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x float> [[DPAS2]], {{<4 x float>\*|ptr}} %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %c8F, <8 x i16> %a8, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x float> [[DPAS3]], {{<8 x float>\*|ptr}} %res8

; CHECK-VISAASM-LABEL: .kernel "test_v10"
; CHECK-VISAASM-DAG: dpas.bf.bf.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.bf.bf.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.bf.bf.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.bf.bf.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call36 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_ifi(i32 16, i16 %a1, <8 x i32> %b, float %cF, i32 12288)
  store float %call36, float* %resF
  %call37 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_fi(i32 16, <2 x i16> %a2, <8 x i32> %b, <2 x float> %c2F, i32 12288)
  store <2 x float> %call37, <2 x float>* %res2
  %call38 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_fi(i32 16, <4 x i16> %a4, <8 x i32> %b, <4 x float> %c4F, i32 12288)
  store <4 x float> %call38, <4 x float>* %res4
  %call39 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_fi(i32 16, <8 x i16> %a8, <8 x i32> %b, <8 x float> %c8F, i32 12288)
  store <8 x float> %call39, <8 x float>* %res8

  ret void
}

; fp16 matrix sources, fp16 accumulator:
define spir_kernel void @test_v11(half* %res,  <2 x half>* %res2, <4 x half>* %res4, <8 x half>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 half %c,  <2 x half> %c2, <4 x half> %c4, <8 x half> %c8) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v11(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call half @llvm.genx.GenISA.sub.group.dpas.f16.f16.i16.v8i32(half %c, i16 %a1, <8 x i32> %b, i32 12, i32 12, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store half [[DPAS]], {{half\*|ptr}} %res
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x half> @llvm.genx.GenISA.sub.group.dpas.v2f16.v2f16.v2i16.v8i32(<2 x half> %c2, <2 x i16> %a2, <8 x i32> %b, i32 12, i32 12, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x half> [[DPAS1]], {{<2 x half>\*|ptr}} %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x half> @llvm.genx.GenISA.sub.group.dpas.v4f16.v4f16.v4i16.v8i32(<4 x half> %c4, <4 x i16> %a4, <8 x i32> %b, i32 12, i32 12, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x half> [[DPAS2]], {{<4 x half>\*|ptr}} %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x half> @llvm.genx.GenISA.sub.group.dpas.v8f16.v8f16.v8i16.v8i32(<8 x half> %c8, <8 x i16> %a8, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x half> [[DPAS3]], {{<8 x half>\*|ptr}} %res8

; CHECK-VISAASM-LABEL: .kernel "test_v11"
; CHECK-VISAASM-DAG: dpas.hf.hf.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=hf num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=hf num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.hf.hf.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=hf num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=hf num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.hf.hf.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=hf num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=hf num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.hf.hf.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=hf num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=hf num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call32 = call spir_func half @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_iDhi(i32 16, i16 %a1, <8 x i32> %b, half %c, i32 3072)
  store half %call32, half* %res
  %call33 = call spir_func <2 x half> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iDv2_Dhi(i32 16, <2 x i16> %a2, <8 x i32> %b, <2 x half> %c2, i32 3072)
  store <2 x half> %call33, <2 x half>* %res2
  %call34 = call spir_func <4 x half> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iDv4_Dhi(i32 16, <4 x i16> %a4, <8 x i32> %b, <4 x half> %c4, i32 3072)
  store <4 x half> %call34, <4 x half>* %res4
  %call35 = call spir_func <8 x half> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iDv8_Dhi(i32 16, <8 x i16> %a8, <8 x i32> %b, <8 x half> %c8, i32 3072)
  store <8 x half> %call35, <8 x half>* %res8

  ret void
}

; bf16 matrix sources, bf16 accumulator:
define spir_kernel void @test_v12(i16* %res, <2 x i16>* %res2, <4 x i16>* %res4, <8 x i16>* %res8,
                                 i16 %a1, <2 x i16> %a2, <4 x i16> %a4, <8 x i16> %a8,
                                 <8 x i32> %b,
                                 i16 %cF,  <2 x i16> %c2F, <4 x i16> %c4F, <8 x i16> %c8F) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v12(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %cF, i16 %a1, <8 x i32> %b, i32 11, i32 11, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store i16 [[DPAS]], {{i16\*|ptr}} %res
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x i16> @llvm.genx.GenISA.sub.group.dpas.v2i16.v2i16.v2i16.v8i32(<2 x i16> %c2F, <2 x i16> %a2, <8 x i32> %b, i32 11, i32 11, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x i16> [[DPAS1]], {{<2 x i16>\*|ptr}} %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x i16> @llvm.genx.GenISA.sub.group.dpas.v4i16.v4i16.v4i16.v8i32(<4 x i16> %c4F, <4 x i16> %a4, <8 x i32> %b, i32 11, i32 11, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x i16> [[DPAS2]], {{<4 x i16>\*|ptr}} %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16> %c8F, <8 x i16> %a8, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x i16> [[DPAS3]], {{<8 x i16>\*|ptr}} %res8

; CHECK-VISAASM-LABEL: .kernel "test_v12"
; CHECK-VISAASM-DAG: dpas.bf.bf.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=bf num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=8 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=w num_elts=16
; CHECK-VISAASM-DAG: dpas.bf.bf.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=bf num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=w num_elts=32
; CHECK-VISAASM-DAG: dpas.bf.bf.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=bf num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=w num_elts=64
; CHECK-VISAASM-DAG: dpas.bf.bf.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=bf num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=w num_elts=128

  %call36 = call spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32 16, i16 %a1, <8 x i32> %b, i16 %cF, i32 12300)
  store i16 %call36, i16* %res
  %call37 = call spir_func <2 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_sDv8_iS_i(i32 16, <2 x i16> %a2, <8 x i32> %b, <2 x i16> %c2F, i32 12300)
  store <2 x i16> %call37, <2 x i16>* %res2
  %call38 = call spir_func <4 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_sDv8_iS_i(i32 16, <4 x i16> %a4, <8 x i32> %b, <4 x i16> %c4F, i32 12300)
  store <4 x i16> %call38, <4 x i16>* %res4
  %call39 = call spir_func <8 x i16> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_sDv8_iS_i(i32 16, <8 x i16> %a8, <8 x i32> %b, <8 x i16> %c8F, i32 12300)
  store <8 x i16> %call39, <8 x i16>* %res8

  ret void
}

; tf32 matrix sources, fp32 accumulator:
define spir_kernel void @test_v13(float* %resF,  <2 x float>* %res2, <4 x float>* %res4, <8 x float>* %res8,
                                 float %a1, float %a2, <2 x float> %a4, <4 x float> %a8,
                                 <8 x float> %b,
                                 float %cF,  <2 x float> %c2F, <4 x float> %c4F, <8 x float> %c8F) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-GENISA-LABEL: @test_v13(
; CHECK-GENISA:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.f32.v8i32(float %cF, float %a1, <8 x i32> %{{.*}}, i32 10, i32 10, i32 8, i32 1, i1 false)
; CHECK-GENISA:  store float [[DPAS]], {{float\*|ptr}} %resF
; CHECK-GENISA:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.f32.v8i32(<2 x float> %c2F, float %a2, <8 x i32> %{{.*}}, i32 10, i32 10, i32 8, i32 2, i1 false)
; CHECK-GENISA:  store <2 x float> [[DPAS1]], {{<2 x float>\*|ptr}} %res2
; CHECK-GENISA:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v2f32.v8i32(<4 x float> %c4F, <2 x float> %a4, <8 x i32> %{{.*}}, i32 10, i32 10, i32 8, i32 4, i1 false)
; CHECK-GENISA:  store <4 x float> [[DPAS2]], {{<4 x float>\*|ptr}} %res4
; CHECK-GENISA:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v4f32.v8i32(<8 x float> %c8F, <4 x float> %a8, <8 x i32> %{{.*}}, i32 10, i32 10, i32 8, i32 8, i1 false)
; CHECK-GENISA:  store <8 x float> [[DPAS3]], {{<8 x float>\*|ptr}} %res8

; CHECK-VISAASM-LABEL: .kernel "test_v13"
; CHECK-VISAASM-DAG: dpas.tf32.tf32.8.1 (M1, 16) [[D1:[A-z0-9_]*]].0 [[C1:[A-z0-9_]*]].0 [[B1:[A-z0-9_]*]].0 [[A1:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[C1]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: .decl [[B1]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A1]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A1_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A1_ALIAS]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: dpas.tf32.tf32.8.2 (M1, 16) [[D2:[A-z0-9_]*]].0 [[C2:[A-z0-9_]*]].0 [[B2:[A-z0-9_]*]].0 [[A2:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[C2]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: .decl [[B2]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A2]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[A2_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A2_ALIAS]] v_type=G type=f num_elts=16
; CHECK-VISAASM-DAG: dpas.tf32.tf32.8.4 (M1, 16) [[D4:[A-z0-9_]*]].0 [[C4:[A-z0-9_]*]].0 [[B4:[A-z0-9_]*]].0 [[A4:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[C4]] v_type=G type=f num_elts=64
; CHECK-VISAASM-DAG: .decl [[B4]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A4]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[A4_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A4_ALIAS]] v_type=G type=f num_elts=32
; CHECK-VISAASM-DAG: dpas.tf32.tf32.8.8 (M1, 16) [[D8:[A-z0-9_]*]].0 [[C8:[A-z0-9_]*]].0 [[B8:[A-z0-9_]*]].0 [[A8:[A-z0-9_]*]](0,0)
; CHECK-VISAASM-DAG: .decl [[D8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[C8]] v_type=G type=f num_elts=128
; CHECK-VISAASM-DAG: .decl [[B8]] v_type=G type=d num_elts=128
; CHECK-VISAASM-DAG: .decl [[A8]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[A8_ALIAS:[A-z0-9_]*]], 0>
; CHECK-VISAASM-DAG: .decl [[A8_ALIAS]] v_type=G type=f num_elts=64

  %call32 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELifDv8_ffi(i32 8, float %a1, <8 x float> %b, float %cF, i32 768)
  store float %call32, float* %resF
  %call33 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_fDv8_fS_i(i32 8, float %a2, <8 x float> %b, <2 x float> %c2F, i32 768)
  store <2 x float> %call33, <2 x float>* %res2
  %call34 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_fDv8_fS_i(i32 8, <2 x float> %a4, <8 x float> %b, <4 x float> %c4F, i32 768)
  store <4 x float> %call34, <4 x float>* %res4
  %call35 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_fS_S_i(i32 8, <4 x float> %a8, <8 x float> %b, <8 x float> %c8F, i32 768)
  store <8 x float> %call35, <8 x float>* %res8

  ret void
}

!100 = !{i32 16}
