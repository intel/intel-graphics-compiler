;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys,pvc-supported,llvm-14-plus

; Check that no inline prevents function inlining.

; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -options "-igc_opts 'DisableInlining=1, PrintToConsole=1, PrintBefore=EmitPass'" -device pvc 2>&1 | FileCheck %s

; CHECK-LABEL: @testKernel(
; CHECK: call spir_func void @testInlineFn1
; CHECK: call spir_func void @testInlineFn2
; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func void @externFn1(i32)
declare spir_func void @externFn2(i32)

define linkonce_odr spir_func void @testInlineFn1(i32 %d) #0 {
  call spir_func void @externFn1(i32 %d)
  ret void
}

define linkonce_odr spir_func void @testInlineFn2(i32 %d) {
  call spir_func void @externFn2(i32 %d)
  ret void
}

define spir_kernel void @testKernel(i32 %a, i32 %b, i32 %c) {
  call spir_func void @testInlineFn1(i32 %b)
  call spir_func void @testInlineFn2(i32 %c)
  ret void
}

attributes #0 = { alwaysinline }
