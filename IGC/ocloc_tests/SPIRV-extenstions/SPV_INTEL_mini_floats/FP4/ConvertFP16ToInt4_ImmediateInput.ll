;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
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

;
; Special case test with immediate inputs
;
define spir_kernel void @FP16_to_Int4_vector4_immediate_input(<4 x i4> addrspace(1)* %output) {
; CHECK-LABEL: .kernel "FP16_to_Int4_vector4_immediate_input"
; CHECK-DAG: dnscl.hftoint4.mode0.rne (M1_NM, 1) [[DST:[A-z0-9]*]].0 [[INPUT0:[A-z0-9]*]].0 [[INPUT1:[A-z0-9]*]].0 %null.0
; CHECK-DAG: mov (M1_NM, 1) [[INPUT0]](0,0)<1> 0x3c003800:ud
; CHECK-DAG: mov (M1_NM, 1) [[INPUT1]](0,0)<1> 0x41333e00:ud
; CHECK-DAG: // .decl [[INPUT0]] v_type=G type=ud num_elts=1
; CHECK-DAG: // .decl [[INPUT1]] v_type=G type=ud num_elts=1
; CHECK-DAG: // .decl [[DST]] v_type=G type=ud num_elts=1
  %gid = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %outputAddr = getelementptr <4 x i4>, <4 x i4> addrspace(1)* %output, i64 %gid

  %conv = call <4 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv4_Dh(<4 x half> <half 0xH3800, half 0xH3C00, half 0xH3E00, half 0xH4133>)

  store <4 x i4> %conv, <4 x i4> addrspace(1)* %outputAddr, align 2
  ret void
}
declare dso_local spir_func <4 x i4> @_Z38__builtin_spirv_ConvertFP16ToInt4INTELDv4_Dh(<4 x half>)
