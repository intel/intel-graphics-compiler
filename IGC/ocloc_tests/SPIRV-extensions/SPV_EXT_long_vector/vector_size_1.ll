;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_EXT_long_vector -o %t.spv
; RUN: llvm-spirv %t.spv -to-text -o - | FileCheck %s --check-prefix=CHECK-SPIRV

; IGC must consume the size-1 vector SPIR-V and emit vISA for the kernel.
; RUN: ocloc compile -spirv_input -file %t.spv -device pvc -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-VISA

; CHECK-SPIRV-DAG: Capability LongVectorEXT
; CHECK-SPIRV-DAG: Extension "SPV_EXT_long_vector"
; CHECK-SPIRV-DAG: TypeFloat [[#F32:]] 32
; CHECK-SPIRV-DAG: TypeVector [[#]] [[#F32]] 1

; CHECK-VISA: .kernel{{.*}}test_vec1

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_vec1(<1 x float> addrspace(1)* %in, <1 x float> addrspace(1)* %out) {
entry:
  %v = load <1 x float>, <1 x float> addrspace(1)* %in, align 4
  %sum = fadd <1 x float> %v, %v
  store <1 x float> %sum, <1 x float> addrspace(1)* %out, align 4
  ret void
}
