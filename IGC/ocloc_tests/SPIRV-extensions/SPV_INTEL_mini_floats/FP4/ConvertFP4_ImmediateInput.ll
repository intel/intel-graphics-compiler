;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_INTEL_float4,+SPV_KHR_bfloat16,+SPV_INTEL_int4
; RUN: ocloc compile -spirv_input -file %t.spv -device cri
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" > %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-FP16DN < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-BF16DN < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-E2M1HF < %t.visaasm
; RUN: FileCheck %s -check-prefix=CHECK-E2M1BF < %t.visaasm

; Verify opcode lowering; _builtin_spirv* functions shouldn't be called directly
; RUN: llvm-spirv --to-text %t.spv -o %t.spt
; RUN: FileCheck < %t.spt %s -check-prefix=CHECK-SPIRV
; CHECK-SPIRV-NOT: __builtin_spirv_

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

define spir_kernel void @FP16_to_E2M1_immediate(<4 x i4> addrspace(1)* %output) {
; CHECK-FP16DN-LABEL: .kernel "FP16_to_E2M1_immediate"
; CHECK-FP16DN-DAG: dnscl.hftoe2m1.mode0.rne (M1_NM, 1) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 %null.0
; CHECK-FP16DN-DAG: mov (M1_NM, 1) [[INPUT0]](0,0)<1> 0x3c003800:ud
; CHECK-FP16DN-DAG: mov (M1_NM, 1) [[INPUT1]](0,0)<1> 0x41333e00:ud
; CHECK-FP16DN-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=1
; CHECK-FP16DN-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=1
; CHECK-FP16DN-DAG: // .decl [[DST]] v_type=G type=ud num_elts=1
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid
  %conv = call <4 x i4> @_Z38__builtin_spirv_ConvertFP16ToE2M1INTELDv4_Dh(<4 x half> <half 0xH3800, half 0xH3C00, half 0xH3E00, half 0xH4133>)
  store <4 x i4> %conv, <4 x i4> addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func <4 x i4> @_Z38__builtin_spirv_ConvertFP16ToE2M1INTELDv4_Dh(<4 x half>)

define spir_kernel void @BF16_to_E2M1_immediate(<4 x i4> addrspace(1)* %output) {
; CHECK-BF16DN-LABEL: .kernel "BF16_to_E2M1_immediate"
; CHECK-BF16DN-DAG: dnscl.bftoe2m1.mode0.rne (M1_NM, 1) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 %null.0
; CHECK-BF16DN-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=1
; CHECK-BF16DN-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=1
; CHECK-BF16DN-DAG: // .decl [[DST]] v_type=G type=ud num_elts=1
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid
  %conv = call <4 x i4> @_Z38__builtin_spirv_ConvertBF16ToE2M1INTELDv4_DF16b(<4 x bfloat> <bfloat 0xR3F00, bfloat 0xR3F80, bfloat 0xR4000, bfloat 0xR4040>)
  store <4 x i4> %conv, <4 x i4> addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func <4 x i4> @_Z38__builtin_spirv_ConvertBF16ToE2M1INTELDv4_DF16b(<4 x bfloat>)

define spir_kernel void @E2M1_to_FP16_immediate(half addrspace(1)* %output) {
; CHECK-E2M1HF-LABEL: .kernel "E2M1_to_FP16_immediate"
; CHECK-E2M1HF: shfl_idx4
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %outputAddr = getelementptr half, half addrspace(1)* %output, i64 %gid
  %conv = call half @_Z38__builtin_spirv_ConvertE2M1ToFP16INTELi(i4 3)
  store half %conv, half addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func half @_Z38__builtin_spirv_ConvertE2M1ToFP16INTELi(i4)

define spir_kernel void @E2M1_to_BF16_immediate(bfloat addrspace(1)* %output) {
; CHECK-E2M1BF-LABEL: .kernel "E2M1_to_BF16_immediate"
; CHECK-E2M1BF: shfl_idx4
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %outputAddr = getelementptr bfloat, bfloat addrspace(1)* %output, i64 %gid
  %conv = call bfloat @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELi(i4 3)
  store bfloat %conv, bfloat addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func bfloat @_Z38__builtin_spirv_ConvertE2M1ToBF16INTELi(i4)
