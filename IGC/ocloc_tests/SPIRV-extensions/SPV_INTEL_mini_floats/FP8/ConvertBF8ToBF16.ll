;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;
; Tests E5M2 -> BF16 conversion software emulation.
; Emulation uses following conversions: E5M2 -> FP16 -> F -> BF16
;

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fp_conversions,+SPV_KHR_bfloat16,+SPV_EXT_float8 -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

; CHECK-LABEL: .kernel "Test_ConvertE5M2ToBF16"
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F:[A-z0-9]*]](0,0)<1> [[OUT_HF]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF:[A-z0-9]*]](0,0)<1> [[OUT_F]](0,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=32
; CHECK-DAG: .decl [[OUT_HF]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_F]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_BF]] v_type=G type=bf num_elts=32
declare spir_func bfloat @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELc(i8 signext)
define spir_kernel void @Test_ConvertE5M2ToBF16(i8 addrspace(1)* %inbuf, bfloat addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %inbuf, i64 %call
  %loaded = load i8, i8 addrspace(1)* %arrayidx, align 1
  %call1 = call spir_func bfloat @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELc(i8 signext %loaded)
  %arrayidx3 = getelementptr inbounds bfloat, bfloat addrspace(1)* %outbuf, i64 %call
  store bfloat %call1, bfloat addrspace(1)* %arrayidx3, align 2
  ret void
}

; CHECK-LABEL: .kernel "Test2_ConvertE5M2ToBF16"
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](1,0)<1> [[IN]](0,32)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_F0:[A-z0-9]*]](0,0)<1> [[OUT_HF]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F1:[A-z0-9]*]](0,0)<1> [[OUT_HF]](1,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F:[A-z0-9]*]](0,0)<1> [[OUT_F0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](2,0)<1> [[OUT_F1]](0,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_BF:[A-z0-9]*]](0,0)<1> [[OUT_VEC_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](1,0)<1> [[OUT_VEC_F]](2,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=64
; CHECK-DAG: .decl [[OUT_HF]] v_type=G type=hf num_elts=64
; CHECK-DAG: .decl [[OUT_F0]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F1]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_F]] v_type=G type=f num_elts=64
; CHECK-DAG: .decl [[OUT_BF]] v_type=G type=bf num_elts=64
declare spir_func <2 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv2_c(<2 x i8>)
define spir_kernel void @Test2_ConvertE5M2ToBF16(<2 x i8> addrspace(1)* %inbuf, <2 x bfloat> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %inbuf, i64 %call
  %loaded = load <2 x i8>, <2 x i8> addrspace(1)* %arrayidx, align 2
  %call1 = call spir_func <2 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv2_c(<2 x i8> %loaded)
  %arrayidx3 = getelementptr inbounds <2 x bfloat>, <2 x bfloat> addrspace(1)* %outbuf, i64 %call
  store <2 x bfloat> %call1, <2 x bfloat> addrspace(1)* %arrayidx3, align 4
  ret void
}

; CHECK-LABEL: .kernel "Test3_ConvertE5M2ToBF16"
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](1,0)<1> [[IN]](0,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](2,0)<1> [[IN]](1,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_F0:[A-z0-9]*]](0,0)<1> [[OUT_HF]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F1:[A-z0-9]*]](0,0)<1> [[OUT_HF]](1,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F2:[A-z0-9]*]](0,0)<1> [[OUT_HF]](2,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F:[A-z0-9]*]](0,0)<1> [[OUT_F0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](2,0)<1> [[OUT_F1]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](4,0)<1> [[OUT_F2]](0,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_BF:[A-z0-9]*]](0,0)<1> [[OUT_VEC_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](1,0)<1> [[OUT_VEC_F]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](2,0)<1> [[OUT_VEC_F]](4,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=96
; CHECK-DAG: .decl [[OUT_HF]] v_type=G type=hf num_elts=96
; CHECK-DAG: .decl [[OUT_F0]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F1]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F2]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_F]] v_type=G type=f num_elts=96
; CHECK-DAG: .decl [[OUT_BF]] v_type=G type=bf num_elts=96
declare spir_func <3 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv3_c(<3 x i8>)
define spir_kernel void @Test3_ConvertE5M2ToBF16(<3 x i8> addrspace(1)* %inbuf, <3 x bfloat> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <3 x i8>, <3 x i8> addrspace(1)* %inbuf, i64 %call
  %inputLoaded = load <3 x i8>, <3 x i8> addrspace(1)* %arrayidx, align 4
  %call1 = call spir_func <3 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv3_c(<3 x i8> %inputLoaded)
  %arrayidx3 = getelementptr inbounds <3 x bfloat>, <3 x bfloat> addrspace(1)* %outbuf, i64 %call
  store <3 x bfloat> %call1, <3 x bfloat> addrspace(1)* %arrayidx3, align 8
  ret void
}

; CHECK-LABEL: .kernel "Test4_ConvertE5M2ToBF16"
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](1,0)<1> [[IN]](0,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](3,0)<1> [[IN]](1,32)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_F0:[A-z0-9]*]](0,0)<1> [[OUT_HF]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F1:[A-z0-9]*]](0,0)<1> [[OUT_HF]](1,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F2:[A-z0-9]*]](0,0)<1> [[OUT_HF]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F3:[A-z0-9]*]](0,0)<1> [[OUT_HF]](3,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F:[A-z0-9]*]](0,0)<1> [[OUT_F0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](2,0)<1> [[OUT_F1]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](4,0)<1> [[OUT_F2]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](6,0)<1> [[OUT_F3]](0,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_BF:[A-z0-9]*]](0,0)<1> [[OUT_VEC_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](1,0)<1> [[OUT_VEC_F]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](2,0)<1> [[OUT_VEC_F]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](3,0)<1> [[OUT_VEC_F]](6,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=128
; CHECK-DAG: .decl [[OUT_HF]] v_type=G type=hf num_elts=128
; CHECK-DAG: .decl [[OUT_F0]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F1]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F2]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F3]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_F]] v_type=G type=f num_elts=128
; CHECK-DAG: .decl [[OUT_BF]] v_type=G type=bf num_elts=128
declare spir_func <4 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv4_c(<4 x i8>)
define spir_kernel void @Test4_ConvertE5M2ToBF16(<4 x i8> addrspace(1)* %inbuf, <4 x bfloat> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %inbuf, i64 %call
  %loaded = load <4 x i8>, <4 x i8> addrspace(1)* %arrayidx, align 4
  %call1 = call spir_func <4 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv4_c(<4 x i8> %loaded)
  %arrayidx3 = getelementptr inbounds <4 x bfloat>, <4 x bfloat> addrspace(1)* %outbuf, i64 %call
  store <4 x bfloat> %call1, <4 x bfloat> addrspace(1)* %arrayidx3, align 8
  ret void
}

; CHECK-LABEL: .kernel "Test8_ConvertE5M2ToBF16"
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](1,0)<1> [[IN]](0,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](3,0)<1> [[IN]](1,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](4,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](5,0)<1> [[IN]](2,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](6,0)<1> [[IN]](3,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](7,0)<1> [[IN]](3,32)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_F0:[A-z0-9]*]](0,0)<1> [[OUT_HF]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F1:[A-z0-9]*]](0,0)<1> [[OUT_HF]](1,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F2:[A-z0-9]*]](0,0)<1> [[OUT_HF]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F3:[A-z0-9]*]](0,0)<1> [[OUT_HF]](3,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F4:[A-z0-9]*]](0,0)<1> [[OUT_HF]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F5:[A-z0-9]*]](0,0)<1> [[OUT_HF]](5,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F6:[A-z0-9]*]](0,0)<1> [[OUT_HF]](6,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F7:[A-z0-9]*]](0,0)<1> [[OUT_HF]](7,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F:[A-z0-9]*]](0,0)<1> [[OUT_F0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](2,0)<1> [[OUT_F1]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](4,0)<1> [[OUT_F2]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](6,0)<1> [[OUT_F3]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](8,0)<1> [[OUT_F4]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](10,0)<1> [[OUT_F5]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](12,0)<1> [[OUT_F6]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](14,0)<1> [[OUT_F7]](0,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_BF:[A-z0-9]*]](0,0)<1> [[OUT_VEC_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](1,0)<1> [[OUT_VEC_F]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](2,0)<1> [[OUT_VEC_F]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](3,0)<1> [[OUT_VEC_F]](6,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](4,0)<1> [[OUT_VEC_F]](8,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](5,0)<1> [[OUT_VEC_F]](10,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](6,0)<1> [[OUT_VEC_F]](12,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](7,0)<1> [[OUT_VEC_F]](14,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=256
; CHECK-DAG: .decl [[OUT_HF]] v_type=G type=hf num_elts=256
; CHECK-DAG: .decl [[OUT_F0]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F1]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F2]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F3]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F4]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F5]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F6]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F7]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_F]] v_type=G type=f num_elts=256
; CHECK-DAG: .decl [[OUT_BF]] v_type=G type=bf num_elts=256
declare spir_func <8 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv8_c(<8 x i8>)
define spir_kernel void @Test8_ConvertE5M2ToBF16(<8 x i8> addrspace(1)* %inbuf, <8 x bfloat> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <8 x i8>, <8 x i8> addrspace(1)* %inbuf, i64 %call
  %loaded = load <8 x i8>, <8 x i8> addrspace(1)* %arrayidx, align 8
  %call1 = call spir_func <8 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv8_c(<8 x i8> %loaded)
  %arrayidx3 = getelementptr inbounds <8 x bfloat>, <8 x bfloat> addrspace(1)* %outbuf, i64 %call
  store <8 x bfloat> %call1, <8 x bfloat> addrspace(1)* %arrayidx3, align 16
  ret void
}

; CHECK-LABEL: .kernel "Test16_ConvertE5M2ToBF16"
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](1,0)<1> [[IN]](0,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](3,0)<1> [[IN]](1,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](4,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](5,0)<1> [[IN]](2,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](6,0)<1> [[IN]](3,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](7,0)<1> [[IN]](3,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](8,0)<1> [[IN]](4,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](9,0)<1> [[IN]](4,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](10,0)<1> [[IN]](5,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](11,0)<1> [[IN]](5,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](12,0)<1> [[IN]](6,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](13,0)<1> [[IN]](6,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](14,0)<1> [[IN]](7,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT_HF]](15,0)<1> [[IN]](7,32)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_F0:[A-z0-9]*]](0,0)<1> [[OUT_HF]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F1:[A-z0-9]*]](0,0)<1> [[OUT_HF]](1,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F2:[A-z0-9]*]](0,0)<1> [[OUT_HF]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F3:[A-z0-9]*]](0,0)<1> [[OUT_HF]](3,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F4:[A-z0-9]*]](0,0)<1> [[OUT_HF]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F5:[A-z0-9]*]](0,0)<1> [[OUT_HF]](5,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F6:[A-z0-9]*]](0,0)<1> [[OUT_HF]](6,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F7:[A-z0-9]*]](0,0)<1> [[OUT_HF]](7,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F8:[A-z0-9]*]](0,0)<1> [[OUT_HF]](8,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F9:[A-z0-9]*]](0,0)<1> [[OUT_HF]](9,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F10:[A-z0-9]*]](0,0)<1> [[OUT_HF]](10,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F11:[A-z0-9]*]](0,0)<1> [[OUT_HF]](11,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F12:[A-z0-9]*]](0,0)<1> [[OUT_HF]](12,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F13:[A-z0-9]*]](0,0)<1> [[OUT_HF]](13,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F14:[A-z0-9]*]](0,0)<1> [[OUT_HF]](14,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F15:[A-z0-9]*]](0,0)<1> [[OUT_HF]](15,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F:[A-z0-9]*]](0,0)<1> [[OUT_F0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](2,0)<1> [[OUT_F1]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](4,0)<1> [[OUT_F2]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](6,0)<1> [[OUT_F3]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](8,0)<1> [[OUT_F4]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](10,0)<1> [[OUT_F5]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](12,0)<1> [[OUT_F6]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](14,0)<1> [[OUT_F7]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](16,0)<1> [[OUT_F8]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](18,0)<1> [[OUT_F9]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](20,0)<1> [[OUT_F10]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](22,0)<1> [[OUT_F11]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](24,0)<1> [[OUT_F12]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](26,0)<1> [[OUT_F13]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](28,0)<1> [[OUT_F14]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_F]](30,0)<1> [[OUT_F15]](0,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_BF:[A-z0-9]*]](0,0)<1> [[OUT_VEC_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](1,0)<1> [[OUT_VEC_F]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](2,0)<1> [[OUT_VEC_F]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](3,0)<1> [[OUT_VEC_F]](6,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](4,0)<1> [[OUT_VEC_F]](8,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](5,0)<1> [[OUT_VEC_F]](10,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](6,0)<1> [[OUT_VEC_F]](12,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](7,0)<1> [[OUT_VEC_F]](14,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](8,0)<1> [[OUT_VEC_F]](16,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](9,0)<1> [[OUT_VEC_F]](18,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](10,0)<1> [[OUT_VEC_F]](20,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](11,0)<1> [[OUT_VEC_F]](22,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](12,0)<1> [[OUT_VEC_F]](24,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](13,0)<1> [[OUT_VEC_F]](26,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](14,0)<1> [[OUT_VEC_F]](28,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_BF]](15,0)<1> [[OUT_VEC_F]](30,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=512
; CHECK-DAG: .decl [[OUT_HF]] v_type=G type=hf num_elts=512
; CHECK-DAG: .decl [[OUT_F0]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F1]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F2]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F3]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F4]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F5]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F6]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F7]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F8]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F9]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F10]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F11]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F12]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F13]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F14]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_F15]] v_type=G type=f num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_F]] v_type=G type=f num_elts=512
; CHECK-DAG: .decl [[OUT_BF]] v_type=G type=bf num_elts=512
declare spir_func <16 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv16_c(<16 x i8>)
define spir_kernel void @Test16_ConvertE5M2ToBF16(<16 x i8> addrspace(1)* %inbuf, <16 x bfloat> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <16 x i8>, <16 x i8> addrspace(1)* %inbuf, i64 %call
  %loaded = load <16 x i8>, <16 x i8> addrspace(1)* %arrayidx, align 16
  %call1 = call spir_func <16 x bfloat> @_Z38__builtin_spirv_ConvertE5M2ToBF16INTELDv16_c(<16 x i8> %loaded)
  %arrayidx3 = getelementptr inbounds <16 x bfloat>, <16 x bfloat> addrspace(1)* %outbuf, i64 %call
  store <16 x bfloat> %call1, <16 x bfloat> addrspace(1)* %arrayidx3, align 32
  ret void
}
