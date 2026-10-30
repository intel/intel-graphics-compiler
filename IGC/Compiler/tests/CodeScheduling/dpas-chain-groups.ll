;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableCodeSchedulingIfNoSpills=1 --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --regkey ForceOCLSIMDWidth=16 --igc-code-scheduling -S %s | FileCheck %s
;
; DPAS chains whose heads share an A or B matrix defined in the block form a
; group, and when one head is scheduled, FireChainGroupHeads schedules the other
; heads of its group next. Otherwise %c1, whose MaxWeight is higher than %h2's,
; follows %h1.

; %h1 and %h2 have the same precisions, systolic depth and repeat count, but no
; common matrix.
;
; CHECK-LABEL: define spir_kernel void @independent_chains
; CHECK: %h1 = call
; CHECK-NEXT: %c1 = call
; CHECK-NEXT: %c2 = call
; CHECK-NEXT: %h2 = call

; %h1 and %h2 share the B matrix %b, which the block loads.
;
; CHECK-LABEL: define spir_kernel void @shared_matrix_chains
; CHECK: %h1 = call
; CHECK-NEXT: %h2 = call
; CHECK-NEXT: %c1 = call
; CHECK-NEXT: %c2 = call

; %h1 and %h2 share the B matrix %b, a kernel argument. Firing %h2 early does
; not free %b, which stays live through the block, so the chains are not
; grouped.
;
; CHECK-LABEL: define spir_kernel void @shared_argument_matrix_chains
; CHECK: %h1 = call
; CHECK-NEXT: %c1 = call
; CHECK-NEXT: %c2 = call
; CHECK-NEXT: %h2 = call

define spir_kernel void @independent_chains(<8 x i16> %a1, <8 x i32> %b1, <8 x i16> %a2, <8 x i32> %b2,
                                            ptr addrspace(1) %dst1, ptr addrspace(1) %dst2) {
entry:
  %h2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> %a2, <8 x i32> %b2, i32 11, i32 11, i32 8, i32 8, i1 false)
  %h1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> %a1, <8 x i32> %b1, i32 11, i32 11, i32 8, i32 8, i1 false)
  %c1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> %h1, <8 x i16> %a1, <8 x i32> %b1, i32 11, i32 11, i32 8, i32 8, i1 false)
  %c2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> %c1, <8 x i16> %a1, <8 x i32> %b1, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %c2, ptr addrspace(1) %dst1, align 32
  store <8 x float> %h2, ptr addrspace(1) %dst2, align 32
  ret void
}

define spir_kernel void @shared_matrix_chains(<8 x i16> %a1, <8 x i16> %a2, ptr addrspace(1) %src,
                                              ptr addrspace(1) %dst1, ptr addrspace(1) %dst2) {
entry:
  %b = load <8 x i32>, ptr addrspace(1) %src, align 32
  %h2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> %a2, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %h1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> %a1, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %c1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> %h1, <8 x i16> %a1, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %c2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> %c1, <8 x i16> %a1, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %c2, ptr addrspace(1) %dst1, align 32
  store <8 x float> %h2, ptr addrspace(1) %dst2, align 32
  ret void
}

define spir_kernel void @shared_argument_matrix_chains(<8 x i16> %a1, <8 x i16> %a2, <8 x i32> %b,
                                                       ptr addrspace(1) %dst1, ptr addrspace(1) %dst2) {
entry:
  %h2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> %a2, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %h1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> %a1, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %c1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> %h1, <8 x i16> %a1, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %c2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> %c1, <8 x i16> %a1, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %c2, ptr addrspace(1) %dst1, align 32
  store <8 x float> %h2, ptr addrspace(1) %dst2, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
    <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
