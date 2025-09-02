;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, dg2-supported, llvm-15-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_subgroup_matrix_multiply_accumulate -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,PrintToConsole=1,PrintAfter=Layout,DisableCodeScheduling=1'" 2>&1 | FileCheck %s

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_matrix_multiply_accumulate -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'PrintToConsole=1,PrintAfter=Layout,DisableCodeScheduling=1'" 2>&1 | FileCheck %s

target triple = "spir64-unknown-unknown"

declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_iii(i32, i32, <8 x i32>, i32, i32)
declare spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iS_i(i32, <2 x i32>, <8 x i32>, <2 x i32>, i32)
declare spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iS_i(i32, <4 x i32>, <8 x i32>, <4 x i32>, i32)
declare spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_S_i(i32, <8 x i32>, <8 x i32>, <8 x i32>, i32)

declare spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_ifi(i32, i32, <8 x i32>, float, i32)
declare spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iDv2_fi(i32, <2 x i32>, <8 x i32>, <2 x float>, i32)
declare spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iDv4_fi(i32, <4 x i32>, <8 x i32>, <4 x float>, i32)
declare spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_Dv8_fi(i32, <8 x i32>, <8 x i32>, <8 x float>, i32)

define spir_kernel void @test_v1(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v1(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i32.v8i32(i32 %c1I32, i32 %a1I32, <8 x i32> %b, i32 4, i32 4, i32 8, i32 1, i1 false)
; CHECK:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i32.v8i32(<2 x i32> %c2I32, <2 x i32> %a2I32, <8 x i32> %b, i32 4, i32 4, i32 8, i32 2, i1 false)
; CHECK:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i32.v8i32(<4 x i32> %c4I32, <4 x i32> %a4I32, <8 x i32> %b, i32 4, i32 4, i32 8, i32 4, i1 false)
; CHECK:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i32.v8i32(<8 x i32> %c8I32, <8 x i32> %a8I32, <8 x i32> %b, i32 4, i32 4, i32 8, i32 8, i1 false)
; CHECK:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

  ; 8-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
  %call0 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_iii(i32 32, i32 %a1I32, <8 x i32> %b, i32 %c1I32, i32 51)
  store i32 %call0, i32* %res1I32
  %call1 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iS_i(i32 32, <2 x i32> %a2I32, <8 x i32> %b, <2 x i32> %c2I32, i32 51)
  store <2 x i32> %call1, <2 x i32>* %res2I32
  %call2 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iS_i(i32 32, <4 x i32> %a4I32, <8 x i32> %b, <4 x i32> %c4I32, i32 51)
  store <4 x i32> %call2, <4 x i32>* %res4I32
  %call3 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_S_i(i32 32, <8 x i32> %a8I32, <8 x i32> %b, <8 x i32> %c8I32, i32 51)
  store <8 x i32> %call3, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v2(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v2(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i32.v8i32(i32 %c1I32, i32 %a1I32, <8 x i32> %b, i32 4, i32 1, i32 8, i32 1, i1 false)
; CHECK:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i32.v8i32(<2 x i32> %c2I32, <2 x i32> %a2I32, <8 x i32> %b, i32 4, i32 1, i32 8, i32 2, i1 false)
; CHECK:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i32.v8i32(<4 x i32> %c4I32, <4 x i32> %a4I32, <8 x i32> %b, i32 4, i32 1, i32 8, i32 4, i1 false)
; CHECK:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i32.v8i32(<8 x i32> %c8I32, <8 x i32> %a8I32, <8 x i32> %b, i32 4, i32 1, i32 8, i32 8, i1 false)
; CHECK:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

  %call4 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_iii(i32 32, i32 %a1I32, <8 x i32> %b, i32 %c1I32, i32 49)
  store i32 %call4, i32* %res1I32
  %call5 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iS_i(i32 32, <2 x i32> %a2I32, <8 x i32> %b, <2 x i32> %c2I32, i32 49)
  store <2 x i32> %call5, <2 x i32>* %res2I32
  %call6 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iS_i(i32 32, <4 x i32> %a4I32, <8 x i32> %b, <4 x i32> %c4I32, i32 49)
  store <4 x i32> %call6, <4 x i32>* %res4I32
  %call7 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_S_i(i32 32, <8 x i32> %a8I32, <8 x i32> %b, <8 x i32> %c8I32, i32 49)
  store <8 x i32> %call7, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v3(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v3(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i32.v8i32(i32 %c1I32, i32 %a1I32, <8 x i32> %b, i32 1, i32 4, i32 8, i32 1, i1 false)
; CHECK:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i32.v8i32(<2 x i32> %c2I32, <2 x i32> %a2I32, <8 x i32> %b, i32 1, i32 4, i32 8, i32 2, i1 false)
; CHECK:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i32.v8i32(<4 x i32> %c4I32, <4 x i32> %a4I32, <8 x i32> %b, i32 1, i32 4, i32 8, i32 4, i1 false)
; CHECK:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i32.v8i32(<8 x i32> %c8I32, <8 x i32> %a8I32, <8 x i32> %b, i32 1, i32 4, i32 8, i32 8, i1 false)
; CHECK:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

  %call8 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_iii(i32 32, i32 %a1I32, <8 x i32> %b, i32 %c1I32, i32 50)
  store i32 %call8, i32* %res1I32
  %call9 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iS_i(i32 32, <2 x i32> %a2I32, <8 x i32> %b, <2 x i32> %c2I32, i32 50)
  store <2 x i32> %call9, <2 x i32>* %res2I32
  %call10 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iS_i(i32 32, <4 x i32> %a4I32, <8 x i32> %b, <4 x i32> %c4I32, i32 50)
  store <4 x i32> %call10, <4 x i32>* %res4I32
  %call11 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_S_i(i32 32, <8 x i32> %a8I32, <8 x i32> %b, <8 x i32> %c8I32, i32 50)
  store <8 x i32> %call11, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v4(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v4(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i32.v8i32(i32 %c1I32, i32 %a1I32, <8 x i32> %b, i32 1, i32 1, i32 8, i32 1, i1 false)
; CHECK:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i32.v8i32(<2 x i32> %c2I32, <2 x i32> %a2I32, <8 x i32> %b, i32 1, i32 1, i32 8, i32 2, i1 false)
; CHECK:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i32.v8i32(<4 x i32> %c4I32, <4 x i32> %a4I32, <8 x i32> %b, i32 1, i32 1, i32 8, i32 4, i1 false)
; CHECK:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i32.v8i32(<8 x i32> %c8I32, <8 x i32> %a8I32, <8 x i32> %b, i32 1, i32 1, i32 8, i32 8, i1 false)
; CHECK:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

  %call12 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_iii(i32 32, i32 %a1I32, <8 x i32> %b, i32 %c1I32, i32 48)
  store i32 %call12, i32* %res1I32
  %call13 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iS_i(i32 32, <2 x i32> %a2I32, <8 x i32> %b, <2 x i32> %c2I32, i32 48)
  store <2 x i32> %call13, <2 x i32>* %res2I32
  %call14 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iS_i(i32 32, <4 x i32> %a4I32, <8 x i32> %b, <4 x i32> %c4I32, i32 48)
  store <4 x i32> %call14, <4 x i32>* %res4I32
  %call15 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_S_i(i32 32, <8 x i32> %a8I32, <8 x i32> %b, <8 x i32> %c8I32, i32 48)
  store <8 x i32> %call15, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v5(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v5(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i32.v8i32(i32 %c1I32, i32 %a1I32, <8 x i32> %b, i32 5, i32 5, i32 8, i32 1, i1 false)
; CHECK:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i32.v8i32(<2 x i32> %c2I32, <2 x i32> %a2I32, <8 x i32> %b, i32 5, i32 5, i32 8, i32 2, i1 false)
; CHECK:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i32.v8i32(<4 x i32> %c4I32, <4 x i32> %a4I32, <8 x i32> %b, i32 5, i32 5, i32 8, i32 4, i1 false)
; CHECK:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i32.v8i32(<8 x i32> %c8I32, <8 x i32> %a8I32, <8 x i32> %b, i32 5, i32 5, i32 8, i32 8, i1 false)
; CHECK:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

  ; 4-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
  %call16 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_iii(i32 64, i32 %a1I32, <8 x i32> %b, i32 %c1I32, i32 195)
  store i32 %call16, i32* %res1I32
  %call17 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iS_i(i32 64, <2 x i32> %a2I32, <8 x i32> %b, <2 x i32> %c2I32, i32 195)
  store <2 x i32> %call17, <2 x i32>* %res2I32
  %call18 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iS_i(i32 64, <4 x i32> %a4I32, <8 x i32> %b, <4 x i32> %c4I32, i32 195)
  store <4 x i32> %call18, <4 x i32>* %res4I32
  %call19 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_S_i(i32 64, <8 x i32> %a8I32, <8 x i32> %b, <8 x i32> %c8I32, i32 195)
  store <8 x i32> %call19, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v6(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v6(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i32.v8i32(i32 %c1I32, i32 %a1I32, <8 x i32> %b, i32 5, i32 2, i32 8, i32 1, i1 false)
; CHECK:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i32.v8i32(<2 x i32> %c2I32, <2 x i32> %a2I32, <8 x i32> %b, i32 5, i32 2, i32 8, i32 2, i1 false)
; CHECK:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i32.v8i32(<4 x i32> %c4I32, <4 x i32> %a4I32, <8 x i32> %b, i32 5, i32 2, i32 8, i32 4, i1 false)
; CHECK:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i32.v8i32(<8 x i32> %c8I32, <8 x i32> %a8I32, <8 x i32> %b, i32 5, i32 2, i32 8, i32 8, i1 false)
; CHECK:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

  %call20 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_iii(i32 64, i32 %a1I32, <8 x i32> %b, i32 %c1I32, i32 193)
  store i32 %call20, i32* %res1I32
  %call21 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iS_i(i32 64, <2 x i32> %a2I32, <8 x i32> %b, <2 x i32> %c2I32, i32 193)
  store <2 x i32> %call21, <2 x i32>* %res2I32
  %call22 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iS_i(i32 64, <4 x i32> %a4I32, <8 x i32> %b, <4 x i32> %c4I32, i32 193)
  store <4 x i32> %call22, <4 x i32>* %res4I32
  %call23 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_S_i(i32 64, <8 x i32> %a8I32, <8 x i32> %b, <8 x i32> %c8I32, i32 193)
  store <8 x i32> %call23, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v7(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v7(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i32.v8i32(i32 %c1I32, i32 %a1I32, <8 x i32> %b, i32 2, i32 5, i32 8, i32 1, i1 false)
; CHECK:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i32.v8i32(<2 x i32> %c2I32, <2 x i32> %a2I32, <8 x i32> %b, i32 2, i32 5, i32 8, i32 2, i1 false)
; CHECK:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i32.v8i32(<4 x i32> %c4I32, <4 x i32> %a4I32, <8 x i32> %b, i32 2, i32 5, i32 8, i32 4, i1 false)
; CHECK:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i32.v8i32(<8 x i32> %c8I32, <8 x i32> %a8I32, <8 x i32> %b, i32 2, i32 5, i32 8, i32 8, i1 false)
; CHECK:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

  %call24 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_iii(i32 64, i32 %a1I32, <8 x i32> %b, i32 %c1I32, i32 194)
  store i32 %call24, i32* %res1I32
  %call25 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iS_i(i32 64, <2 x i32> %a2I32, <8 x i32> %b, <2 x i32> %c2I32, i32 194)
  store <2 x i32> %call25, <2 x i32>* %res2I32
  %call26 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iS_i(i32 64, <4 x i32> %a4I32, <8 x i32> %b, <4 x i32> %c4I32, i32 194)
  store <4 x i32> %call26, <4 x i32>* %res4I32
  %call27 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_S_i(i32 64, <8 x i32> %a8I32, <8 x i32> %b, <8 x i32> %c8I32, i32 194)
  store <8 x i32> %call27, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v8(i32* %res1I32, <2 x i32>* %res2I32, <4 x i32>* %res4I32, <8 x i32>* %res8I32,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 i32 %c1I32, <2 x i32> %c2I32, <4 x i32> %c4I32, <8 x i32> %c8I32) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v8(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i32.v8i32(i32 %c1I32, i32 %a1I32, <8 x i32> %b, i32 2, i32 2, i32 8, i32 1, i1 false)
; CHECK:  store i32 [[DPAS]], {{i32\*|ptr}} %res1I32
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x i32> @llvm.genx.GenISA.sub.group.dpas.v2i32.v2i32.v2i32.v8i32(<2 x i32> %c2I32, <2 x i32> %a2I32, <8 x i32> %b, i32 2, i32 2, i32 8, i32 2, i1 false)
; CHECK:  store <2 x i32> [[DPAS1]], {{<2 x i32>\*|ptr}} %res2I32
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v4i32.v8i32(<4 x i32> %c4I32, <4 x i32> %a4I32, <8 x i32> %b, i32 2, i32 2, i32 8, i32 4, i1 false)
; CHECK:  store <4 x i32> [[DPAS2]], {{<4 x i32>\*|ptr}} %res4I32
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i32.v8i32(<8 x i32> %c8I32, <8 x i32> %a8I32, <8 x i32> %b, i32 2, i32 2, i32 8, i32 8, i1 false)
; CHECK:  store <8 x i32> [[DPAS3]], {{<8 x i32>\*|ptr}} %res8I32

  %call28 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_iii(i32 64, i32 %a1I32, <8 x i32> %b, i32 %c1I32, i32 192)
  store i32 %call28, i32* %res1I32
  %call29 = call spir_func <2 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iS_i(i32 64, <2 x i32> %a2I32, <8 x i32> %b, <2 x i32> %c2I32, i32 192)
  store <2 x i32> %call29, <2 x i32>* %res2I32
  %call30 = call spir_func <4 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iS_i(i32 64, <4 x i32> %a4I32, <8 x i32> %b, <4 x i32> %c4I32, i32 192)
  store <4 x i32> %call30, <4 x i32>* %res4I32
  %call31 = call spir_func <8 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_S_i(i32 64, <8 x i32> %a8I32, <8 x i32> %b, <8 x i32> %c8I32, i32 192)
  store <8 x i32> %call31, <8 x i32>* %res8I32

  ret void
}

define spir_kernel void @test_v9(float* %resF,  <2 x float>* %res2F, <4 x float>* %res4F, <8 x float>* %res8F,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 float %cF,  <2 x float> %c2F, <4 x float> %c4F, <8 x float> %c8F) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v9(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float %cF, i32 %a1I32, <8 x i32> %b, i32 12, i32 12, i32 8, i32 1, i1 false)
; CHECK:  store float [[DPAS]], {{float\*|ptr}} %resF
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.v2i32.v8i32(<2 x float> %c2F, <2 x i32> %a2I32, <8 x i32> %b, i32 12, i32 12, i32 8, i32 2, i1 false)
; CHECK:  store <2 x float> [[DPAS1]], {{<2 x float>\*|ptr}} %res2F
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i32.v8i32(<4 x float> %c4F, <4 x i32> %a4I32, <8 x i32> %b, i32 12, i32 12, i32 8, i32 4, i1 false)
; CHECK:  store <4 x float> [[DPAS2]], {{<4 x float>\*|ptr}} %res4F
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i32.v8i32(<8 x float> %c8F, <8 x i32> %a8I32, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
; CHECK:  store <8 x float> [[DPAS3]], {{<8 x float>\*|ptr}} %res8F

  ; fp16 matrix sources, fp32 accumulator:
  %call32 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_ifi(i32 16, i32 %a1I32, <8 x i32> %b, float %cF, i32 3072)
  store float %call32, float* %resF
  %call33 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iDv2_fi(i32 16, <2 x i32> %a2I32, <8 x i32> %b, <2 x float> %c2F, i32 3072)
  store <2 x float> %call33, <2 x float>* %res2F
  %call34 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iDv4_fi(i32 16, <4 x i32> %a4I32, <8 x i32> %b, <4 x float> %c4F, i32 3072)
  store <4 x float> %call34, <4 x float>* %res4F
  %call35 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_Dv8_fi(i32 16, <8 x i32> %a8I32, <8 x i32> %b, <8 x float> %c8F, i32 3072)
  store <8 x float> %call35, <8 x float>* %res8F

  ret void
}

define spir_kernel void @test_v10(float* %resF,  <2 x float>* %res2F, <4 x float>* %res4F, <8 x float>* %res8F,
                                 i32 %a1I32, <2 x i32> %a2I32, <4 x i32> %a4I32, <8 x i32> %a8I32,
                                 <8 x i32> %b,
                                 float %cF,  <2 x float> %c2F, <4 x float> %c4F, <8 x float> %c8F) !intel_reqd_sub_group_size !100 {
entry:
; CHECK-LABEL: @test_v10(
; CHECK:  [[DPAS:%[A-z0-9]*]] = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float %cF, i32 %a1I32, <8 x i32> %b, i32 11, i32 11, i32 8, i32 1, i1 false)
; CHECK:  store float [[DPAS]], {{float\*|ptr}} %resF
; CHECK:  [[DPAS1:%[A-z0-9]*]] = call <2 x float> @llvm.genx.GenISA.sub.group.dpas.v2f32.v2f32.v2i32.v8i32(<2 x float> %c2F, <2 x i32> %a2I32, <8 x i32> %b, i32 11, i32 11, i32 8, i32 2, i1 false)
; CHECK:  store <2 x float> [[DPAS1]], {{<2 x float>\*|ptr}} %res2F
; CHECK:  [[DPAS2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i32.v8i32(<4 x float> %c4F, <4 x i32> %a4I32, <8 x i32> %b, i32 11, i32 11, i32 8, i32 4, i1 false)
; CHECK:  store <4 x float> [[DPAS2]], {{<4 x float>\*|ptr}} %res4F
; CHECK:  [[DPAS3:%[A-z0-9]*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i32.v8i32(<8 x float> %c8F, <8 x i32> %a8I32, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:  store <8 x float> [[DPAS3]], {{<8 x float>\*|ptr}} %res8F

  ; bf16 matrix sources, fp32 accumulator:
  %call36 = call spir_func float @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiiDv8_ifi(i32 16, i32 %a1I32, <8 x i32> %b, float %cF, i32 12288)
  store float %call36, float* %resF
  %call37 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_iDv8_iDv2_fi(i32 16, <2 x i32> %a2I32, <8 x i32> %b, <2 x float> %c2F, i32 12288)
  store <2 x float> %call37, <2 x float>* %res2F
  %call38 = call spir_func <4 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv4_iDv8_iDv4_fi(i32 16, <4 x i32> %a4I32, <8 x i32> %b, <4 x float> %c4F, i32 12288)
  store <4 x float> %call38, <4 x float>* %res4F
  %call39 = call spir_func <8 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv8_iS_Dv8_fi(i32 16, <8 x i32> %a8I32, <8 x i32> %b, <8 x float> %c8F, i32 12288)
  store <8 x float> %call39, <8 x float>* %res8F

  ret void
}

!100 = !{i32 8}
