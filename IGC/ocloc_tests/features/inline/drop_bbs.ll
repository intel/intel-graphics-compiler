;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: false
; REQUIRES: regkeys,pvc-supported,llvm-16-plus

; Check that specifed BB is removed from the function.

; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: echo "call1bb" > %T/testKernel.txt
; RUN: ocloc compile -llvm_input -file %t.bc -options "-igc_opts 'DisableInlining=1, DisableRecompilation=1, EnableDropTargetBBs=1, DropTargetBBListPath=%T, VerboseDropTargetBBs=1, PrintToConsole=1, PrintBefore=EmitPass'" -device pvc 2>&1 | FileCheck %s

; CHECK-LABEL: @testKernel(
; CHECK-NOT: call1bb
; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32)

define linkonce_odr spir_func void @testInlineFn(i32 %d) {
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  call spir_func void @_Z8mulAccumRP4DataRK5ColorS4_(i32 %d)
  ret void
}

define spir_kernel void @testKernel(i32 %a, i32 %b, i32 %c) {
  %Pivot = icmp slt i32 %a, 1
  br i1 %Pivot, label %LeafBlock, label %LeafBlock6

  LeafBlock6:
  %SwitchLeaf7 = icmp eq i32 %a, 1
  br i1 %SwitchLeaf7, label %call2bb, label %NewDefault

  LeafBlock:
  %SwitchLeaf = icmp eq i32 %a, 0
  br i1 %SwitchLeaf, label %call1bb, label %NewDefault

  call1bb:
  call spir_func void @testInlineFn(i32 %b)
  br label %NewDefault

  call2bb:
  call spir_func void @testInlineFn(i32 %c)
  br label %NewDefault

  NewDefault:
  ret void
}