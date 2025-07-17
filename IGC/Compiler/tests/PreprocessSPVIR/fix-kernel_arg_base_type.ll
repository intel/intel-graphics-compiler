;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s

; The second element of !kernel_arg_base_type is deliberately wrong
; ("char*" instead of "image1d_t"). The pass must make the node
; identical to !kernel_arg_type.

define spir_kernel void @testKernel(target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 2) %img, ptr addrspace(1) %newImg, target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 1) %storeResultsImg) !kernel_arg_type !0 !kernel_arg_base_type !1 {
  ret void
}

!0 = !{!"image1d_t", !"image1d_t", !"image1d_t"}
!1 = !{!"image1d_t", !"char*",     !"image1d_t"}

; CHECK: !kernel_arg_base_type ![[BASE:[0-9]+]]
; CHECK: ![[BASE]] = !{!"image1d_t", !"image1d_t", !"image1d_t"}
