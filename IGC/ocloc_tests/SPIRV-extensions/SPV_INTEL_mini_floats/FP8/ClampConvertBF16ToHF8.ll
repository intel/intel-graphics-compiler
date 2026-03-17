;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;
; Tests BF16 -> E5M2 saturated conversion software emulation.
; Emulation uses following conversions: BF16 -> F -> FP16 (sat)-> E5M2
;

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fp_conversions,+SPV_KHR_bfloat16,+SPV_EXT_float8 -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

; CHECK-LABEL: .kernel "Test_ClampConvertBF16ToE4M3"
; CHECK-DAG: .decl [[OUT_F:[A-z0-9]*]] v_type=G type=f num_elts=32
; CHECK-DAG: mov (M1, 32) [[OUT_F]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF:[A-z0-9]*]](0,0)<1> [[OUT_F]](0,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3:[A-z0-9]*]](0,0)<1> [[OUT_HF]](0,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=bf num_elts=32
; CHECK-DAG: .decl [[OUT_HF]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_E4M3]] v_type=G type=b num_elts=32
declare spir_func signext i8 @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDF16b(bfloat)
define spir_kernel void @Test_ClampConvertBF16ToE4M3(bfloat addrspace(1)* %inbuf, i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds bfloat, bfloat addrspace(1)* %inbuf, i64 %call
  %loaded = load bfloat, bfloat addrspace(1)* %arrayidx, align 2
  %call1 = call spir_func signext i8 @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDF16b(bfloat %loaded)
  %arrayidx3 = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %call1, i8 addrspace(1)* %arrayidx3, align 1
  ret void
}

; CHECK-LABEL: .kernel "Test2_ClampConvertBF16ToE4M3"
; CHECK-DAG: .decl [[OUT_F:[A-z0-9]*]] v_type=G type=f num_elts=64
; CHECK-DAG: mov (M1, 32) [[OUT_F]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](2,0)<1> [[IN]](1,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_HF_0:[A-z0-9]*]](0,0)<1> [[OUT_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_1:[A-z0-9]*]](0,0)<1> [[OUT_F]](2,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF:[A-z0-9]*]](0,0)<1> [[OUT_HF_0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](1,0)<1> [[OUT_HF_1]](0,0)<1;1,0>
;
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3:[A-z0-9]*]](0,0)<1> [[OUT_VEC_HF]](0,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](0,32)<1> [[OUT_VEC_HF]](1,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=bf num_elts=64
; CHECK-DAG: .decl [[OUT_HF_0]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_1]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_HF]] v_type=G type=hf num_elts=64
; CHECK-DAG: .decl [[OUT_E4M3]] v_type=G type=b num_elts=64
declare spir_func <2 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv2_DF16b(<2 x bfloat>)
define spir_kernel void @Test2_ClampConvertBF16ToE4M3(<2 x bfloat> addrspace(1)* %inbuf, <2 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <2 x bfloat>, <2 x bfloat> addrspace(1)* %inbuf, i64 %call
  %loaded = load <2 x bfloat>, <2 x bfloat> addrspace(1)* %arrayidx, align 4
  %call1 = call spir_func <2 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv2_DF16b(<2 x bfloat> %loaded)
  %arrayidx3 = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %outbuf, i64 %call
  store <2 x i8> %call1, <2 x i8> addrspace(1)* %arrayidx3, align 2
  ret void
}

; CHECK-LABEL: .kernel "Test3_ClampConvertBF16ToE4M3"
; CHECK-DAG: .decl [[OUT_F:[A-z0-9]*]] v_type=G type=f num_elts=96
; CHECK-DAG: mov (M1, 32) [[OUT_F]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](4,0)<1> [[IN]](2,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_HF_0:[A-z0-9]*]](0,0)<1> [[OUT_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_1:[A-z0-9]*]](0,0)<1> [[OUT_F]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_2:[A-z0-9]*]](0,0)<1> [[OUT_F]](4,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF:[A-z0-9]*]](0,0)<1> [[OUT_HF_0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](1,0)<1> [[OUT_HF_1]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](2,0)<1> [[OUT_HF_2]](0,0)<1;1,0>
;
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3:[A-z0-9]*]](0,0)<1> [[OUT_VEC_HF]](0,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](0,32)<1> [[OUT_VEC_HF]](1,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](1,0)<1> [[OUT_VEC_HF]](2,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=bf num_elts=96
; CHECK-DAG: .decl [[OUT_HF_0]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_1]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_2]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_HF]] v_type=G type=hf num_elts=96
; CHECK-DAG: .decl [[OUT_E4M3]] v_type=G type=b num_elts=96
declare spir_func <3 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv3_DF16b(<3 x bfloat>)
define spir_kernel void @Test3_ClampConvertBF16ToE4M3(<3 x bfloat> addrspace(1)* %inbuf, <3 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <3 x bfloat>, <3 x bfloat> addrspace(1)* %inbuf, i64 %call
  %loaded = load <3 x bfloat>, <3 x bfloat> addrspace(1)* %arrayidx, align 8
  %call1 = call spir_func <3 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv3_DF16b(<3 x bfloat> %loaded)
  %arrayidx3 = getelementptr inbounds <3 x i8>, <3 x i8> addrspace(1)* %outbuf, i64 %call
  store <3 x i8> %call1, <3 x i8> addrspace(1)* %arrayidx3, align 4
  ret void
}

; CHECK-LABEL: .kernel "Test4_ClampConvertBF16ToE4M3"
; CHECK-DAG: .decl [[OUT_F:[A-z0-9]*]] v_type=G type=f num_elts=128
; CHECK-DAG: mov (M1, 32) [[OUT_F]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](4,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](6,0)<1> [[IN]](3,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_HF_0:[A-z0-9]*]](0,0)<1> [[OUT_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_1:[A-z0-9]*]](0,0)<1> [[OUT_F]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_2:[A-z0-9]*]](0,0)<1> [[OUT_F]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_3:[A-z0-9]*]](0,0)<1> [[OUT_F]](6,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF:[A-z0-9]*]](0,0)<1> [[OUT_HF_0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](1,0)<1> [[OUT_HF_1]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](2,0)<1> [[OUT_HF_2]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](3,0)<1> [[OUT_HF_3]](0,0)<1;1,0>
;
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3:[A-z0-9]*]](0,0)<1> [[OUT_VEC_HF]](0,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](0,32)<1> [[OUT_VEC_HF]](1,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](1,0)<1> [[OUT_VEC_HF]](2,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](1,32)<1> [[OUT_VEC_HF]](3,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=bf num_elts=128
; CHECK-DAG: .decl [[OUT_HF_0]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_1]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_2]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_3]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_HF]] v_type=G type=hf num_elts=128
; CHECK-DAG: .decl [[OUT_E4M3]] v_type=G type=b num_elts=128
declare spir_func <4 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv4_DF16b(<4 x bfloat>)
define spir_kernel void @Test4_ClampConvertBF16ToE4M3(<4 x bfloat> addrspace(1)* %inbuf, <4 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <4 x bfloat>, <4 x bfloat> addrspace(1)* %inbuf, i64 %call
  %loaded = load <4 x bfloat>, <4 x bfloat> addrspace(1)* %arrayidx, align 8
  %call1 = call spir_func <4 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv4_DF16b(<4 x bfloat> %loaded)
  %arrayidx3 = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %outbuf, i64 %call
  store <4 x i8> %call1, <4 x i8> addrspace(1)* %arrayidx3, align 4
  ret void
}

; CHECK-LABEL: .kernel "Test8_ClampConvertBF16ToE4M3"
; CHECK-DAG: .decl [[OUT_F:[A-z0-9]*]] v_type=G type=f num_elts=256
; CHECK-DAG: mov (M1, 32) [[OUT_F]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](4,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](6,0)<1> [[IN]](3,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](8,0)<1> [[IN]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](10,0)<1> [[IN]](5,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](12,0)<1> [[IN]](6,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](14,0)<1> [[IN]](7,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_HF_0:[A-z0-9]*]](0,0)<1> [[OUT_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_1:[A-z0-9]*]](0,0)<1> [[OUT_F]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_2:[A-z0-9]*]](0,0)<1> [[OUT_F]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_3:[A-z0-9]*]](0,0)<1> [[OUT_F]](6,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_4:[A-z0-9]*]](0,0)<1> [[OUT_F]](8,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_5:[A-z0-9]*]](0,0)<1> [[OUT_F]](10,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_6:[A-z0-9]*]](0,0)<1> [[OUT_F]](12,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_7:[A-z0-9]*]](0,0)<1> [[OUT_F]](14,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF:[A-z0-9]*]](0,0)<1> [[OUT_HF_0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](1,0)<1> [[OUT_HF_1]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](2,0)<1> [[OUT_HF_2]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](3,0)<1> [[OUT_HF_3]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](4,0)<1> [[OUT_HF_4]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](5,0)<1> [[OUT_HF_5]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](6,0)<1> [[OUT_HF_6]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](7,0)<1> [[OUT_HF_7]](0,0)<1;1,0>
;
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3:[A-z0-9]*]](0,0)<1> [[OUT_VEC_HF]](0,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](0,32)<1> [[OUT_VEC_HF]](1,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](1,0)<1> [[OUT_VEC_HF]](2,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](1,32)<1> [[OUT_VEC_HF]](3,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](2,0)<1> [[OUT_VEC_HF]](4,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](2,32)<1> [[OUT_VEC_HF]](5,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](3,0)<1> [[OUT_VEC_HF]](6,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](3,32)<1> [[OUT_VEC_HF]](7,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=bf num_elts=256
; CHECK-DAG: .decl [[OUT_HF_0]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_1]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_2]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_3]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_4]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_5]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_6]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_7]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_HF]] v_type=G type=hf num_elts=256
; CHECK-DAG: .decl [[OUT_E4M3]] v_type=G type=b num_elts=256
declare spir_func <8 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv8_DF16b(<8 x bfloat>)
define spir_kernel void @Test8_ClampConvertBF16ToE4M3(<8 x bfloat> addrspace(1)* %inbuf, <8 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <8 x bfloat>, <8 x bfloat> addrspace(1)* %inbuf, i64 %call
  %loaded = load <8 x bfloat>, <8 x bfloat> addrspace(1)* %arrayidx, align 16
  %call1 = call spir_func <8 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv8_DF16b(<8 x bfloat> %loaded)
  %arrayidx3 = getelementptr inbounds <8 x i8>, <8 x i8> addrspace(1)* %outbuf, i64 %call
  store <8 x i8> %call1, <8 x i8> addrspace(1)* %arrayidx3, align 8
  ret void
}

; CHECK-LABEL: .kernel "Test16_ClampConvertBF16ToE4M3"
; CHECK-DAG: .decl [[OUT_F:[A-z0-9]*]] v_type=G type=f num_elts=512
; CHECK-DAG: mov (M1, 32) [[OUT_F]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](4,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](6,0)<1> [[IN]](3,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](8,0)<1> [[IN]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](10,0)<1> [[IN]](5,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](12,0)<1> [[IN]](6,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](14,0)<1> [[IN]](7,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](16,0)<1> [[IN]](8,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](18,0)<1> [[IN]](9,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](20,0)<1> [[IN]](10,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](22,0)<1> [[IN]](11,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](24,0)<1> [[IN]](12,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](26,0)<1> [[IN]](13,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](28,0)<1> [[IN]](14,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_F]](30,0)<1> [[IN]](15,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_HF_0:[A-z0-9]*]](0,0)<1> [[OUT_F]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_1:[A-z0-9]*]](0,0)<1> [[OUT_F]](2,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_2:[A-z0-9]*]](0,0)<1> [[OUT_F]](4,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_3:[A-z0-9]*]](0,0)<1> [[OUT_F]](6,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_4:[A-z0-9]*]](0,0)<1> [[OUT_F]](8,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_5:[A-z0-9]*]](0,0)<1> [[OUT_F]](10,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_6:[A-z0-9]*]](0,0)<1> [[OUT_F]](12,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_7:[A-z0-9]*]](0,0)<1> [[OUT_F]](14,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_8:[A-z0-9]*]](0,0)<1> [[OUT_F]](16,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_9:[A-z0-9]*]](0,0)<1> [[OUT_F]](18,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_10:[A-z0-9]*]](0,0)<1> [[OUT_F]](20,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_11:[A-z0-9]*]](0,0)<1> [[OUT_F]](22,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_12:[A-z0-9]*]](0,0)<1> [[OUT_F]](24,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_13:[A-z0-9]*]](0,0)<1> [[OUT_F]](26,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_14:[A-z0-9]*]](0,0)<1> [[OUT_F]](28,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_HF_15:[A-z0-9]*]](0,0)<1> [[OUT_F]](30,0)<1;1,0>
;
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF:[A-z0-9]*]](0,0)<1> [[OUT_HF_0]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](1,0)<1> [[OUT_HF_1]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](2,0)<1> [[OUT_HF_2]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](3,0)<1> [[OUT_HF_3]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](4,0)<1> [[OUT_HF_4]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](5,0)<1> [[OUT_HF_5]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](6,0)<1> [[OUT_HF_6]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](7,0)<1> [[OUT_HF_7]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](8,0)<1> [[OUT_HF_8]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](9,0)<1> [[OUT_HF_9]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](10,0)<1> [[OUT_HF_10]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](11,0)<1> [[OUT_HF_11]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](12,0)<1> [[OUT_HF_12]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](13,0)<1> [[OUT_HF_13]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](14,0)<1> [[OUT_HF_14]](0,0)<1;1,0>
; CHECK-DAG: mov (M1, 32) [[OUT_VEC_HF]](15,0)<1> [[OUT_HF_15]](0,0)<1;1,0>
;
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3:[A-z0-9]*]](0,0)<1> [[OUT_VEC_HF]](0,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](0,32)<1> [[OUT_VEC_HF]](1,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](1,0)<1> [[OUT_VEC_HF]](2,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](1,32)<1> [[OUT_VEC_HF]](3,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](2,0)<1> [[OUT_VEC_HF]](4,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](2,32)<1> [[OUT_VEC_HF]](5,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](3,0)<1> [[OUT_VEC_HF]](6,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](3,32)<1> [[OUT_VEC_HF]](7,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](4,0)<1> [[OUT_VEC_HF]](8,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](4,32)<1> [[OUT_VEC_HF]](9,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](5,0)<1> [[OUT_VEC_HF]](10,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](5,32)<1> [[OUT_VEC_HF]](11,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](6,0)<1> [[OUT_VEC_HF]](12,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](6,32)<1> [[OUT_VEC_HF]](13,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](7,0)<1> [[OUT_VEC_HF]](14,0)<1;1,0>
; CHECK-DAG: fcvt.sat (M1_NM, 32) [[OUT_E4M3]](7,32)<1> [[OUT_VEC_HF]](15,0)<1;1,0>
;
; CHECK-DAG: .decl [[IN]] v_type=G type=bf num_elts=512
; CHECK-DAG: .decl [[OUT_HF_0]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_1]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_2]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_3]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_4]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_5]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_6]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_7]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_8]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_9]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_10]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_11]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_12]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_13]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_14]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_HF_15]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT_VEC_HF]] v_type=G type=hf num_elts=512
; CHECK-DAG: .decl [[OUT_E4M3]] v_type=G type=b num_elts=512
declare spir_func <16 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv16_DF16b(<16 x bfloat>)
define spir_kernel void @Test16_ClampConvertBF16ToE4M3(<16 x bfloat> addrspace(1)* %inbuf, <16 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <16 x bfloat>, <16 x bfloat> addrspace(1)* %inbuf, i64 %call
  %loaded = load <16 x bfloat>, <16 x bfloat> addrspace(1)* %arrayidx, align 32
  %call1 = call spir_func <16 x i8> @_Z43__builtin_spirv_ClampConvertBF16ToE4M3INTELDv16_DF16b(<16 x bfloat> %loaded)
  %arrayidx3 = getelementptr inbounds <16 x i8>, <16 x i8> addrspace(1)* %outbuf, i64 %call
  store <16 x i8> %call1, <16 x i8> addrspace(1)* %arrayidx3, align 16
  ret void
}
