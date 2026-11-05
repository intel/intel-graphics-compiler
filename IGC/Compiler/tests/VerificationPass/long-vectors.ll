;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-verify -S < %s 2>&1 | FileCheck %s

; SPV_EXT_long_vector allows any positive number of vector elements, and
; codegen legalizes them, so the verifier doesn't restrict vector widths.

; CHECK-NOT: Unexpected vector size
; CHECK-NOT: IGC IR verification failed
; CHECK: define spir_kernel void @long_vectors(

define spir_kernel void @long_vectors(ptr addrspace(1) %p, <1 x half> %h) {
  %v = load <17 x float>, ptr addrspace(1) %p, align 4
  %w = fadd <17 x float> %v, %v
  store <17 x float> %w, ptr addrspace(1) %p, align 4
  %b = load <1024 x i8>, ptr addrspace(1) %p, align 1
  %c = shufflevector <1024 x i8> %b, <1024 x i8> %b, <2048 x i32> zeroinitializer
  %e = extractelement <2048 x i8> %c, i32 7
  store i8 %e, ptr addrspace(1) %p, align 1
  %x = fadd <1 x half> %h, %h
  %y = extractelement <1 x half> %x, i32 0
  store half %y, ptr addrspace(1) %p, align 2
  ret void
}
