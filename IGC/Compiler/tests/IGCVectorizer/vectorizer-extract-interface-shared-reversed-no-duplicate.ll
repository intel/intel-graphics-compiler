;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg < %s 2>&1 | not FileCheck %s --check-prefix=DUP

; Two insertelement chains read the same reversed extract slice
; (%e0..%e7 = %src[7..0]).  A reversed swizzle cannot be substituted by %src, so
; the first chain builds an extract interface (an insertelement chain gathering
; %e0..%e7) and registers it for the extracts.  The second chain finds that
; interface via checkPrevVectorization; it holds exactly this slice in this
; order, so it must be reused.  Building a second identical interface only adds
; eight redundant movs.
;
; The DUP prefix matches the duplicate; the RUN line requires it NOT to match.
; DUP: insertelement <8 x float> undef, float %e0, i32 0
; DUP: insertelement <8 x float> undef, float %e0, i32 0

target triple = "spir64-unknown-unknown"

define spir_kernel void @shared_reversed_extracts(<8 x i16> %a, <8 x i32> %b) {
entry:
  br label %loop

loop:
  %src = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %e0 = extractelement <8 x float> %src, i64 7
  %e1 = extractelement <8 x float> %src, i64 6
  %e2 = extractelement <8 x float> %src, i64 5
  %e3 = extractelement <8 x float> %src, i64 4
  %e4 = extractelement <8 x float> %src, i64 3
  %e5 = extractelement <8 x float> %src, i64 2
  %e6 = extractelement <8 x float> %src, i64 1
  %e7 = extractelement <8 x float> %src, i64 0
  %A0 = fmul float %e0, 2.0
  %A1 = fmul float %e1, 2.0
  %A2 = fmul float %e2, 2.0
  %A3 = fmul float %e3, 2.0
  %A4 = fmul float %e4, 2.0
  %A5 = fmul float %e5, 2.0
  %A6 = fmul float %e6, 2.0
  %A7 = fmul float %e7, 2.0
  %Av0 = insertelement <8 x float> undef, float %A0, i64 0
  %Av1 = insertelement <8 x float> %Av0, float %A1, i64 1
  %Av2 = insertelement <8 x float> %Av1, float %A2, i64 2
  %Av3 = insertelement <8 x float> %Av2, float %A3, i64 3
  %Av4 = insertelement <8 x float> %Av3, float %A4, i64 4
  %Av5 = insertelement <8 x float> %Av4, float %A5, i64 5
  %Av6 = insertelement <8 x float> %Av5, float %A6, i64 6
  %Av7 = insertelement <8 x float> %Av6, float %A7, i64 7
  %B0 = fadd float %e0, 3.0
  %B1 = fadd float %e1, 3.0
  %B2 = fadd float %e2, 3.0
  %B3 = fadd float %e3, 3.0
  %B4 = fadd float %e4, 3.0
  %B5 = fadd float %e5, 3.0
  %B6 = fadd float %e6, 3.0
  %B7 = fadd float %e7, 3.0
  %Bv0 = insertelement <8 x float> undef, float %B0, i64 0
  %Bv1 = insertelement <8 x float> %Bv0, float %B1, i64 1
  %Bv2 = insertelement <8 x float> %Bv1, float %B2, i64 2
  %Bv3 = insertelement <8 x float> %Bv2, float %B3, i64 3
  %Bv4 = insertelement <8 x float> %Bv3, float %B4, i64 4
  %Bv5 = insertelement <8 x float> %Bv4, float %B5, i64 5
  %Bv6 = insertelement <8 x float> %Bv5, float %B6, i64 6
  %Bv7 = insertelement <8 x float> %Bv6, float %B7, i64 7
  %dA = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %Av7, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dB = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %Bv7, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  call void @use(<8 x float> %dA, <8 x float> %dB)
  br label %loop
}

declare void @use(<8 x float>, <8 x float>)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}
!0 = !{ptr @shared_reversed_extracts, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @shared_reversed_extracts}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
