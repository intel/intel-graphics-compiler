;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_INTEL_int4,+SPV_INTEL_fp_conversions,+SPV_EXT_float8,+SPV_EXT_ocp_microscaling_types
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s

; dnscl takes raw operands without a region, so every lane reads its own element
; of each source. Uniform sources must be broadcast to a SIMD-wide variable,
; otherwise lanes other than lane 0 read whatever follows the scalar in the GRF.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

define spir_kernel void @FP16_to_Int4_uniform_seed(half addrspace(1)* %input, i32 %seed, <2 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_Int4_uniform_seed"
; CHECK: lfsr.b16v2 (M1_NM, 1) [[RAND:[A-z0-9]*]](0,0)<1>
; CHECK: mov (M1, 32) [[BIAS:[A-z0-9]*]](0,0)<1> [[RAND]](0,0)<0;1,0>
; CHECK: dnscl.hftoint4.mode0.srnd (M1, 32) {{[A-z0-9]*}}.0 {{[A-z0-9]*}}.0 {{[A-z0-9]*}}.0 [[BIAS]].0
; CHECK: // .decl [[BIAS]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr half, half addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %output, i64 %gid
  %inputVal = load half, half addrspace(1)* %inputAddr, align 2
  %conv = call i4 @_Z51__builtin_spirv_ClampStochasticRoundFP16ToInt4INTELDhi(half %inputVal, i32 %seed)
  %v1 = insertelement <2 x i4> undef, i4 %conv, i32 0
  %v2 = insertelement <2 x i4> %v1, i4 0, i32 1
  store <2 x i4> %v2, <2 x i4> addrspace(1)* %outputAddr, align 1
  ret void
}

define spir_kernel void @FP16_to_E2M1_uniform_seed(half addrspace(1)* %input, i32 %seed, <2 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_E2M1_uniform_seed"
; CHECK: lfsr.b16v2 (M1_NM, 1) [[RAND:[A-z0-9]*]](0,0)<1>
; CHECK: mov (M1, 32) [[BIAS:[A-z0-9]*]](0,0)<1> [[RAND]](0,0)<0;1,0>
; CHECK: dnscl.hftoe2m1.mode0.srnd (M1, 32) {{[A-z0-9]*}}.0 {{[A-z0-9]*}}.0 {{[A-z0-9]*}}.0 [[BIAS]].0
; CHECK: // .decl [[BIAS]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr half, half addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %output, i64 %gid
  %inputVal = load half, half addrspace(1)* %inputAddr, align 2
  %conv = call i4 @_Z46__builtin_spirv_StochasticRoundFP16ToE2M1INTELDhi(half %inputVal, i32 %seed)
  %v1 = insertelement <2 x i4> undef, i4 %conv, i32 0
  %v2 = insertelement <2 x i4> %v1, i4 0, i32 1
  store <2 x i4> %v2, <2 x i4> addrspace(1)* %outputAddr, align 1
  ret void
}

define spir_kernel void @FP16_to_Int4_uniform_input(half %input, i32 addrspace(1)* %seed, <2 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_Int4_uniform_input"
; CHECK: lfsr.b16v2 (M1, 32) [[RAND:[A-z0-9]*]](0,0)<1>
; CHECK: mov (M1, 32) [[SRC0:[A-z0-9]*]](0,0)<1> {{[A-z0-9]*}}(0,0)<0;1,0>
; CHECK: dnscl.hftoint4.mode0.srnd (M1, 32) {{[A-z0-9]*}}.0 [[SRC0]].0 {{[A-z0-9]*}}.0 [[RAND]].0
; CHECK: // .decl [[SRC0]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %output, i64 %gid
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call i4 @_Z51__builtin_spirv_ClampStochasticRoundFP16ToInt4INTELDhi(half %input, i32 %seedVal)
  %v1 = insertelement <2 x i4> undef, i4 %conv, i32 0
  %v2 = insertelement <2 x i4> %v1, i4 0, i32 1
  store <2 x i4> %v2, <2 x i4> addrspace(1)* %outputAddr, align 1
  ret void
}

define spir_kernel void @FP16_to_E5M2_uniform_seed(half addrspace(1)* %input, i32 %seed, i8 addrspace(1)* %output) {
; srnd is a regular region-based instruction: a uniform seed is read with a scalar region.
; CHECK-LABEL: .kernel "FP16_to_E5M2_uniform_seed"
; CHECK: lfsr.b8v4 (M1_NM, 1)
; CHECK: srnd (M1_NM, 32) {{[A-z0-9]*}}(0,0)<1> {{[A-z0-9]*}}(0,0)<1;1,0> {{[A-z0-9]*}}(0,0)<0;1,0>
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr half, half addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr i8, i8 addrspace(1)* %output, i64 %gid
  %inputVal = load half, half addrspace(1)* %inputAddr, align 2
  %privateSeed = alloca i32, align 4
  %conv = call spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundFP16ToE5M2INTELDhiPi(half %inputVal, i32 %seed, i32* %privateSeed)
  store i8 %conv, i8 addrspace(1)* %outputAddr, align 1
  ret void
}

declare dso_local spir_func i4 @_Z51__builtin_spirv_ClampStochasticRoundFP16ToInt4INTELDhi(half, i32)
declare dso_local spir_func i4 @_Z46__builtin_spirv_StochasticRoundFP16ToE2M1INTELDhi(half, i32)
declare spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundFP16ToE5M2INTELDhiPi(half, i32, i32*)
