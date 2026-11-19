;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S -dce -platformbmg -rev-id B -has-emulated-64-bit-insts -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK-LABEL: .kernel "splat_uniform"
; CHECK: mov (M1_NM, 8) vector_splat(0,0)<1> scalar_uniform(0,0)<0;1,0>
; CHECK: mul (M1_NM, 8) vectorized_phi(0,0)<1> vector_splat(0,0)<1;1,0> vectorized_phi(0,0)<1;1,0>

; CHECK-LABEL: .kernel "splat_non_uniform"
; CHECK: mov (M1_NM, 8) vector_splat(0,0)<1> scalar_uniform(0,0)<0;1,0>
; CHECK: mul (M1, 16) vectorized_phi(0,0)<1> vector_splat(0,0)<0;1,0> vectorized_phi(0,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(1,0)<1> vector_splat(0,1)<0;1,0> vectorized_phi(1,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(2,0)<1> vector_splat(0,2)<0;1,0> vectorized_phi(2,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(3,0)<1> vector_splat(0,3)<0;1,0> vectorized_phi(3,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(4,0)<1> vector_splat(0,4)<0;1,0> vectorized_phi(4,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(5,0)<1> vector_splat(0,5)<0;1,0> vectorized_phi(5,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(6,0)<1> vector_splat(0,6)<0;1,0> vectorized_phi(6,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(7,0)<1> vector_splat(0,7)<0;1,0> vectorized_phi(7,0)<1;1,0>

; CHECK-LABEL: .kernel "splat_of_non_uniform"
; CHECK: .decl vector_splat v_type=G type=f num_elts=128 align=wordx32
; CHECK: mov (M1, 16) vector_splat(0,0)<1> scalar_phi(0,0)<1;1,0>
; CHECK: mov (M1, 16) vector_splat(1,0)<1> scalar_phi(0,0)<1;1,0>
; CHECK: mov (M1, 16) vector_splat(2,0)<1> scalar_phi(0,0)<1;1,0>
; CHECK: mov (M1, 16) vector_splat(3,0)<1> scalar_phi(0,0)<1;1,0>
; CHECK: mov (M1, 16) vector_splat(4,0)<1> scalar_phi(0,0)<1;1,0>
; CHECK: mov (M1, 16) vector_splat(5,0)<1> scalar_phi(0,0)<1;1,0>
; CHECK: mov (M1, 16) vector_splat(6,0)<1> scalar_phi(0,0)<1;1,0>
; CHECK: mov (M1, 16) vector_splat(7,0)<1> scalar_phi(0,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(0,0)<1> vector_splat(0,0)<1;1,0> vectorized_phi(0,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(1,0)<1> vector_splat(1,0)<1;1,0> vectorized_phi(1,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(2,0)<1> vector_splat(2,0)<1;1,0> vectorized_phi(2,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(3,0)<1> vector_splat(3,0)<1;1,0> vectorized_phi(3,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(4,0)<1> vector_splat(4,0)<1;1,0> vectorized_phi(4,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(5,0)<1> vector_splat(5,0)<1;1,0> vectorized_phi(5,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(6,0)<1> vector_splat(6,0)<1;1,0> vectorized_phi(6,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_phi(7,0)<1> vector_splat(7,0)<1;1,0> vectorized_phi(7,0)<1;1,0>

define spir_kernel void @splat_uniform(float %scalar_uniform) {
entry:
  br label %loop

loop:
  %vectorized_phi = phi <8 x float> [ zeroinitializer, %entry ], [ %vectorized_binary, %loop ]
  %vector_splat = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %scalar_uniform)
  %vectorized_binary = fmul <8 x float> %vector_splat, %vectorized_phi
  br label %loop
}

define spir_kernel void @splat_non_uniform(float %scalar_uniform) {
entry:
  br label %loop

loop:
  %vectorized_phi = phi <8 x float> [ zeroinitializer, %entry ], [ %nonuniformness, %loop ]
  %vector_splat = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %scalar_uniform)
  %vectorized_binary = fmul <8 x float> %vector_splat, %vectorized_phi
  %nonuniformness = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vectorized_binary, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %loop
}

define spir_kernel void @splat_of_non_uniform(float %scalar_uniform) {
entry:
  br label %loop

loop:
  %scalar_phi = phi float [ 0.000000e+00, %entry ], [ %scalar_non_uniform, %loop ]
  %vectorized_phi = phi <8 x float> [ zeroinitializer, %entry ], [ %nonuniformness, %loop ]
  %vector_splat = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %scalar_phi)
  %vectorized_binary = fmul <8 x float> %vector_splat, %vectorized_phi
  %nonuniformness = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vectorized_binary, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %scalar_non_uniform = extractelement <8 x float> %nonuniformness, i64 0
  br label %loop
}

declare <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0, !3, !11}

!0 = !{void (float)* @splat_uniform, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (float)* @splat_non_uniform, !1}
!11 = !{void (float)* @splat_of_non_uniform, !1}

!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (float)* @splat_uniform}
!7 = !{!"FuncMDValue[1]", !4}
!8 = !{!"FuncMDMap[1]", void (float)* @splat_non_uniform}
!12 = !{!"FuncMDValue[2]", !4}
!13 = !{!"FuncMDMap[2]", void (float)* @splat_of_non_uniform}
!9 = !{!"FuncMD", !6, !5, !8, !7, !13, !12}
!10 = !{!"ModuleMD", !9}
!IGCMetadata = !{!10}
