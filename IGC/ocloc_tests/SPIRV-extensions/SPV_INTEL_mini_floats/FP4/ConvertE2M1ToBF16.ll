;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_INTEL_float4,+SPV_KHR_bfloat16,+SPV_INTEL_int4
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

define spir_kernel void @E2M1_to_BF16_scalar(<2 x i4> addrspace(1)* %input, bfloat addrspace(1)* %output) {
; CHECK-LABEL: .kernel "E2M1_to_BF16_scalar"
; CHECK: mov ({{.*}}, 1) [[LUT:[A-z0-9.]*]](0,0)<1> 0x3f003f0000000000:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,1)<1> 0x3fc03fc03f803f80:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,2)<1> 0x4040404040004000:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,3)<1> 0x40c040c040804080:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,4)<1> 0xbf00bf0080008000:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,5)<1> 0xbfc0bfc0bf80bf80:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,6)<1> 0xc040c040c000c000:q
; CHECK: mov ({{.*}}, 1) [[LUT]](0,7)<1> 0xc0c0c0c0c080c080:q

; CHECK: shfl_idx4 ({{.*}}, 32) [[DST:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<4;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ub num_elts=128
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr bfloat, bfloat addrspace(1)* %output, i64 %gid
  %1 = load <2 x i4>, <2 x i4> addrspace(1)* %inputAddr, align 1
  %2 = extractelement <2 x i4> %1, i32 0
  %3 = call bfloat @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELi(i4 %2)
  store bfloat %3, bfloat addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func bfloat @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELi(i4)

define spir_kernel void @E2M1_to_BF16_vector2(<2 x i4> addrspace(1)* %input, <2 x bfloat> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "E2M1_to_BF16_vector2"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<4;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ub num_elts=128
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <2 x bfloat>, <2 x bfloat> addrspace(1)* %output, i64 %gid
  %1 = load <2 x i4>, <2 x i4> addrspace(1)* %inputAddr, align 1
  %2 = call <2 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv2_i(<2 x i4> %1)
  store <2 x bfloat> %2, <2 x bfloat> addrspace(1)* %outputAddr, align 4
  ret void
}
declare dso_local spir_func <2 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv2_i(<2 x i4>)

define spir_kernel void @E2M1_to_BF16_vector3(<4 x i4> addrspace(1)* %input, <3 x bfloat> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "E2M1_to_BF16_vector3"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST0:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST1:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ub num_elts=128
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <3 x bfloat>, <3 x bfloat> addrspace(1)* %output, i64 %gid
  %1 = load <4 x i4>, <4 x i4> addrspace(1)* %inputAddr, align 2
  %2 = shufflevector <4 x i4> %1, <4 x i4> undef, <3 x i32> <i32 0, i32 1, i32 2>
  %3 = call <3 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv3_i(<3 x i4> %2)
  store <3 x bfloat> %3, <3 x bfloat> addrspace(1)* %outputAddr, align 8
  ret void
}
declare dso_local spir_func <3 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv3_i(<3 x i4>)

define spir_kernel void @E2M1_to_BF16_vector4(<4 x i4> addrspace(1)* %input, <4 x bfloat> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "E2M1_to_BF16_vector4"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST0:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST1:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ub num_elts=128
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <4 x bfloat>, <4 x bfloat> addrspace(1)* %output, i64 %gid
  %1 = load <4 x i4>, <4 x i4> addrspace(1)* %inputAddr, align 2
  %2 = call <4 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv4_i(<4 x i4> %1)
  store <4 x bfloat> %2, <4 x bfloat> addrspace(1)* %outputAddr, align 8
  ret void
}
declare dso_local spir_func <4 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv4_i(<4 x i4>)

define spir_kernel void @E2M1_to_BF16_vector8(<8 x i4> addrspace(1)* %input, <8 x bfloat> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "E2M1_to_BF16_vector8"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST0:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST1:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST2:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST3:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST3]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ub num_elts=128
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <8 x i4>, <8 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <8 x bfloat>, <8 x bfloat> addrspace(1)* %output, i64 %gid
  %1 = load <8 x i4>, <8 x i4> addrspace(1)* %inputAddr, align 4
  %2 = call <8 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv8_i(<8 x i4> %1)
  store <8 x bfloat> %2, <8 x bfloat> addrspace(1)* %outputAddr, align 16
  ret void
}
declare dso_local spir_func <8 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv8_i(<8 x i4>)

define spir_kernel void @E2M1_to_BF16_vector16(<16 x i4> addrspace(1)* %input, <16 x bfloat> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "E2M1_to_BF16_vector16"
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST0:[A-z0-9]*]].0 [[LUT_SRC:[A-z0-9]*]](0,0)<0;16,1> [[INPUT:[A-z0-9]*]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST1:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST2:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST3:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST4:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST5:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST6:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK: shfl_idx4 ({{.*}}, 32) [[DST7:[A-z0-9]*]].0 [[LUT_SRC]](0,0)<0;16,1> [[INPUT]](0,0)<4;1,0>
; CHECK-NOT: shfl_idx4
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST3]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST4]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST5]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST6]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST7]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[LUT_SRC]] v_type=G type=ud num_elts=16
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ub num_elts=128
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <16 x i4>, <16 x i4> addrspace(1)* %input, i64 %gid
  %outputAddr = getelementptr <16 x bfloat>, <16 x bfloat> addrspace(1)* %output, i64 %gid
  %1 = load <16 x i4>, <16 x i4> addrspace(1)* %inputAddr, align 8
  %2 = call <16 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv16_i(<16 x i4> %1)
  store <16 x bfloat> %2, <16 x bfloat> addrspace(1)* %outputAddr, align 32
  ret void
}
declare dso_local spir_func <16 x bfloat> @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELDv16_i(<16 x i4>)
