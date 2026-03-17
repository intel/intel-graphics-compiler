;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_EXT_float8,+SPV_INTEL_int4
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

define spir_kernel void @Int4_to_E5M2_scalar(<2 x i4> addrspace(1)* %input, i8 addrspace(1)* %output) {
; CHECK-LABEL: .kernel "Int4_to_E5M2_scalar"
; CHECK: mov ({{.*}}, 1) [[LUT:[A-z0-9.]*]](0,0)<1> 0x3c3c3c3c00000000:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,1)<1> 0x4242424240404040:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,2)<1> 0x4545454544444444:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,3)<1> 0x4747474746464646:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,4)<1> 0xc7c7c7c7c8c8c8c8:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,5)<1> 0xc5c5c5c5c6c6c6c6:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,6)<1> 0xc2c2c2c2c4c4c4c4:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,7)<1> 0xbcbcbcbcc0c0c0c0:q

; CHECK: shfl_idx4 ({{.*}}, 32) [[DST:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<2;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=uw num_elts=64
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr i8, i8 addrspace(1)* %output, i64 %gid
  %1 = load <2 x i4>, <2 x i4> addrspace(1)* %inputAddr, align 1
  %2 = extractelement <2 x i4> %1, i32 0
  %3 = call i8 @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELi(i4 %2)
  store i8 %3, i8 addrspace(1)* %outputAddr, align 1
  ret void
}
declare dso_local spir_func i8 @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELi(i4)

define spir_kernel void @Int4_to_E5M2_vector2(<2 x i4> addrspace(1)* %input, <2 x i8> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "Int4_to_E5M2_vector2"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<2;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=uw num_elts=64
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <2 x i8>, <2 x i8> addrspace(1)* %output, i64 %gid
  %1 = load <2 x i4>, <2 x i4> addrspace(1)* %inputAddr, align 1
  %2 = call <2 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv2_i(<2 x i4> %1)
  store <2 x i8> %2, <2 x i8> addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func <2 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv2_i(<2 x i4>)

define spir_kernel void @Int4_to_E5M2_vector3(<4 x i4> addrspace(1)* %input, <3 x i8> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "Int4_to_E5M2_vector3"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<2;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=uw num_elts=64
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <3 x i8>, <3 x i8> addrspace(1)* %output, i64 %gid
  %1 = load <4 x i4>, <4 x i4> addrspace(1)* %inputAddr, align 2
  %2 = shufflevector <4 x i4> %1, <4 x i4> undef, <3 x i32> <i32 0, i32 1, i32 2>
  %3 = call <3 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv3_i(<3 x i4> %2)
  store <3 x i8> %3, <3 x i8> addrspace(1)* %outputAddr, align 4
  ret void
}
declare dso_local spir_func <3 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv3_i(<3 x i4>)

define spir_kernel void @Int4_to_E5M2_vector4(<4 x i4> addrspace(1)* %input, <4 x i8> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "Int4_to_E5M2_vector4"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<2;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=uw num_elts=64
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <4 x i8>, <4 x i8> addrspace(1)* %output, i64 %gid
  %1 = load <4 x i4>, <4 x i4> addrspace(1)* %inputAddr, align 2
  %2 = call <4 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv4_i(<4 x i4> %1)
  store <4 x i8> %2, <4 x i8> addrspace(1)* %outputAddr, align 4
  ret void
}
declare dso_local spir_func <4 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv4_i(<4 x i4>)

define spir_kernel void @Int4_to_E5M2_vector8(<8 x i4> addrspace(1)* %input, <8 x i8> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "Int4_to_E5M2_vector8"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<2;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<2;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=uw num_elts=64
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <8 x i4>, <8 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <8 x i8>, <8 x i8> addrspace(1)* %output, i64 %gid
  %1 = load <8 x i4>, <8 x i4> addrspace(1)* %inputAddr, align 4
  %2 = call <8 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv8_i(<8 x i4> %1)
  store <8 x i8> %2, <8 x i8> addrspace(1)* %outputAddr, align 8
  ret void
}
declare dso_local spir_func <8 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv8_i(<8 x i4>)

define spir_kernel void @Int4_to_E5M2_vector16(<16 x i4> addrspace(1)* %input, <16 x i8> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "Int4_to_E5M2_vector16"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<2;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<2;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<2;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<2;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=uw num_elts=64
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <16 x i4>, <16 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <16 x i8>, <16 x i8> addrspace(1)* %output, i64 %gid
  %1 = load <16 x i4>, <16 x i4> addrspace(1)* %inputAddr, align 8
  %2 = call <16 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv16_i(<16 x i4> %1)
  store <16 x i8> %2, <16 x i8> addrspace(1)* %outputAddr, align 16
  ret void
}
declare dso_local spir_func <16 x i8> @_Z38__builtin_spirv_ConvertInt4ToE5M2INTELDv16_i(<16 x i4>)
