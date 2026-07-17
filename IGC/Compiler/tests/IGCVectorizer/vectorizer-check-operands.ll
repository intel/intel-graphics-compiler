;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --platformCri < %s 2>&1 | FileCheck %s
; CHECK: vectorized_phi

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo() {
cond-add:
  br label %LeafBlock

LeafBlock:                                        ; preds = %LeafBlock, %cond-add
  %0 = phi float [ 0.000000e+00, %cond-add ], [ 0.000000e+00, %LeafBlock ]
  %1 = phi float [ 0.000000e+00, %cond-add ], [ 0.000000e+00, %LeafBlock ]
  %2 = phi float [ 0.000000e+00, %cond-add ], [ 0.000000e+00, %LeafBlock ]
  %3 = phi float [ 0.000000e+00, %cond-add ], [ 0.000000e+00, %LeafBlock ]
  %4 = phi float [ 0.000000e+00, %cond-add ], [ 0.000000e+00, %LeafBlock ]
  %5 = phi float [ 0.000000e+00, %cond-add ], [ 0.000000e+00, %LeafBlock ]
  %6 = phi float [ 0.000000e+00, %cond-add ], [ 0.000000e+00, %LeafBlock ]
  %7 = phi float [ 0.000000e+00, %cond-add ], [ 0.000000e+00, %LeafBlock ]
  %B241 = fsub float %0, 0.000000e+00
  %B242 = fsub float 0.000000e+00, 0.000000e+00
  %B243 = fsub float 0.000000e+00, 0.000000e+00
  %B244 = fsub float 0.000000e+00, 0.000000e+00
  %B245 = fsub float 0.000000e+00, 0.000000e+00
  %B246 = fsub float 0.000000e+00, 0.000000e+00
  %B247 = fsub float 0.000000e+00, 0.000000e+00
  %B248 = fsub float 0.000000e+00, 0.000000e+00
  %B.assembled.vect = insertelement <8 x float> zeroinitializer, float %B241, i64 0
  %B.assembled.vect403 = insertelement <8 x float> %B.assembled.vect, float %B242, i64 1
  %B.assembled.vect404 = insertelement <8 x float> %B.assembled.vect403, float %B243, i64 2
  %B.assembled.vect405 = insertelement <8 x float> %B.assembled.vect404, float %B244, i64 3
  %B.assembled.vect406 = insertelement <8 x float> %B.assembled.vect405, float %B245, i64 4
  %B.assembled.vect407 = insertelement <8 x float> %B.assembled.vect406, float %B246, i64 5
  %B.assembled.vect408 = insertelement <8 x float> %B.assembled.vect407, float %B247, i64 6
  %B.assembled.vect409 = insertelement <8 x float> %B.assembled.vect408, float %B248, i64 7
  %dpas29 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %B.assembled.vect409, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %.assembled.vect458 = insertelement <8 x float> zeroinitializer, float %0, i64 0
  %.assembled.vect459 = insertelement <8 x float> %.assembled.vect458, float %1, i64 1
  %.assembled.vect460 = insertelement <8 x float> %.assembled.vect459, float %2, i64 2
  %.assembled.vect461 = insertelement <8 x float> %.assembled.vect460, float %3, i64 3
  %.assembled.vect462 = insertelement <8 x float> %.assembled.vect461, float %4, i64 4
  %.assembled.vect463 = insertelement <8 x float> %.assembled.vect462, float %5, i64 5
  %.assembled.vect464 = insertelement <8 x float> %.assembled.vect463, float %6, i64 6
  %.assembled.vect465 = insertelement <8 x float> %.assembled.vect464, float %7, i64 7
  %dpas33 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %.assembled.vect465, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %LeafBlock
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
