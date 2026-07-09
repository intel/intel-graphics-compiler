;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-constant-coalescing -instcombine -dce | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define <4 x float> @f0(i32 %src) {
entry:
  %0 = inttoptr i32 %src to i8 addrspace(2490368)*
  %1 = call <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2490368i8(i8 addrspace(2490368)* %0, i32 %src, i32 8, i1 false), !lsc.cache.ctrl !3
  %2 = add i32 %src, 8
  %3 = call <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2490368i8(i8 addrspace(2490368)* %0, i32 %2, i32 8, i1 false), !lsc.cache.ctrl !3
  %4 = extractelement <2 x float> %1, i64 0
  %5 = extractelement <2 x float> %1, i64 1
  %6 = extractelement <2 x float> %3, i64 0
  %7 = extractelement <2 x float> %3, i64 1
  %8 = insertelement <4 x float> undef, float %4, i32 0
  %9 = insertelement <4 x float> %8, float %5, i32 1
  %10 = insertelement <4 x float> %9, float %6, i32 2
  %11 = insertelement <4 x float> %10, float %7, i32 3
  ret <4 x float> %11
}

 ; CHECK-LABEL: define <4 x float> @f0
 ; CHECK: %0 = inttoptr i32 %src to i8 addrspace(2490368)*
 ; CHECK: %1 = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368i8(i8 addrspace(2490368)* %0, i32 %src, i32 8, i1 false), !lsc.cache.ctrl !3
 ; CHECK: ret <4 x float> %1

; Function Attrs: argmemonly nounwind readonly
declare <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2490368i8(i8 addrspace(2490368)*, i32, i32, i1) #1

!igc.functions = !{!0}

!0 = !{<4 x float> (i32)* @f0, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{i32 9}

