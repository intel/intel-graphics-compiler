;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fp_conversions,+SPV_EXT_float8 -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

; CHECK-LABEL: .kernel "Test_ConvertFP16ToE5M2"
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=32
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=32
declare spir_func signext i8 @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDh(half)
define spir_kernel void @Test_ConvertFP16ToE5M2(half addrspace(1)* %inbuf, i8 addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds half, half addrspace(1)* %inbuf, i64 %call
  %loaded = load half, half addrspace(1)* %arrayidx, align 2
  %call1 = call spir_func signext i8 @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDh(half %loaded)
  %arrayidx3 = getelementptr inbounds i8, i8 addrspace(1)* %outbuf, i64 %call
  store i8 %call1, i8 addrspace(1)* %arrayidx3, align 1
  ret void
}

; CHECK-LABEL: .kernel "Test2_ConvertFP16ToE5M2"
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=64
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=64
declare spir_func <2 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv2_Dh(<2 x half>)
define spir_kernel void @Test2_ConvertFP16ToE5M2(<2 x half> addrspace(1)* %inbuf, <2 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %inbuf, i64 %call
  %loaded = load <2 x half>, <2 x half> addrspace(1)* %arrayidx, align 4
  %call1 = call spir_func <2 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv2_Dh(<2 x half> %loaded)
  %arrayidx3 = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %outbuf, i64 %call
  store <2 x i8> %call1, <2 x i8> addrspace(1)* %arrayidx3, align 2
  ret void
}

; CHECK-LABEL: .kernel "Test3_ConvertFP16ToE5M2"
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=96
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=96
declare spir_func <3 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv3_Dh(<3 x half>)
define spir_kernel void @Test3_ConvertFP16ToE5M2(<3 x half> addrspace(1)* %inbuf, <3 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <3 x half>, <3 x half> addrspace(1)* %inbuf, i64 %call
  %inputLoaded = load <3 x half>, <3 x half> addrspace(1)* %arrayidx, align 8
  %call1 = call spir_func <3 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv3_Dh(<3 x half> %inputLoaded)
  %arrayidx3 = getelementptr inbounds <3 x i8>, <3 x i8> addrspace(1)* %outbuf, i64 %call
  store <3 x i8> %call1, <3 x i8> addrspace(1)* %arrayidx3, align 4
  ret void
}

; CHECK-LABEL: .kernel "Test4_ConvertFP16ToE5M2"
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](1,32)<1> [[IN]](3,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=128
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=128
declare spir_func <4 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv4_Dh(<4 x half>)
define spir_kernel void @Test4_ConvertFP16ToE5M2(<4 x half> addrspace(1)* %inbuf, <4 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <4 x half>, <4 x half> addrspace(1)* %inbuf, i64 %call
  %loaded = load <4 x half>, <4 x half> addrspace(1)* %arrayidx, align 8
  %call1 = call spir_func <4 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv4_Dh(<4 x half> %loaded)
  %arrayidx3 = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %outbuf, i64 %call
  store <4 x i8> %call1, <4 x i8> addrspace(1)* %arrayidx3, align 4
  ret void
}

; CHECK-LABEL: .kernel "Test8_ConvertFP16ToE5M2"
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](1,32)<1> [[IN]](3,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](2,0)<1> [[IN]](4,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](2,32)<1> [[IN]](5,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](3,0)<1> [[IN]](6,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](3,32)<1> [[IN]](7,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=256
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=256
declare spir_func <8 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv8_Dh(<8 x half>)
define spir_kernel void @Test8_ConvertFP16ToE5M2(<8 x half> addrspace(1)* %inbuf, <8 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <8 x half>, <8 x half> addrspace(1)* %inbuf, i64 %call
  %loaded = load <8 x half>, <8 x half> addrspace(1)* %arrayidx, align 16
  %call1 = call spir_func <8 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv8_Dh(<8 x half> %loaded)
  %arrayidx3 = getelementptr inbounds <8 x i8>, <8 x i8> addrspace(1)* %outbuf, i64 %call
  store <8 x i8> %call1, <8 x i8> addrspace(1)* %arrayidx3, align 8
  ret void
}

; CHECK-LABEL: .kernel "Test16_ConvertFP16ToE5M2"
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](0,32)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](1,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](1,32)<1> [[IN]](3,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](2,0)<1> [[IN]](4,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](2,32)<1> [[IN]](5,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](3,0)<1> [[IN]](6,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](3,32)<1> [[IN]](7,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](4,0)<1> [[IN]](8,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](4,32)<1> [[IN]](9,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](5,0)<1> [[IN]](10,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](5,32)<1> [[IN]](11,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](6,0)<1> [[IN]](12,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](6,32)<1> [[IN]](13,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](7,0)<1> [[IN]](14,0)<1;1,0>
; CHECK-DAG: fcvt (M1_NM, 32) [[OUT]](7,32)<1> [[IN]](15,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=hf num_elts=512
; CHECK-DAG: .decl [[OUT]] v_type=G type=ub num_elts=512
declare spir_func <16 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv16_Dh(<16 x half>)
define spir_kernel void @Test16_ConvertFP16ToE5M2(<16 x half> addrspace(1)* %inbuf, <16 x i8> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <16 x half>, <16 x half> addrspace(1)* %inbuf, i64 %call
  %loaded = load <16 x half>, <16 x half> addrspace(1)* %arrayidx, align 32
  %call1 = call spir_func <16 x i8> @_Z38__builtin_spirv_ConvertFP16ToE5M2INTELDv16_Dh(<16 x half> %loaded)
  %arrayidx3 = getelementptr inbounds <16 x i8>, <16 x i8> addrspace(1)* %outbuf, i64 %call
  store <16 x i8> %call1, <16 x i8> addrspace(1)* %arrayidx3, align 16
  ret void
}
