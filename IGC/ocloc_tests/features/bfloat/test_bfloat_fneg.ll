;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv,pvc-supported,cri-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_bfloat16,+SPV_INTEL_bfloat16_arithmetic -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'DumpVISAASMToConsole=1'" -device pvc 2>&1 | FileCheck %s --check-prefixes=CHECK-VISA-PVC

; CHECK-VISA-PVC-DAG: mov (M1_NM, 1) [[RESVAR_PVC:.*]](0,0)<1> (-)[[SRCVAR_PVC:.*]](0,0)<0;1,0>
; CHECK-VISA-PVC-DAG: mov (M1_NM, 1) [[RESVARBF_PVC:.*]](0,0)<1> [[RESVAR_PVC]](0,0)<0;1,0>
; CHECK-VISA-PVC-DAG: .decl [[RESVAR_PVC]] v_type=G type=f num_elts=1
; CHECK-VISA-PVC-DAG: .decl [[SRCVAR_PVC]] v_type=G type=f num_elts=1
; CHECK-VISA-PVC-DAG: .decl [[RESVARBF_PVC]] v_type=G type=bf num_elts=1

; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'DumpVISAASMToConsole=1'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK-VISA-CRI

; CHECK-VISA-CRI-DAG: mov (M1_NM, 1) [[RESVAR_CRI:.*]](0,0)<1> (-)[[SRCVAR_CRI:.*]](0,0)<0;1,0>
; CHECK-VISA-CRI-DAG: .decl [[RESVAR_CRI]] v_type=G type=bf num_elts=1
; CHECK-VISA-CRI-DAG: .decl [[SRCVAR_CRI]] v_type=G type=bf num_elts=1

source_filename = "bfloat16.cpp"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

define spir_kernel void @test_bfloat_fneg(bfloat %b1, bfloat addrspace(1)* %out) {
entry:
  %neg = fneg bfloat %b1
  store bfloat %neg, bfloat addrspace(1)* %out, align 2
  ret void
}
