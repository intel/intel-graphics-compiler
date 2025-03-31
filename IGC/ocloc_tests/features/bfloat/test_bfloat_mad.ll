;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv,regkeys,pvc-supported,spirv-promote

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_bfloat16 -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-cl-fast-relaxed-math -igc_opts 'DumpVISAASMToConsole=1'" -device pvc 2>&1 | FileCheck %s --check-prefixes=CHECK-VISA

source_filename = "bfloat16.cpp"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

; CHECK-VISA: .kernel "test_bfloat_mad"
; CHECK-VISA-DAG: mad (M1_NM, 1) [[RES:.*]](0,0)<1> [[BFLOAT1:.*]](0,0)<0;1,0> [[BFLOAT2:.*]](0,0)<0;1,0> [[BFLOAT3:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RES]] {{.*}} type=bf
; CHECK-VISA-DAG: .decl [[BFLOAT1]] {{.*}} type=bf
; CHECK-VISA-DAG: .decl [[BFLOAT2]] {{.*}} type=bf
; CHECK-VISA-DAG: .decl [[BFLOAT3]] {{.*}} type=bf
define spir_kernel void @test_bfloat_mad(bfloat %b1, bfloat %b2, bfloat %b3, i1 %cond, bfloat addrspace(1)* %out) {
entry:
  %mul = fmul bfloat %b1, %b2
  %res = fadd bfloat %mul, %b3
  store bfloat %res, bfloat addrspace(1)* %out, align 4
  ret void
}
