;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --platformCri -dce < %s | FileCheck %s
; CHECK: %vector_extract8 = extractelement <8 x float> %vectorized_binary, i32 0
; CHECK: %vector_extract = extractelement <8 x float> %vectorized_phi, i32 0
; CHECK: %mix0 = fsub float %vector_extract, 0.000000e+00
; CHECK: %mix1 = fsub float %vector_extract8, 0.000000e+00

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128-v192:256:256-v256:256-v512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo() {
entry:
  br label %loop

loop:
  %p0 = phi float [ 0.000000e+00, %entry ], [ 0.000000e+00, %loop ]
  %p1 = phi float [ 0.000000e+00, %entry ], [ 0.000000e+00, %loop ]
  %p2 = phi float [ 0.000000e+00, %entry ], [ 0.000000e+00, %loop ]
  %p3 = phi float [ 0.000000e+00, %entry ], [ 0.000000e+00, %loop ]
  %p4 = phi float [ 0.000000e+00, %entry ], [ 0.000000e+00, %loop ]
  %p5 = phi float [ 0.000000e+00, %entry ], [ 0.000000e+00, %loop ]
  %p6 = phi float [ 0.000000e+00, %entry ], [ 0.000000e+00, %loop ]
  %p7 = phi float [ 0.000000e+00, %entry ], [ 0.000000e+00, %loop ]

  %pre0 = insertelement <8 x float> zeroinitializer, float %p0, i64 0
  %pre1 = insertelement <8 x float> %pre0, float %p1, i64 1
  %pre2 = insertelement <8 x float> %pre1, float %p2, i64 2
  %pre3 = insertelement <8 x float> %pre2, float %p3, i64 3
  %pre4 = insertelement <8 x float> %pre3, float %p4, i64 4
  %pre5 = insertelement <8 x float> %pre4, float %p5, i64 5
  %pre6 = insertelement <8 x float> %pre5, float %p6, i64 6
  %pre7 = insertelement <8 x float> %pre6, float %p7, i64 7
  %pre_dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %pre7, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)

  %y1 = fadd float 1.000000e+00, 2.000000e+00
  %y2 = fadd float 1.000000e+00, 2.000000e+00
  %y3 = fadd float 1.000000e+00, 2.000000e+00
  %y4 = fadd float 1.000000e+00, 2.000000e+00
  %y5 = fadd float 1.000000e+00, 2.000000e+00
  %y6 = fadd float 1.000000e+00, 2.000000e+00
  %y7 = fadd float 1.000000e+00, 2.000000e+00
  %y8 = fadd float 1.000000e+00, 2.000000e+00

  %mix0 = fsub float %p0, 0.000000e+00
  %mix1 = fsub float %y1, 0.000000e+00
  %mix2 = fsub float 0.000000e+00, 0.000000e+00
  %mix3 = fsub float 0.000000e+00, 0.000000e+00
  %mix4 = fsub float 0.000000e+00, 0.000000e+00
  %mix5 = fsub float 0.000000e+00, 0.000000e+00
  %mix6 = fsub float 0.000000e+00, 0.000000e+00
  %mix7 = fsub float 0.000000e+00, 0.000000e+00
  %mix.vec0 = insertelement <8 x float> zeroinitializer, float %mix0, i64 0
  %mix.vec1 = insertelement <8 x float> %mix.vec0, float %mix1, i64 1
  %mix.vec2 = insertelement <8 x float> %mix.vec1, float %mix2, i64 2
  %mix.vec3 = insertelement <8 x float> %mix.vec2, float %mix3, i64 3
  %mix.vec4 = insertelement <8 x float> %mix.vec3, float %mix4, i64 4
  %mix.vec5 = insertelement <8 x float> %mix.vec4, float %mix5, i64 5
  %mix.vec6 = insertelement <8 x float> %mix.vec5, float %mix6, i64 6
  %mix.vec7 = insertelement <8 x float> %mix.vec6, float %mix7, i64 7
  %mix_dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %mix.vec7, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)

  %final0 = insertelement <8 x float> zeroinitializer, float %y1, i64 0
  %final1 = insertelement <8 x float> %final0, float %y2, i64 1
  %final2 = insertelement <8 x float> %final1, float %y3, i64 2
  %final3 = insertelement <8 x float> %final2, float %y4, i64 3
  %final4 = insertelement <8 x float> %final3, float %y5, i64 4
  %final5 = insertelement <8 x float> %final4, float %y6, i64 5
  %final6 = insertelement <8 x float> %final5, float %y7, i64 6
  %final7 = insertelement <8 x float> %final6, float %y8, i64 7
  %final_dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %final7, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)

  store <8 x float> %mix_dpas, ptr null
  store <8 x float> %final_dpas, ptr null
  br label %loop
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare spir_kernel void @Intel_Symbol_Table_Void_Program()

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{!"requiredSubGroupSize", i32 16}
!4 = !{!"FuncMDValue[0]", !3}
!5 = !{!"FuncMDMap[0]", ptr @foo}
!6 = !{!"FuncMD", !5, !4}
!7 = !{!"ModuleMD", !6}
!IGCMetadata = !{!7}
