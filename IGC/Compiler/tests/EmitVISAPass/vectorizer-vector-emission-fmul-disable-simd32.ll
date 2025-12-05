;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus

; RUN: not --crash igc_opt -S -opaque-pointers -dce -platformpvc -igc-emit-visa --regkey=EnableAssertEvaluation=1 --regkey=EnableAssertProgramTermination=0 --regkey=EnableLogAssertToStderr=1 --regkey=EnableStandardAssert=1 --regkey=DumpVISAASMToConsole=1 -simd-mode 32 < %s &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; CHECK: numLanes(m_encoder->GetSimdSize()) == 16, As of now Vector Emission is only supported for SIMD16

define spir_kernel void @widget() {
entry:
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge.._crit_edge_crit_edge, %6
  %vectorized_phi = phi <8 x float> [ zeroinitializer, %entry ], [ %result, %._crit_edge.._crit_edge_crit_edge ]
  %vector = insertelement <8 x float> zeroinitializer, float 0.000000e+00, i64 0
  %vectorized_binary = fmul <8 x float> %vector, %vectorized_phi
  %result = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vectorized_binary, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %._crit_edge.._crit_edge_crit_edge

._crit_edge.._crit_edge_crit_edge:                ; preds = %._crit_edge
  br label %._crit_edge
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}

!0 = distinct !{ptr @widget, !1}
!1 = distinct !{!2}
!2 = distinct !{!"function_type", i32 0}
