;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-supported, llvm-14-plus

; Check that functions called once from kernel are inlined in it even if they are called multiple times from other kernels.

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -options "-igc_opts 'EnableOpaquePointersBackend=1, DisableRecompilation=1, SubroutineThreshold=50, SubroutineInlinerThreshold=10, KernelTotalSizeThreshold=50, PrintToConsole=1, PrintBefore=EmitPass'" -device pvc 2>&1 | FileCheck %s

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -options "-igc_opts 'DisableRecompilation=1, SubroutineThreshold=50, SubroutineInlinerThreshold=10, KernelTotalSizeThreshold=50, PrintToConsole=1, PrintBefore=EmitPass'" -device pvc 2>&1 | FileCheck %s

; CHECK-LABEL: @_ZTS28Kernel_A_Supposed_2B_Inlined(
; CHECK-NOT: call spir_func void @testInlineFn
; CHECK: ret void

; CHECK-LABEL: @_ZTS28Kernel_B_Supposed_2B_Inlined(
; CHECK: call spir_func void @testInlineFn
; CHECK: call spir_func void @testInlineFn
; CHECK: ret void


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @_ZTS28Kernel_A_Supposed_2B_Inlined() {
  call spir_func void @testInlineFn()
  ret void
}

define linkonce_odr spir_func void @testInlineFn() {
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()
  ret void
}

declare spir_func void @_Z8mulAccumRP4DataRK5ColorS4_()

define spir_kernel void @_ZTS28Kernel_B_Supposed_2B_Inlined() {
  call spir_func void @testInlineFn()
  call spir_func void @testInlineFn()
  ret void
}

; uselistorder directives
uselistorder void ()* @_Z8mulAccumRP4DataRK5ColorS4_, { 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }