;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s

; This test verifies PreprocessSPVIR retypes TargetExtTy constant
; zeroinitializer arguments to pointer null.

declare spir_func void @UseEvent(target("spirv.Event") %A)

define spir_kernel void @TestKernel() {
entry:
  call spir_func void @UseEvent(target("spirv.Event") zeroinitializer)
  ret void
}

; CHECK-LABEL: define spir_kernel void @TestKernel(
; CHECK: call spir_func void @UseEvent(
; CHECK-SAME: ptr null
; CHECK-NOT: target("spirv.Event")
; CHECK: ret void
