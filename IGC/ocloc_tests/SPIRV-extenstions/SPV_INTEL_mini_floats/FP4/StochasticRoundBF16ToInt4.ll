;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_INTEL_int4,+SPV_KHR_bfloat16,+SPV_INTEL_fp_conversions
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

;
; Variants without pointers
;

define spir_kernel void @BF16_to_Int4_scalar(bfloat addrspace(1)* %input, i32 addrspace(1)* %seed, <2 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_scalar"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK-DAG: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 [[RAND]].0
; CHECK-DAG: mov (M1, 32) [[INPUT1]](0,0)<1> 0x0:ud
; CHECK-DAG: // .decl [[RAND]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr bfloat, bfloat addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load bfloat, bfloat addrspace(1)* %inputAddr, align 2
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call i4 @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDF16bi(bfloat %inputVal, i32 %seedVal)
  %v1 = insertelement <2 x i4> undef, i4 %conv, i32 0
  %v2 = insertelement <2 x i4> %v1, i4 0, i32 1

  store <2 x i4> %v2, <2 x i4> addrspace(1)* %outputAddr, align 1
  ret void
}
declare dso_local spir_func i4 @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDF16bi(bfloat, i32)

define spir_kernel void @BF16_to_Int4_vector2(<2 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <2 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_vector2"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK-DAG: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 [[RAND]].0
; CHECK-DAG: mov (M1, 32) [[INPUT1]](0,0)<1> 0x0:ud
; CHECK-DAG: // .decl [[RAND]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <2 x bfloat>, <2 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <2 x bfloat>, <2 x bfloat> addrspace(1)* %inputAddr, align 4
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <2 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv2_DF16bi(<2 x bfloat> %inputVal, i32 %seedVal)

  store <2 x i4> %conv, <2 x i4> addrspace(1)* %outputAddr, align 1
  ret void
}
declare dso_local spir_func <2 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv2_DF16bi(<2 x bfloat>, i32)

define spir_kernel void @BF16_to_Int4_vector3(<3 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <4 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_vector3"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 [[RAND]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <3 x bfloat>, <3 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <3 x bfloat>, <3 x bfloat> addrspace(1)* %inputAddr, align 8
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <3 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv3_DF16bi(<3 x bfloat> %inputVal, i32 %seedVal)
  %conv4 = shufflevector <3 x i4> %conv, <3 x i4> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 undef>

  store <4 x i4> %conv4, <4 x i4> addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func <3 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv3_DF16bi(<3 x bfloat>, i32)

define spir_kernel void @BF16_to_Int4_vector4(<4 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <4 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_vector4"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].128 [[RAND]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=64
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <4 x bfloat>, <4 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <4 x bfloat>, <4 x bfloat> addrspace(1)* %inputAddr, align 8
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <4 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv4_DF16bi(<4 x bfloat> %inputVal, i32 %seedVal)

  store <4 x i4> %conv, <4 x i4> addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func <4 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv4_DF16bi(<4 x bfloat>, i32)

define spir_kernel void @BF16_to_Int4_vector8(<8 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <8 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_vector8"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND0:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND1:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST0:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].256 [[RAND0]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST1:[A-z0-9]*]].0 [[INPUT]].128 [[INPUT]].384 [[RAND1]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=128
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <8 x bfloat>, <8 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <8 x i4>, <8 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <8 x bfloat>, <8 x bfloat> addrspace(1)* %inputAddr, align 16
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <8 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv8_DF16bi(<8 x bfloat> %inputVal, i32 %seedVal)

  store <8 x i4> %conv, <8 x i4> addrspace(1)* %outputAddr, align 4
  ret void
}
declare dso_local spir_func <8 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv8_DF16bi(<8 x bfloat>, i32)

define spir_kernel void @BF16_to_Int4_vector16(<16 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <16 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_vector16"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND0:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND1:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND2:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND3:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST0:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].256 [[RAND0]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST1:[A-z0-9]*]].0 [[INPUT]].128 [[INPUT]].384 [[RAND1]].0
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST2:[A-z0-9]*]].0 [[INPUT]].512 [[INPUT]].768 [[RAND2]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST3:[A-z0-9]*]].0 [[INPUT]].640 [[INPUT]].896 [[RAND3]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND3]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=256
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST3]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <16 x bfloat>, <16 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <16 x i4>, <16 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <16 x bfloat>, <16 x bfloat> addrspace(1)* %inputAddr, align 32
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <16 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv16_DF16bi(<16 x bfloat> %inputVal, i32 %seedVal)

  store <16 x i4> %conv, <16 x i4> addrspace(1)* %outputAddr, align 8
  ret void
}
declare dso_local spir_func <16 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv16_DF16bi(<16 x bfloat>, i32)

; Variant with clamping should be identical to regular one
define spir_kernel void @ClampBF16_to_Int4_vector16(<16 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <16 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "ClampBF16_to_Int4_vector16"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND0:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND1:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND2:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND3:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST0:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].256 [[RAND0]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST1:[A-z0-9]*]].0 [[INPUT]].128 [[INPUT]].384 [[RAND1]].0
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST2:[A-z0-9]*]].0 [[INPUT]].512 [[INPUT]].768 [[RAND2]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST3:[A-z0-9]*]].0 [[INPUT]].640 [[INPUT]].896 [[RAND3]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND3]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=256
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST3]] v_type=G type=ud num_elts=32
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <16 x bfloat>, <16 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <16 x i4>, <16 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <16 x bfloat>, <16 x bfloat> addrspace(1)* %inputAddr, align 32
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <16 x i4> @_Z51__builtin_spirv_ClampStochasticRoundBF16ToInt4INTELDv16_DF16bi(<16 x bfloat> %inputVal, i32 %seedVal)

  store <16 x i4> %conv, <16 x i4> addrspace(1)* %outputAddr, align 8
  ret void
}
declare dso_local spir_func <16 x i4> @_Z51__builtin_spirv_ClampStochasticRoundBF16ToInt4INTELDv16_DF16bi(<16 x bfloat>, i32)

;
; Variants with pointers
;

define spir_kernel void @BF16_to_Int4_ptr_scalar(bfloat addrspace(1)* %input, i32 addrspace(1)* %seed, <2 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_ptr_scalar"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK-DAG: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 [[RAND]].0
; CHECK-DAG: mov (M1, 32) [[INPUT1]](0,0)<1> 0x0:ud
; CHECK-DAG: // .decl [[RAND]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %seedMem = alloca i32, align 4

  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr bfloat, bfloat addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load bfloat, bfloat addrspace(1)* %inputAddr, align 2
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call i4 @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDF16biPi(bfloat %inputVal, i32 %seedVal, i32* %seedMem)
  %v1 = insertelement <2 x i4> undef, i4 %conv, i32 0
  %v2 = insertelement <2 x i4> %v1, i4 0, i32 1

  store <2 x i4> %v2, <2 x i4> addrspace(1)* %outputAddr, align 1
  %seedMemVal = load i32, i32* %seedMem, align 4
  store i32 %seedMemVal, i32 addrspace(1)* %seedAddr, align 4
  ret void
}
declare dso_local spir_func i4 @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDF16biPi(bfloat, i32, i32*)

define spir_kernel void @BF16_to_Int4_ptr_vector2(<2 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <2 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_ptr_vector2"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK-DAG: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 [[RAND]].0
; CHECK-DAG: mov (M1, 32) [[INPUT1]](0,0)<1> 0x0:ud
; CHECK-DAG: // .decl [[RAND]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %seedMem = alloca i32, align 4

  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <2 x bfloat>, <2 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <2 x i4>, <2 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <2 x bfloat>, <2 x bfloat> addrspace(1)* %inputAddr, align 4
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <2 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv2_DF16biPi(<2 x bfloat> %inputVal, i32 %seedVal, i32* %seedMem)

  store <2 x i4> %conv, <2 x i4> addrspace(1)* %outputAddr, align 1
  %seedMemVal = load i32, i32* %seedMem, align 4
  store i32 %seedMemVal, i32 addrspace(1)* %seedAddr, align 4
  ret void
}
declare dso_local spir_func <2 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv2_DF16biPi(<2 x bfloat>, i32, i32*)

define spir_kernel void @BF16_to_Int4_ptr_vector3(<3 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <4 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_ptr_vector3"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 [[RAND]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %seedMem = alloca i32, align 4

  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <3 x bfloat>, <3 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <3 x bfloat>, <3 x bfloat> addrspace(1)* %inputAddr, align 8
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <3 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv3_DF16biPi(<3 x bfloat> %inputVal, i32 %seedVal, i32* %seedMem)
  %conv4 = shufflevector <3 x i4> %conv, <3 x i4> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 undef>

  store <4 x i4> %conv4, <4 x i4> addrspace(1)* %outputAddr, align 2
  %seedMemVal = load i32, i32* %seedMem, align 4
  store i32 %seedMemVal, i32 addrspace(1)* %seedAddr, align 4
  ret void
}
declare dso_local spir_func <3 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv3_DF16biPi(<3 x bfloat>, i32, i32*)

define spir_kernel void @BF16_to_Int4_ptr_vector4(<4 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <4 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_ptr_vector4"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].128 [[RAND]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=64
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=32
  %seedMem = alloca i32, align 4

  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <4 x bfloat>, <4 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <4 x bfloat>, <4 x bfloat> addrspace(1)* %inputAddr, align 8
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <4 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv4_DF16biPi(<4 x bfloat> %inputVal, i32 %seedVal, i32* %seedMem)

  store <4 x i4> %conv, <4 x i4> addrspace(1)* %outputAddr, align 2
  %seedMemVal = load i32, i32* %seedMem, align 4
  store i32 %seedMemVal, i32 addrspace(1)* %seedAddr, align 4
  ret void
}
declare dso_local spir_func <4 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv4_DF16biPi(<4 x bfloat>, i32, i32*)

define spir_kernel void @BF16_to_Int4_ptr_vector8(<8 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <8 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_ptr_vector8"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND0:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND1:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST0:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].256 [[RAND0]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST1:[A-z0-9]*]].0 [[INPUT]].128 [[INPUT]].384 [[RAND1]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=128
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
  %seedMem = alloca i32, align 4

  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <8 x bfloat>, <8 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <8 x i4>, <8 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <8 x bfloat>, <8 x bfloat> addrspace(1)* %inputAddr, align 16
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <8 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv8_DF16biPi(<8 x bfloat> %inputVal, i32 %seedVal, i32* %seedMem)

  store <8 x i4> %conv, <8 x i4> addrspace(1)* %outputAddr, align 4
  %seedMemVal = load i32, i32* %seedMem, align 4
  store i32 %seedMemVal, i32 addrspace(1)* %seedAddr, align 4
  ret void
}
declare dso_local spir_func <8 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv8_DF16biPi(<8 x bfloat>, i32, i32*)

define spir_kernel void @BF16_to_Int4_ptr_vector16(<16 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <16 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "BF16_to_Int4_ptr_vector16"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND0:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND1:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND2:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND3:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST0:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].256 [[RAND0]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST1:[A-z0-9]*]].0 [[INPUT]].128 [[INPUT]].384 [[RAND1]].0
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST2:[A-z0-9]*]].0 [[INPUT]].512 [[INPUT]].768 [[RAND2]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST3:[A-z0-9]*]].0 [[INPUT]].640 [[INPUT]].896 [[RAND3]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND3]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=256
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST3]] v_type=G type=ud num_elts=32
  %seedMem = alloca i32, align 4

  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <16 x bfloat>, <16 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <16 x i4>, <16 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <16 x bfloat>, <16 x bfloat> addrspace(1)* %inputAddr, align 32
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <16 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv16_DF16biPi(<16 x bfloat> %inputVal, i32 %seedVal, i32* %seedMem)

  store <16 x i4> %conv, <16 x i4> addrspace(1)* %outputAddr, align 8
  %seedMemVal = load i32, i32* %seedMem, align 4
  store i32 %seedMemVal, i32 addrspace(1)* %seedAddr, align 4
  ret void
}
declare dso_local spir_func <16 x i4> @_Z46__builtin_spirv_StochasticRoundBF16ToInt4INTELDv16_DF16biPi(<16 x bfloat>, i32, i32*)

; Variant with clamping should be identical to regular one
define spir_kernel void @ClampBF16_to_Int4_ptr_vector16(<16 x bfloat> addrspace(1)* %input, i32 addrspace(1)* %seed, <16 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "ClampBF16_to_Int4_ptr_vector16"
; CHECK: lfsr.b8v4 (M1, 32) [[RAND0:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND1:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND2:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK: lfsr.b8v4 (M1, 32) [[RAND3:[A-z0-9]*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK-NOT: lfsr.b8v4
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST0:[A-z0-9]*]].0 [[INPUT:[A-z0-9]*]].0 [[INPUT]].256 [[RAND0]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST1:[A-z0-9]*]].0 [[INPUT]].128 [[INPUT]].384 [[RAND1]].0
; CHECK: dnscl.bftoint4.mode0.srnd (M1, 32) [[DST2:[A-z0-9]*]].0 [[INPUT]].512 [[INPUT]].768 [[RAND2]].0
; CHECK: dnscl.bftoint4.mode2.srnd (M1, 32) [[DST3:[A-z0-9]*]].0 [[INPUT]].640 [[INPUT]].896 [[RAND3]].0
; CHECK-NOT: dnscl.bftoint4
; CHECK-DAG: // .decl [[RAND0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[RAND3]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[INPUT]] v_type=G type=ud num_elts=256
; CHECK-DAG: // .decl [[DST0]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST1]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST2]] v_type=G type=ud num_elts=32
; CHECK-DAG: // .decl [[DST3]] v_type=G type=ud num_elts=32
  %seedMem = alloca i32, align 4

  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr <16 x bfloat>, <16 x bfloat> addrspace(1)* %input, i64 %gid
  %seedAddr = getelementptr i32, i32 addrspace(1)* %seed, i64 %gid
  %outputAddr = getelementptr <16 x i4>, <16 x i4> addrspace(1)* %output, i64 %gid

  %inputVal = load <16 x bfloat>, <16 x bfloat> addrspace(1)* %inputAddr, align 32
  %seedVal = load i32, i32 addrspace(1)* %seedAddr, align 4
  %conv = call <16 x i4> @_Z51__builtin_spirv_ClampStochasticRoundBF16ToInt4INTELDv16_DF16biPi(<16 x bfloat> %inputVal, i32 %seedVal, i32* %seedMem)

  store <16 x i4> %conv, <16 x i4> addrspace(1)* %outputAddr, align 8
  %seedMemVal = load i32, i32* %seedMem, align 4
  store i32 %seedMemVal, i32 addrspace(1)* %seedAddr, align 4
  ret void
}
declare dso_local spir_func <16 x i4> @_Z51__builtin_spirv_ClampStochasticRoundBF16ToInt4INTELDv16_DF16biPi(<16 x bfloat>, i32, i32*)
