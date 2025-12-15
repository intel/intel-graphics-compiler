;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported
; UNSUPPORTED: legacy-translator, sys32

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_predicated_io -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'PrintToConsole=1 PrintAfter=SpvPredicatedIOResolution'" 2>&1 | FileCheck %s

target triple = "spir64-unknown-unknown"

; ===== LOAD FUNCTIONS =====
; Basic loads (3 params: ptr, predicate, default_value)
declare i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1cbc(i8 addrspace(1)*, i1, i8)
declare i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1hbh(i8 addrspace(1)*, i1, i8)
declare i16 @_Z27__spirv_PredicatedLoadINTELPU3AS1sbs(i16 addrspace(1)*, i1, i16)
declare i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibi(i32 addrspace(1)*, i1, i32)
declare i64 @_Z27__spirv_PredicatedLoadINTELPU3AS1lbl(i64 addrspace(1)*, i1, i64)
declare half @_Z27__spirv_PredicatedLoadINTELPU3AS1DhbDh(half addrspace(1)*, i1, half)
declare float @_Z27__spirv_PredicatedLoadINTELPU3AS1fbf(float addrspace(1)*, i1, float)
declare double @_Z27__spirv_PredicatedLoadINTELPU3AS1dbd(double addrspace(1)*, i1, double)

; Loads with memory operand (4 params: ptr, predicate, default_value, memop)
declare i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1cbcj(i8 addrspace(1)*, i1, i8, i32)
declare i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1hbhj(i8 addrspace(1)*, i1, i8, i32)
declare i16 @_Z27__spirv_PredicatedLoadINTELPU3AS1sbsj(i16 addrspace(1)*, i1, i16, i32)
declare i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibij(i32 addrspace(1)*, i1, i32, i32)
declare i64 @_Z27__spirv_PredicatedLoadINTELPU3AS1lblj(i64 addrspace(1)*, i1, i64, i32)
declare half @_Z27__spirv_PredicatedLoadINTELPU3AS1DhbDhj(half addrspace(1)*, i1, half, i32)
declare float @_Z27__spirv_PredicatedLoadINTELPU3AS1fbfj(float addrspace(1)*, i1, float, i32)
declare double @_Z27__spirv_PredicatedLoadINTELPU3AS1dbdj(double addrspace(1)*, i1, double, i32)

; Loads with memory operand and alignment (5 params: ptr, predicate, default_value, memop, align)
declare i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1cbcjj(i8 addrspace(1)*, i1, i8, i32, i32)
declare i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1hbhjj(i8 addrspace(1)*, i1, i8, i32, i32)
declare i16 @_Z27__spirv_PredicatedLoadINTELPU3AS1sbsjj(i16 addrspace(1)*, i1, i16, i32, i32)
declare i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibijj(i32 addrspace(1)*, i1, i32, i32, i32)
declare i64 @_Z27__spirv_PredicatedLoadINTELPU3AS1lbljj(i64 addrspace(1)*, i1, i64, i32, i32)
declare half @_Z27__spirv_PredicatedLoadINTELPU3AS1DhbDhjj(half addrspace(1)*, i1, half, i32, i32)
declare float @_Z27__spirv_PredicatedLoadINTELPU3AS1fbfjj(float addrspace(1)*, i1, float, i32, i32)
declare double @_Z27__spirv_PredicatedLoadINTELPU3AS1dbdjj(double addrspace(1)*, i1, double, i32, i32)

; Vector loads - basic
declare <4 x i8> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_cbDv4_c(<4 x i8> addrspace(1)*, i1, <4 x i8>)
declare <4 x i8> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_hbDv4_h(<4 x i8> addrspace(1)*, i1, <4 x i8>)
declare <4 x i16> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_sbDv4_s(<4 x i16> addrspace(1)*, i1, <4 x i16>)
declare <4 x i32> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_ibDv4_i(<4 x i32> addrspace(1)*, i1, <4 x i32>)
declare <4 x i64> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_lbDv4_l(<4 x i64> addrspace(1)*, i1, <4 x i64>)
declare <4 x half> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_DhbDv4_Dh(<4 x half> addrspace(1)*, i1, <4 x half>)
declare <4 x float> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_fbDv4_f(<4 x float> addrspace(1)*, i1, <4 x float>)
declare <4 x double> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_dbDv4_d(<4 x double> addrspace(1)*, i1, <4 x double>)

; Vector loads - with memory operands
declare <4 x i8> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_cbDv4_cj(<4 x i8> addrspace(1)*, i1, <4 x i8>, i32)
declare <4 x i8> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_hbDv4_hj(<4 x i8> addrspace(1)*, i1, <4 x i8>, i32)
declare <4 x i16> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_sbDv4_sj(<4 x i16> addrspace(1)*, i1, <4 x i16>, i32)
declare <4 x i32> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_ibDv4_ij(<4 x i32> addrspace(1)*, i1, <4 x i32>, i32)
declare <4 x i64> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_lbDv4_lj(<4 x i64> addrspace(1)*, i1, <4 x i64>, i32)
declare <4 x half> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_DhbDv4_Dhj(<4 x half> addrspace(1)*, i1, <4 x half>, i32)
declare <4 x float> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_fbDv4_fj(<4 x float> addrspace(1)*, i1, <4 x float>, i32)
declare <4 x double> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_dbDv4_dj(<4 x double> addrspace(1)*, i1, <4 x double>, i32)

; Vector loads - with memory operands and alignment
declare <4 x i8> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_cbDv4_cjj(<4 x i8> addrspace(1)*, i1, <4 x i8>, i32, i32)
declare <4 x i8> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_hbDv4_hjj(<4 x i8> addrspace(1)*, i1, <4 x i8>, i32, i32)
declare <4 x i16> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_sbDv4_sjj(<4 x i16> addrspace(1)*, i1, <4 x i16>, i32, i32)
declare <4 x i32> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_ibDv4_ijj(<4 x i32> addrspace(1)*, i1, <4 x i32>, i32, i32)
declare <4 x i64> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_lbDv4_ljj(<4 x i64> addrspace(1)*, i1, <4 x i64>, i32, i32)
declare <4 x half> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_DhbDv4_Dhjj(<4 x half> addrspace(1)*, i1, <4 x half>, i32, i32)
declare <4 x float> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_fbDv4_fjj(<4 x float> addrspace(1)*, i1, <4 x float>, i32, i32)
declare <4 x double> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_dbDv4_djj(<4 x double> addrspace(1)*, i1, <4 x double>, i32, i32)

; ===== STORE FUNCTIONS =====
; Basic stores (3 params: ptr, value, predicate)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ccb(i8 addrspace(1)*, i8, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1hhb(i8 addrspace(1)*, i8, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ssb(i16 addrspace(1)*, i16, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1iib(i32 addrspace(1)*, i32, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1llb(i64 addrspace(1)*, i64, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1DhDhb(half addrspace(1)*, half, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ffb(float addrspace(1)*, float, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ddb(double addrspace(1)*, double, i1)

; Stores with memory operand (4 params: ptr, value, predicate, memop)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ccbj(i8 addrspace(1)*, i8, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1hhbj(i8 addrspace(1)*, i8, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ssbj(i16 addrspace(1)*, i16, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1iibj(i32 addrspace(1)*, i32, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1llbj(i64 addrspace(1)*, i64, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1DhDhbj(half addrspace(1)*, half, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ffbj(float addrspace(1)*, float, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ddbj(double addrspace(1)*, double, i1, i32)

; Stores with memory operand and alignment (5 params: ptr, value, predicate, memop, align)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ccbjj(i8 addrspace(1)*, i8, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1hhbjj(i8 addrspace(1)*, i8, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ssbjj(i16 addrspace(1)*, i16, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1iibjj(i32 addrspace(1)*, i32, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1llbjj(i64 addrspace(1)*, i64, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1DhDhbjj(half addrspace(1)*, half, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ffbjj(float addrspace(1)*, float, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1ddbjj(double addrspace(1)*, double, i1, i32, i32)

; Vector stores - basic
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_cDv4_cb(<4 x i8> addrspace(1)*, <4 x i8>, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_hDv4_hb(<4 x i8> addrspace(1)*, <4 x i8>, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_sDv4_sb(<4 x i16> addrspace(1)*, <4 x i16>, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_iDv4_ib(<4 x i32> addrspace(1)*, <4 x i32>, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_lDv4_lb(<4 x i64> addrspace(1)*, <4 x i64>, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_DhDv4_Dhb(<4 x half> addrspace(1)*, <4 x half>, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_fDv4_fb(<4 x float> addrspace(1)*, <4 x float>, i1)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_dDv4_db(<4 x double> addrspace(1)*, <4 x double>, i1)

; Vector stores - with memory operands
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_cDv4_cbj(<4 x i8> addrspace(1)*, <4 x i8>, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_hDv4_hbj(<4 x i8> addrspace(1)*, <4 x i8>, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_sDv4_sbj(<4 x i16> addrspace(1)*, <4 x i16>, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_iDv4_ibj(<4 x i32> addrspace(1)*, <4 x i32>, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_lDv4_lbj(<4 x i64> addrspace(1)*, <4 x i64>, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_DhDv4_Dhbj(<4 x half> addrspace(1)*, <4 x half>, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_fDv4_fbj(<4 x float> addrspace(1)*, <4 x float>, i1, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_dDv4_dbj(<4 x double> addrspace(1)*, <4 x double>, i1, i32)

; Vector stores - with memory operands and alignment
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_cDv4_cbjj(<4 x i8> addrspace(1)*, <4 x i8>, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_hDv4_hbjj(<4 x i8> addrspace(1)*, <4 x i8>, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_sDv4_sbjj(<4 x i16> addrspace(1)*, <4 x i16>, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_iDv4_ibjj(<4 x i32> addrspace(1)*, <4 x i32>, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_lDv4_lbjj(<4 x i64> addrspace(1)*, <4 x i64>, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_DhDv4_Dhbjj(<4 x half> addrspace(1)*, <4 x half>, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_fDv4_fbjj(<4 x float> addrspace(1)*, <4 x float>, i1, i32, i32)
declare void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_dDv4_dbjj(<4 x double> addrspace(1)*, <4 x double>, i1, i32, i32)

; CHECK: define spir_kernel void @test_all_predicated_options
define spir_kernel void @test_all_predicated_options(
    i8 addrspace(1)* %ptr_i8, i16 addrspace(1)* %ptr_i16, i32 addrspace(1)* %ptr_i32, i64 addrspace(1)* %ptr_i64,
    half addrspace(1)* %ptr_half, float addrspace(1)* %ptr_float, double addrspace(1)* %ptr_double,
    <4 x i8> addrspace(1)* %ptr_v4i8, <4 x i16> addrspace(1)* %ptr_v4i16, <4 x i32> addrspace(1)* %ptr_v4i32, <4 x i64> addrspace(1)* %ptr_v4i64,
    <4 x half> addrspace(1)* %ptr_v4half, <4 x float> addrspace(1)* %ptr_v4float, <4 x double> addrspace(1)* %ptr_v4double,
    i1 %pred) {
entry:

  ; ===== SCALAR LOADS - BASIC =====
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_char = call i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1cbc(i8 addrspace(1)* %ptr_i8, i1 %pred, i8 0)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_uchar = call i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1hbh(i8 addrspace(1)* %ptr_i8, i1 %pred, i8 0)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_short = call i16 @_Z27__spirv_PredicatedLoadINTELPU3AS1sbs(i16 addrspace(1)* %ptr_i16, i1 %pred, i16 0)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_int = call i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibi(i32 addrspace(1)* %ptr_i32, i1 %pred, i32 0)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_long = call i64 @_Z27__spirv_PredicatedLoadINTELPU3AS1lbl(i64 addrspace(1)* %ptr_i64, i1 %pred, i64 0)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_half = call half @_Z27__spirv_PredicatedLoadINTELPU3AS1DhbDh(half addrspace(1)* %ptr_half, i1 %pred, half 0xH0000)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_float = call float @_Z27__spirv_PredicatedLoadINTELPU3AS1fbf(float addrspace(1)* %ptr_float, i1 %pred, float 0.0)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_double = call double @_Z27__spirv_PredicatedLoadINTELPU3AS1dbd(double addrspace(1)* %ptr_double, i1 %pred, double 0.0)

  ; ===== SCALAR LOADS - WITH MEMORY OPERANDS =====
  ; None (0)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_i8_none = call i8 @_Z27__spirv_PredicatedLoadINTELPU3AS1cbcj(i8 addrspace(1)* %ptr_i8, i1 %pred, i8 0, i32 0)

  ; Aligned (2) - with alignment
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_i32_aligned = call i32 @_Z27__spirv_PredicatedLoadINTELPU3AS1ibijj(i32 addrspace(1)* %ptr_i32, i1 %pred, i32 0, i32 2, i32 32)

  ; NonTemporal (4)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_i64_nontemporal = call i64 @_Z27__spirv_PredicatedLoadINTELPU3AS1lblj(i64 addrspace(1)* %ptr_i64, i1 %pred, i64 0, i32 4)

  ; Aligned + NonTemporal (6)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_i16_aligned_nontemp = call i16 @_Z27__spirv_PredicatedLoadINTELPU3AS1sbsjj(i16 addrspace(1)* %ptr_i16, i1 %pred, i16 0, i32 6, i32 16)

  ; Only Aligned (2) for half
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_half_aligned = call half @_Z27__spirv_PredicatedLoadINTELPU3AS1DhbDhjj(half addrspace(1)* %ptr_half, i1 %pred, half 0xH0000, i32 2, i32 16)

  ; ===== VECTOR LOADS - BASIC =====
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4char = call <4 x i8> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_cbDv4_c(<4 x i8> addrspace(1)* %ptr_v4i8, i1 %pred, <4 x i8> zeroinitializer)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4uchar = call <4 x i8> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_hbDv4_h(<4 x i8> addrspace(1)* %ptr_v4i8, i1 %pred, <4 x i8> zeroinitializer)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4short = call <4 x i16> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_sbDv4_s(<4 x i16> addrspace(1)* %ptr_v4i16, i1 %pred, <4 x i16> zeroinitializer)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4int = call <4 x i32> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_ibDv4_i(<4 x i32> addrspace(1)* %ptr_v4i32, i1 %pred, <4 x i32> zeroinitializer)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4long = call <4 x i64> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_lbDv4_l(<4 x i64> addrspace(1)* %ptr_v4i64, i1 %pred, <4 x i64> zeroinitializer)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4half = call <4 x half> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_DhbDv4_Dh(<4 x half> addrspace(1)* %ptr_v4half, i1 %pred, <4 x half> zeroinitializer)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4float = call <4 x float> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_fbDv4_f(<4 x float> addrspace(1)* %ptr_v4float, i1 %pred, <4 x float> zeroinitializer)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4double = call <4 x double> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_dbDv4_d(<4 x double> addrspace(1)* %ptr_v4double, i1 %pred, <4 x double> zeroinitializer)

  ; ===== VECTOR LOADS - WITH MEMORY OPERANDS =====
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4i8_nontemp = call <4 x i8> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_cbDv4_cj(<4 x i8> addrspace(1)* %ptr_v4i8, i1 %pred, <4 x i8> zeroinitializer, i32 4)
  ; CHECK: call {{.*}} @llvm.genx.GenISA.PredicatedLoad.{{.*}}({{.*}})
  %load_v4float_aligned = call <4 x float> @_Z27__spirv_PredicatedLoadINTELPU3AS1Dv4_fbDv4_fjj(<4 x float> addrspace(1)* %ptr_v4float, i1 %pred, <4 x float> zeroinitializer, i32 2, i32 64)

  ; ===== SCALAR STORES - BASIC =====
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1ccb(i8 addrspace(1)* %ptr_i8, i8 %load_char, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1hhb(i8 addrspace(1)* %ptr_i8, i8 %load_uchar, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1ssb(i16 addrspace(1)* %ptr_i16, i16 %load_short, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1iib(i32 addrspace(1)* %ptr_i32, i32 %load_int, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1llb(i64 addrspace(1)* %ptr_i64, i64 %load_long, i1 %pred)

  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1ffb(float addrspace(1)* %ptr_float, float %load_float, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1ddb(double addrspace(1)* %ptr_double, double %load_double, i1 %pred)

  ; ===== SCALAR STORES - WITH MEMORY OPERANDS =====
  ; None (0)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1ccbj(i8 addrspace(1)* %ptr_i8, i8 %load_i8_none, i1 %pred, i32 0)

  ; Aligned (2)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1iibjj(i32 addrspace(1)* %ptr_i32, i32 %load_i32_aligned, i1 %pred, i32 2, i32 32)

  ; NonTemporal (4)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1llbj(i64 addrspace(1)* %ptr_i64, i64 %load_i64_nontemporal, i1 %pred, i32 4)

  ; Aligned + NonTemporal (6)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1ssbjj(i16 addrspace(1)* %ptr_i16, i16 %load_i16_aligned_nontemp, i1 %pred, i32 6, i32 16)

  ; ===== VECTOR STORES - BASIC =====
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_cDv4_cb(<4 x i8> addrspace(1)* %ptr_v4i8, <4 x i8> %load_v4char, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_hDv4_hb(<4 x i8> addrspace(1)* %ptr_v4i8, <4 x i8> %load_v4uchar, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_sDv4_sb(<4 x i16> addrspace(1)* %ptr_v4i16, <4 x i16> %load_v4short, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_iDv4_ib(<4 x i32> addrspace(1)* %ptr_v4i32, <4 x i32> %load_v4int, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_lDv4_lb(<4 x i64> addrspace(1)* %ptr_v4i64, <4 x i64> %load_v4long, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_DhDv4_Dhb(<4 x half> addrspace(1)* %ptr_v4half, <4 x half> %load_v4half, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_fDv4_fb(<4 x float> addrspace(1)* %ptr_v4float, <4 x float> %load_v4float, i1 %pred)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_dDv4_db(<4 x double> addrspace(1)* %ptr_v4double, <4 x double> %load_v4double, i1 %pred)

  ; ===== VECTOR STORES - WITH MEMORY OPERANDS =====
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_cDv4_cbj(<4 x i8> addrspace(1)* %ptr_v4i8, <4 x i8> %load_v4i8_nontemp, i1 %pred, i32 4)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}({{.*}})
  call void @_Z28__spirv_PredicatedStoreINTELPU3AS1Dv4_fDv4_fbjj(<4 x float> addrspace(1)* %ptr_v4float, <4 x float> %load_v4float_aligned, i1 %pred, i32 2, i32 64)

  ret void
}

!opencl.ocl.version = !{!0}
!0 = !{i32 2, i32 0}