;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_INTEL_int4
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

define spir_kernel void @FP16_to_Int4_scalar(half addrspace(1)* %input, <2 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_Int4_scalar"
; CHECK-DAG: dnscl.hftoint4.mode0.rne (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 %null.0
; CHECK-DAG: mov (M1, 32) [[INPUT1]](0,0)<1> 0x0:ud
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr half, half addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load half, half addrspace(1)* %inputAddr, align 2
  %conv = call i4 @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDh(half %inputVal)
  %v1 = insertelement <2 x i4> undef, i4 %conv, i32 0
  %v2 = insertelement <2 x i4> %v1, i4 0, i32 1

  store <2 x i4> %v2, <2 x i4> addrspace(1)* %outputAddr, align 1
  ret void
}
declare dso_local spir_func i4 @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDh(half)

define spir_kernel void @FP16_to_Int4_vector2(<2 x half> addrspace(1)* %input, <2 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_Int4_vector2"
; CHECK-DAG: dnscl.hftoint4.mode0.rne (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 %null.0
; CHECK-DAG: mov (M1, 32) [[INPUT1]](0,0)<1> 0x0:ud
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <2 x half>, <2 x half> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <2 x half>, <2 x half> addrspace(1)* %inputAddr, align 4
  %conv = call <2 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv2_Dh(<2 x half> %inputVal)

  store <2 x i4> %conv, <2 x i4> addrspace(1)* %outputAddr, align 1
  ret void
}
declare dso_local spir_func <2 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv2_Dh(<2 x half>)

define spir_kernel void @FP16_to_Int4_vector3(<3 x half> addrspace(1)* %input, <4 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_Int4_vector3"
; CHECK: dnscl.hftoint4.mode0.rne (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 %null.0
; CHECK-NOT: dnscl.hftoint4
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <3 x half>, <3 x half> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <3 x half>, <3 x half> addrspace(1)* %inputAddr, align 8
  %conv = call <3 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv3_Dh(<3 x half> %inputVal)
  %conv4 = shufflevector <3 x i4> %conv, <3 x i4> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 undef>

  store <4 x i4> %conv4, <4 x i4> addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func <3 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv3_Dh(<3 x half>)

define spir_kernel void @FP16_to_Int4_vector4(<4 x half> addrspace(1)* %input, <4 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_Int4_vector4"
; CHECK: dnscl.hftoint4.mode0.rne (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].128 %null.0
; CHECK-NOT: dnscl.hftoint4
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=64
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <4 x half>, <4 x half> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <4 x half>, <4 x half> addrspace(1)* %inputAddr, align 8
  %conv = call <4 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv4_Dh(<4 x half> %inputVal)

  store <4 x i4> %conv, <4 x i4> addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func <4 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv4_Dh(<4 x half>)

define spir_kernel void @FP16_to_Int4_vector8(<8 x half> addrspace(1)* %input, <8 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_Int4_vector8"
; CHECK: dnscl.hftoint4.mode0.rne (M1, 32) [[DST0:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].256 %null.0
; CHECK: dnscl.hftoint4.mode2.rne (M1, 32) [[DST1:[A-z0-9]*]].0 [[INPUT]].128 [[INPUT]].384 %null.0
; CHECK-NOT: dnscl.hftoint4
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=128
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <8 x half>, <8 x half> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <8 x i4>, <8 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <8 x half>, <8 x half> addrspace(1)* %inputAddr, align 16
  %conv = call <8 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv8_Dh(<8 x half> %inputVal)

  store <8 x i4> %conv, <8 x i4> addrspace(1)* %outputAddr, align 4
  ret void
}
declare dso_local spir_func <8 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv8_Dh(<8 x half>)

define spir_kernel void @FP16_to_Int4_vector16(<16 x half> addrspace(1)* %input, <16 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_Int4_vector16"
; CHECK: dnscl.hftoint4.mode0.rne (M1, 32) [[DST0:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].256 %null.0
; CHECK: dnscl.hftoint4.mode2.rne (M1, 32) [[DST1:[A-z0-9]*]].0 [[INPUT]].128 [[INPUT]].384 %null.0
; CHECK: dnscl.hftoint4.mode0.rne (M1, 32) [[DST2:[A-z0-9]*]].0 [[INPUT]].512 [[INPUT]].768 %null.0
; CHECK: dnscl.hftoint4.mode2.rne (M1, 32) [[DST3:[A-z0-9]*]].0 [[INPUT]].640 [[INPUT]].896 %null.0
; CHECK-NOT: dnscl.hftoint4
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=256
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST3]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <16 x half>, <16 x half> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <16 x i4>, <16 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <16 x half>, <16 x half> addrspace(1)* %inputAddr, align 32
  %conv = call <16 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv16_Dh(<16 x half> %inputVal)

  store <16 x i4> %conv, <16 x i4> addrspace(1)* %outputAddr, align 8
  ret void
}
declare dso_local spir_func <16 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv16_Dh(<16 x half>)
