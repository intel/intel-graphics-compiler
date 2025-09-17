;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv,regkeys,pvc-supported,spirv-promote

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_bfloat16,+SPV_INTEL_bfloat16_arithmetic -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'DumpVISAASMToConsole=1'" -device pvc 2>&1 | FileCheck %s --check-prefixes=CHECK-VISA

source_filename = "bfloat16.cpp"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

; CHECK-VISA: .kernel "test_bfloat_immediates"
define spir_kernel void @test_bfloat_immediates(bfloat %b1, bfloat addrspace(1)* %out) {
entry:
; bfloat immediates are not supported, check if float immediate is used.
; CHECK-VISA: add {{.*}} 0x3f800000:f
  %imm = fadd bfloat %b1, 1.0
  store bfloat %imm, bfloat addrspace(1)* %out, align 2
  ret void
}
