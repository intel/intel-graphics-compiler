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

; CHECK-LABEL: .kernel "Test_ConvertE5M2ToFP16"
; CHECK-DAG: fcvt (M1, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=32
; CHECK-DAG: .decl [[OUT]] v_type=G type=hf num_elts=32
declare spir_func half @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELc(i8 signext)
define spir_kernel void @Test_ConvertE5M2ToFP16(i8 addrspace(1)* %inbuf, half addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %inbuf, i64 %call
  %loaded = load i8, i8 addrspace(1)* %arrayidx, align 1
  %call1 = call spir_func half @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELc(i8 signext %loaded)
  %arrayidx3 = getelementptr inbounds half, half addrspace(1)* %outbuf, i64 %call
  store half %call1, half addrspace(1)* %arrayidx3, align 2
  ret void
}

; CHECK-LABEL: .kernel "Test2_ConvertE5M2ToFP16"
; CHECK-DAG: fcvt (M1, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](1,0)<1> [[IN]](0,32)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=64
; CHECK-DAG: .decl [[OUT]] v_type=G type=hf num_elts=64
declare spir_func <2 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv2_c(<2 x i8>)
define spir_kernel void @Test2_ConvertE5M2ToFP16(<2 x i8> addrspace(1)* %inbuf, <2 x half> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %inbuf, i64 %call
  %loaded = load <2 x i8>, <2 x i8> addrspace(1)* %arrayidx, align 2
  %call1 = call spir_func <2 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv2_c(<2 x i8> %loaded)
  %arrayidx3 = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %outbuf, i64 %call
  store <2 x half> %call1, <2 x half> addrspace(1)* %arrayidx3, align 4
  ret void
}

; CHECK-LABEL: .kernel "Test3_ConvertE5M2ToFP16"
; CHECK-DAG: fcvt (M1, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](1,0)<1> [[IN]](0,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=96
; CHECK-DAG: .decl [[OUT]] v_type=G type=hf num_elts=96
declare spir_func <3 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv3_c(<3 x i8>)
define spir_kernel void @Test3_ConvertE5M2ToFP16(<3 x i8> addrspace(1)* %inbuf, <3 x half> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <3 x i8>, <3 x i8> addrspace(1)* %inbuf, i64 %call
  %inputLoaded = load <3 x i8>, <3 x i8> addrspace(1)* %arrayidx, align 4
  %call1 = call spir_func <3 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv3_c(<3 x i8> %inputLoaded)
  %arrayidx3 = getelementptr inbounds <3 x half>, <3 x half> addrspace(1)* %outbuf, i64 %call
  store <3 x half> %call1, <3 x half> addrspace(1)* %arrayidx3, align 8
  ret void
}

; CHECK-LABEL: .kernel "Test4_ConvertE5M2ToFP16"
; CHECK-DAG: fcvt (M1, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](1,0)<1> [[IN]](0,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](3,0)<1> [[IN]](1,32)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=128
; CHECK-DAG: .decl [[OUT]] v_type=G type=hf num_elts=128
declare spir_func <4 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv4_c(<4 x i8>)
define spir_kernel void @Test4_ConvertE5M2ToFP16(<4 x i8> addrspace(1)* %inbuf, <4 x half> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %inbuf, i64 %call
  %loaded = load <4 x i8>, <4 x i8> addrspace(1)* %arrayidx, align 4
  %call1 = call spir_func <4 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv4_c(<4 x i8> %loaded)
  %arrayidx3 = getelementptr inbounds <4 x half>, <4 x half> addrspace(1)* %outbuf, i64 %call
  store <4 x half> %call1, <4 x half> addrspace(1)* %arrayidx3, align 8
  ret void
}

; CHECK-LABEL: .kernel "Test8_ConvertE5M2ToFP16"
; CHECK-DAG: fcvt (M1, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](1,0)<1> [[IN]](0,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](3,0)<1> [[IN]](1,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](4,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](5,0)<1> [[IN]](2,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](6,0)<1> [[IN]](3,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](7,0)<1> [[IN]](3,32)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=256
; CHECK-DAG: .decl [[OUT]] v_type=G type=hf num_elts=256
declare spir_func <8 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv8_c(<8 x i8>)
define spir_kernel void @Test8_ConvertE5M2ToFP16(<8 x i8> addrspace(1)* %inbuf, <8 x half> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <8 x i8>, <8 x i8> addrspace(1)* %inbuf, i64 %call
  %loaded = load <8 x i8>, <8 x i8> addrspace(1)* %arrayidx, align 8
  %call1 = call spir_func <8 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv8_c(<8 x i8> %loaded)
  %arrayidx3 = getelementptr inbounds <8 x half>, <8 x half> addrspace(1)* %outbuf, i64 %call
  store <8 x half> %call1, <8 x half> addrspace(1)* %arrayidx3, align 16
  ret void
}

; CHECK-LABEL: .kernel "Test16_ConvertE5M2ToFP16"
; CHECK-DAG: fcvt (M1, 32) [[OUT:[A-z0-9]*]](0,0)<1> [[IN:[A-z0-9]*]](0,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](1,0)<1> [[IN]](0,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](2,0)<1> [[IN]](1,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](3,0)<1> [[IN]](1,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](4,0)<1> [[IN]](2,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](5,0)<1> [[IN]](2,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](6,0)<1> [[IN]](3,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](7,0)<1> [[IN]](3,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](8,0)<1> [[IN]](4,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](9,0)<1> [[IN]](4,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](10,0)<1> [[IN]](5,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](11,0)<1> [[IN]](5,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](12,0)<1> [[IN]](6,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](13,0)<1> [[IN]](6,32)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](14,0)<1> [[IN]](7,0)<1;1,0>
; CHECK-DAG: fcvt (M1, 32) [[OUT]](15,0)<1> [[IN]](7,32)<1;1,0>
; CHECK-DAG: .decl [[IN]] v_type=G type=ub num_elts=512
; CHECK-DAG: .decl [[OUT]] v_type=G type=hf num_elts=512
declare spir_func <16 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv16_c(<16 x i8>)
define spir_kernel void @Test16_ConvertE5M2ToFP16(<16 x i8> addrspace(1)* %inbuf, <16 x half> addrspace(1)* %outbuf) {
entry:
  %call = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %arrayidx = getelementptr inbounds <16 x i8>, <16 x i8> addrspace(1)* %inbuf, i64 %call
  %loaded = load <16 x i8>, <16 x i8> addrspace(1)* %arrayidx, align 16
  %call1 = call spir_func <16 x half> @_Z38__builtin_spirv_ConvertE5M2ToFP16INTELDv16_c(<16 x i8> %loaded)
  %arrayidx3 = getelementptr inbounds <16 x half>, <16 x half> addrspace(1)* %outbuf, i64 %call
  store <16 x half> %call1, <16 x half> addrspace(1)* %arrayidx3, align 32
  ret void
}
