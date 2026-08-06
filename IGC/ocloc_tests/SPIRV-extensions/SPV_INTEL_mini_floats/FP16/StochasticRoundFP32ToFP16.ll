;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_INTEL_fp_conversions
; RUN: llvm-spirv --to-text %t.spv -o %t.spt
; RUN: FileCheck < %t.spt %s -check-prefix=CHECK-SPIRV
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s

; CHECK-SPIRV-NOT: __spirv_StochasticRoundFToFINTEL
; CHECK-SPIRV-COUNT-12: StochasticRoundFToFINTEL

target datalayout = "e-p:64:64-i64:64-v16:16-v24:32-v32:32-v48:64-v64:64-v96:128-v128:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @FP32_to_FP16_stochastic_scalar(float addrspace(1)* %input, i32 addrspace(1)* %seed, half addrspace(1)* %output, half addrspace(1)* %ptrOutput) {
; CHECK-LABEL: .kernel "FP32_to_FP16_stochastic_scalar"
; CHECK-COUNT-2: lfsr.b16v2
; CHECK-NOT: lfsr.b16v2
; CHECK: srnd (M1_NM, 1) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[SRC0:[A-Za-z0-9_]+]](0,0)<1;1,0> [[SRC1:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NOT: lfsr.b16v2
; CHECK-DAG: .decl [[DST]] v_type=G type=hf
; CHECK-DAG: .decl [[SRC0]] v_type=G type=f
; CHECK-DAG: .decl [[SRC1]] v_type=G type=uw
  %seedMem = alloca i32, align 4
  %inputVal = load float, float addrspace(1)* %input, align 4
  %seedVal = load i32, i32 addrspace(1)* %seed, align 4
  %ptrSeed = add i32 %seedVal, 1
  %conv = call half @_Z32__spirv_StochasticRoundFToFINTELfi(float %inputVal, i32 %seedVal)
  %ptrConv = call half @_Z32__spirv_StochasticRoundFToFINTELfiPi(float %inputVal, i32 %ptrSeed, i32* %seedMem)
  store half %conv, half addrspace(1)* %output, align 2
  store half %ptrConv, half addrspace(1)* %ptrOutput, align 2
  %newSeed = load i32, i32* %seedMem, align 4
  store i32 %newSeed, i32 addrspace(1)* %seed, align 4
  ret void
}

declare dso_local spir_func half @_Z32__spirv_StochasticRoundFToFINTELfi(float, i32)
declare dso_local spir_func half @_Z32__spirv_StochasticRoundFToFINTELfiPi(float, i32, i32*)

define spir_kernel void @FP32_to_FP16_stochastic_vector2(<2 x float> addrspace(1)* %input, i32 addrspace(1)* %seed, <2 x half> addrspace(1)* %output, <2 x half> addrspace(1)* %ptrOutput) {
; CHECK-LABEL: .kernel "FP32_to_FP16_stochastic_vector2"
; CHECK-COUNT-2: lfsr.b16v2
; CHECK-NOT: lfsr.b16v2
; CHECK: srnd (M1_NM, 2) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[SRC0:[A-Za-z0-9_]+]](0,0)<1;1,0> [[SRC1:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NOT: lfsr.b16v2
; CHECK-DAG: .decl [[DST]] v_type=G type=hf
; CHECK-DAG: .decl [[SRC0]] v_type=G type=f
; CHECK-DAG: .decl [[SRC1]] v_type=G type=uw
  %seedMem = alloca i32, align 4
  %inputVal = load <2 x float>, <2 x float> addrspace(1)* %input, align 8
  %seedVal = load i32, i32 addrspace(1)* %seed, align 4
  %ptrSeed = add i32 %seedVal, 1
  %conv = call <2 x half> @_Z32__spirv_StochasticRoundFToFINTELDv2_fi(<2 x float> %inputVal, i32 %seedVal)
  %ptrConv = call <2 x half> @_Z32__spirv_StochasticRoundFToFINTELDv2_fiPi(<2 x float> %inputVal, i32 %ptrSeed, i32* %seedMem)
  store <2 x half> %conv, <2 x half> addrspace(1)* %output, align 4
  store <2 x half> %ptrConv, <2 x half> addrspace(1)* %ptrOutput, align 4
  %newSeed = load i32, i32* %seedMem, align 4
  store i32 %newSeed, i32 addrspace(1)* %seed, align 4
  ret void
}

declare dso_local spir_func <2 x half> @_Z32__spirv_StochasticRoundFToFINTELDv2_fi(<2 x float>, i32)
declare dso_local spir_func <2 x half> @_Z32__spirv_StochasticRoundFToFINTELDv2_fiPi(<2 x float>, i32, i32*)

define spir_kernel void @FP32_to_FP16_stochastic_vector3(<3 x float> addrspace(1)* %input, i32 addrspace(1)* %seed, <3 x half> addrspace(1)* %output, <3 x half> addrspace(1)* %ptrOutput) {
; CHECK-LABEL: .kernel "FP32_to_FP16_stochastic_vector3"
; CHECK-COUNT-4: lfsr.b16v2
; CHECK-NOT: lfsr.b16v2
; The 3-element vector is split into a 2-wide and a 1-wide srnd.
; CHECK: srnd (M1_NM, 2) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[SRC0:[A-Za-z0-9_]+]](0,0)<1;1,0> [[SRC1:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NEXT: srnd (M1_NM, 1) [[DST]](0,2)<1> [[SRC0]](0,2)<1;1,0> [[SRC1]](0,2)<1;1,0>
; CHECK-NOT: lfsr.b16v2
; CHECK-DAG: .decl [[DST]] v_type=G type=hf
; CHECK-DAG: .decl [[SRC0]] v_type=G type=f
; CHECK-DAG: .decl [[SRC1]] v_type=G type=uw
  %seedMem = alloca i32, align 4
  %inputVal = load <3 x float>, <3 x float> addrspace(1)* %input, align 16
  %seedVal = load i32, i32 addrspace(1)* %seed, align 4
  %ptrSeed = add i32 %seedVal, 1
  %conv = call <3 x half> @_Z32__spirv_StochasticRoundFToFINTELDv3_fi(<3 x float> %inputVal, i32 %seedVal)
  %ptrConv = call <3 x half> @_Z32__spirv_StochasticRoundFToFINTELDv3_fiPi(<3 x float> %inputVal, i32 %ptrSeed, i32* %seedMem)
  store <3 x half> %conv, <3 x half> addrspace(1)* %output, align 8
  store <3 x half> %ptrConv, <3 x half> addrspace(1)* %ptrOutput, align 8
  %newSeed = load i32, i32* %seedMem, align 4
  store i32 %newSeed, i32 addrspace(1)* %seed, align 4
  ret void
}

declare dso_local spir_func <3 x half> @_Z32__spirv_StochasticRoundFToFINTELDv3_fi(<3 x float>, i32)
declare dso_local spir_func <3 x half> @_Z32__spirv_StochasticRoundFToFINTELDv3_fiPi(<3 x float>, i32, i32*)

define spir_kernel void @FP32_to_FP16_stochastic_vector4(<4 x float> addrspace(1)* %input, i32 addrspace(1)* %seed, <4 x half> addrspace(1)* %output, <4 x half> addrspace(1)* %ptrOutput) {
; CHECK-LABEL: .kernel "FP32_to_FP16_stochastic_vector4"
; CHECK-COUNT-4: lfsr.b16v2
; CHECK-NOT: lfsr.b16v2
; CHECK: srnd (M1_NM, 4) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[SRC0:[A-Za-z0-9_]+]](0,0)<1;1,0> [[SRC1:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NOT: lfsr.b16v2
; CHECK-DAG: .decl [[DST]] v_type=G type=hf
; CHECK-DAG: .decl [[SRC0]] v_type=G type=f
; CHECK-DAG: .decl [[SRC1]] v_type=G type=uw
  %seedMem = alloca i32, align 4
  %inputVal = load <4 x float>, <4 x float> addrspace(1)* %input, align 16
  %seedVal = load i32, i32 addrspace(1)* %seed, align 4
  %ptrSeed = add i32 %seedVal, 1
  %conv = call <4 x half> @_Z32__spirv_StochasticRoundFToFINTELDv4_fi(<4 x float> %inputVal, i32 %seedVal)
  %ptrConv = call <4 x half> @_Z32__spirv_StochasticRoundFToFINTELDv4_fiPi(<4 x float> %inputVal, i32 %ptrSeed, i32* %seedMem)
  store <4 x half> %conv, <4 x half> addrspace(1)* %output, align 8
  store <4 x half> %ptrConv, <4 x half> addrspace(1)* %ptrOutput, align 8
  %newSeed = load i32, i32* %seedMem, align 4
  store i32 %newSeed, i32 addrspace(1)* %seed, align 4
  ret void
}

declare dso_local spir_func <4 x half> @_Z32__spirv_StochasticRoundFToFINTELDv4_fi(<4 x float>, i32)
declare dso_local spir_func <4 x half> @_Z32__spirv_StochasticRoundFToFINTELDv4_fiPi(<4 x float>, i32, i32*)

define spir_kernel void @FP32_to_FP16_stochastic_vector8(<8 x float> addrspace(1)* %input, i32 addrspace(1)* %seed, <8 x half> addrspace(1)* %output, <8 x half> addrspace(1)* %ptrOutput) {
; CHECK-LABEL: .kernel "FP32_to_FP16_stochastic_vector8"
; CHECK-COUNT-8: lfsr.b16v2
; CHECK-NOT: lfsr.b16v2
; CHECK: srnd (M1_NM, 8) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[SRC0:[A-Za-z0-9_]+]](0,0)<1;1,0> [[SRC1:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NOT: lfsr.b16v2
; CHECK-DAG: .decl [[DST]] v_type=G type=hf
; CHECK-DAG: .decl [[SRC0]] v_type=G type=f
; CHECK-DAG: .decl [[SRC1]] v_type=G type=uw
  %seedMem = alloca i32, align 4
  %inputVal = load <8 x float>, <8 x float> addrspace(1)* %input, align 32
  %seedVal = load i32, i32 addrspace(1)* %seed, align 4
  %ptrSeed = add i32 %seedVal, 1
  %conv = call <8 x half> @_Z32__spirv_StochasticRoundFToFINTELDv8_fi(<8 x float> %inputVal, i32 %seedVal)
  %ptrConv = call <8 x half> @_Z32__spirv_StochasticRoundFToFINTELDv8_fiPi(<8 x float> %inputVal, i32 %ptrSeed, i32* %seedMem)
  store <8 x half> %conv, <8 x half> addrspace(1)* %output, align 16
  store <8 x half> %ptrConv, <8 x half> addrspace(1)* %ptrOutput, align 16
  %newSeed = load i32, i32* %seedMem, align 4
  store i32 %newSeed, i32 addrspace(1)* %seed, align 4
  ret void
}

declare dso_local spir_func <8 x half> @_Z32__spirv_StochasticRoundFToFINTELDv8_fi(<8 x float>, i32)
declare dso_local spir_func <8 x half> @_Z32__spirv_StochasticRoundFToFINTELDv8_fiPi(<8 x float>, i32, i32*)

define spir_kernel void @FP32_to_FP16_stochastic_vector16(<16 x float> addrspace(1)* %input, i32 addrspace(1)* %seed, <16 x half> addrspace(1)* %output, <16 x half> addrspace(1)* %ptrOutput) {
; CHECK-LABEL: .kernel "FP32_to_FP16_stochastic_vector16"
; CHECK-COUNT-16: lfsr.b16v2
; CHECK-NOT: lfsr.b16v2
; CHECK: srnd (M1_NM, 16) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[SRC0:[A-Za-z0-9_]+]](0,0)<1;1,0> [[SRC1:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NOT: lfsr.b16v2
; CHECK-DAG: .decl [[DST]] v_type=G type=hf
; CHECK-DAG: .decl [[SRC0]] v_type=G type=f
; CHECK-DAG: .decl [[SRC1]] v_type=G type=uw
  %seedMem = alloca i32, align 4
  %inputVal = load <16 x float>, <16 x float> addrspace(1)* %input, align 64
  %seedVal = load i32, i32 addrspace(1)* %seed, align 4
  %ptrSeed = add i32 %seedVal, 1
  %conv = call <16 x half> @_Z32__spirv_StochasticRoundFToFINTELDv16_fi(<16 x float> %inputVal, i32 %seedVal)
  %ptrConv = call <16 x half> @_Z32__spirv_StochasticRoundFToFINTELDv16_fiPi(<16 x float> %inputVal, i32 %ptrSeed, i32* %seedMem)
  store <16 x half> %conv, <16 x half> addrspace(1)* %output, align 32
  store <16 x half> %ptrConv, <16 x half> addrspace(1)* %ptrOutput, align 32
  %newSeed = load i32, i32* %seedMem, align 4
  store i32 %newSeed, i32 addrspace(1)* %seed, align 4
  ret void
}

declare dso_local spir_func <16 x half> @_Z32__spirv_StochasticRoundFToFINTELDv16_fi(<16 x float>, i32)
declare dso_local spir_func <16 x half> @_Z32__spirv_StochasticRoundFToFINTELDv16_fiPi(<16 x float>, i32, i32*)
