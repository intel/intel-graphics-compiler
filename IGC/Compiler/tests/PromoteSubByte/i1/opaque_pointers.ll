;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-promote-sub-byte -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define spir_func float @readSrcPixelSingle() {
; CHECK-NEXT:  call spir_func zeroext i8 @isBorder()

; CHECK-LABEL: define spir_func zeroext i8 @isBorder() {
; CHECK-NEXT:  ret i8 0

define spir_func zeroext i1 @isBorder() {
  ret i1 false
}

define spir_func float @readSrcPixelSingle() {
  %call = call spir_func zeroext i1 @isBorder()
  ret float 0.000000e+00
}
