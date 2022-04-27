;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -igc-constant-coalescing -instcombine -dce | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define <4 x float> @f0(i32 %src) {
entry:
  %0 = inttoptr i32 %src to i8 addrspace(2490373)*
  %1 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %src, i32 4, i1 false)
  %2 = add i32 %src, 16
  %3 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %2, i32 4, i1 false)
  %4 = add i32 %src, 32
  %5 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %4, i32 4, i1 false)
  %6 = add i32 %src, 48
  %7 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %6, i32 4, i1 false)
  %8 = insertelement <4 x float> undef, float %1, i32 0
  %9 = insertelement <4 x float> %8, float %3, i32 1
  %10 = insertelement <4 x float> %9, float %5, i32 2
  %11 = insertelement <4 x float> %10, float %7, i32 3
  ret <4 x float> %11
}

 ; CHECK-LABEL: define <4 x float> @f0
 ; CHECK: %0 = inttoptr i32 %src to i8 addrspace(2490373)*
 ; CHECK: %1 = call <16 x float> @llvm.genx.GenISA.ldrawvector.indexed.v16f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %src, i32 4, i1 false)
 ; CHECK: %2 = shufflevector <16 x float> %1, <16 x float> undef, <4 x i32> <i32 0, i32 4, i32 8, i32 12>
 ; CHECK: ret <4 x float> %2


define <4 x float> @f1(i32 %src, i8 addrspace(2490373)* %dst) {
entry:
  %0 = inttoptr i32 %src to i8 addrspace(2490373)*
  %1 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %src, i32 4, i1 false)
  %2 = add i32 %src, 16
  store i8 5, i8 addrspace(2490373)* %dst
  %3 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %2, i32 4, i1 false)
  %4 = add i32 %src, 32
  %5 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %4, i32 4, i1 false)
  %6 = add i32 %src, 48
  %7 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %6, i32 4, i1 false)
  %8 = insertelement <4 x float> undef, float %1, i32 0
  %9 = insertelement <4 x float> %8, float %3, i32 1
  %10 = insertelement <4 x float> %9, float %5, i32 2
  %11 = insertelement <4 x float> %10, float %7, i32 3
  ret <4 x float> %11
}

 ; CHECK-LABEL: define <4 x float> @f1
 ; CHECK: %1 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %src, i32 4, i1 false)
 ; CHECK: store i8 5, i8 addrspace(2490373)* %dst, align 1
 ; CHECK: %3 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %2, i32 4, i1 false)
 ; CHECK: %5 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %4, i32 4, i1 false)
 ; CHECK: %7 = call fast float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)* %0, i32 %6, i32 4, i1 false)

; Function Attrs: argmemonly nounwind readonly
declare float @llvm.genx.GenISA.ldraw.indexed.f32.p2490373i8(i8 addrspace(2490373)*, i32, i32, i1) argmemonly nounwind readonly

!igc.functions = !{!0, !3}

!0 = !{<4 x float> (i32)* @f0, !1}
!3 = !{<4 x float> (i32, i8 addrspace(2490373)*)* @f1, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}

