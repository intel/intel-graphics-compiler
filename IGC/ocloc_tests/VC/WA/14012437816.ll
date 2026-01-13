;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-xl-a0-supported, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc -device pvc-xl-a0 -llvm_input -options "-vc-codegen -igc_opts 'EnableOpaquePointersBackend=1, ShaderDumpEnable=1, DumpToCustomDir=%t'" -output_no_suffix -file %t.bc
; RUN: cat %t/*.visaasm | FileCheck %s

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc -device pvc-xl-a0 -llvm_input -options "-vc-codegen -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t'" -output_no_suffix -file %t.bc
; RUN: cat %t/*.visaasm | FileCheck %s

; CHECK: Build option:
; CHECK-SAME: -LSCFenceWA

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define dllexport spir_kernel void @test(i32 addrspace(1)* %in, i32 addrspace(1)* %out, i32 %arg) #0 {
entry:
  %val = load i32, i32 addrspace(1)* %in
  %add = add i32 %val, %arg
  store i32 %add, i32 addrspace(1)* %out
  ret void
}

attributes #0 = { noinline "VCFunction" }
