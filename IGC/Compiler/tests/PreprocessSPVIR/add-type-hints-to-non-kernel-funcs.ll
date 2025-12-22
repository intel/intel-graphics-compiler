;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s --check-prefixes=CHECK-A
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -igc-process-func-attributes  -S < %s | FileCheck %s --check-prefixes=CHECK-B

; CHECK-A: !non_kernel_arg_type_hints ![[#NODE:]]
; CHECK-A: ![[#NODE:]] = !{!"spirv.Image", !"spirv.SampledImage"}
; CHECK-B: alwaysinline

define spir_func void @testNonKernel(target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 2) %img, target("spirv.SampledImage", void, 1, 1, 0, 0, 0, 0, 0) %sampledImg) {
  ret void
}
