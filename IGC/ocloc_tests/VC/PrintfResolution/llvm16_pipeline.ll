;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-supported, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc -device pvc -llvm_input -options "-vc-codegen -igc_opts 'EnableOpaquePointersBackend=1, ShaderDumpEnable=1, PrintToConsole=1'" -file %t.bc 2>&1 | FileCheck %s

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc -device pvc -llvm_input -options "-vc-codegen -igc_opts 'ShaderDumpEnable=1, PrintToConsole=1'" -file %t.bc 2>&1 | FileCheck %s

; CHECK: _after_ir_adaptors.ll
; CHECK: spir_func i32 @_Z18__spirv_ocl_printfPciiiiii

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo() {
entry:
  store <4 x i32>* null, <4 x i32>** null, align 4294967296
  %call.i18 = call spir_func i32 @_Z18__spirv_ocl_printfPciiiiii(i8* null)
  ret void
}

declare spir_func i32 @_Z18__spirv_ocl_printfPciiiiii(i8*)
