;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 %s -o /dev/null | FileCheck %s

; CHECK: .decl sx v_type=G type=f num_elts=1
; CHECK: sel (M1_NM, 8) vectorized_select(0,0)<1>
; CHECK-NOT: sx(0,0)<1;1,0>
; CHECK: ret

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo(<8 x float> %v, <8 x float> %w, float %s) {
entry:
  %vv = fadd <8 x float> %v, %w
  %ww = fsub <8 x float> %v, %w
  %sx = fmul float %s, 2.0
  %v0 = extractelement <8 x float> %vv, i32 0
  %v1 = extractelement <8 x float> %vv, i32 1
  %v2 = extractelement <8 x float> %vv, i32 2
  %v3 = extractelement <8 x float> %vv, i32 3
  %v4 = extractelement <8 x float> %vv, i32 4
  %v5 = extractelement <8 x float> %vv, i32 5
  %v6 = extractelement <8 x float> %vv, i32 6
  %v7 = extractelement <8 x float> %vv, i32 7
  %w0 = extractelement <8 x float> %ww, i32 0
  %w1 = extractelement <8 x float> %ww, i32 1
  %w2 = extractelement <8 x float> %ww, i32 2
  %w3 = extractelement <8 x float> %ww, i32 3
  %w4 = extractelement <8 x float> %ww, i32 4
  %w5 = extractelement <8 x float> %ww, i32 5
  %w6 = extractelement <8 x float> %ww, i32 6
  %w7 = extractelement <8 x float> %ww, i32 7
  %c0 = fcmp olt float %v0, %w0
  %c1 = fcmp olt float %v1, %w1
  %c2 = fcmp olt float %v2, %w2
  %c3 = fcmp olt float %v3, %w3
  %c4 = fcmp olt float %v4, %w4
  %c5 = fcmp olt float %v5, %w5
  %c6 = fcmp olt float %v6, %w6
  %c7 = fcmp olt float %v7, %w7
  %s0 = select i1 %c0, float %v0, float %sx
  %s1 = select i1 %c1, float %v1, float %sx
  %s2 = select i1 %c2, float %v2, float %sx
  %s3 = select i1 %c3, float %v3, float %sx
  %s4 = select i1 %c4, float %v4, float %sx
  %s5 = select i1 %c5, float %v5, float %sx
  %s6 = select i1 %c6, float %v6, float %sx
  %s7 = select i1 %c7, float %v7, float %sx
  %i0 = insertelement <8 x float> undef, float %s0, i32 0
  %i1 = insertelement <8 x float> %i0, float %s1, i32 1
  %i2 = insertelement <8 x float> %i1, float %s2, i32 2
  %i3 = insertelement <8 x float> %i2, float %s3, i32 3
  %i4 = insertelement <8 x float> %i3, float %s4, i32 4
  %i5 = insertelement <8 x float> %i4, float %s5, i32 5
  %i6 = insertelement <8 x float> %i5, float %s6, i32 6
  %i7 = insertelement <8 x float> %i6, float %s7, i32 7
  %b = bitcast <8 x float> %i7 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %b)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

!igc.functions = !{!0}
!0 = !{ptr @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!12 = !{!"FuncMDMap[0]", ptr @foo}
!13 = !{!"FuncMDValue[0]", !16}
!14 = !{!"FuncMD", !12, !13}
!15 = !{!"ModuleMD", !14}
!IGCMetadata = !{!15}
!16 = !{!"requiredSubGroupSize", i32 16}
