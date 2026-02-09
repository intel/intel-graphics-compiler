;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 -dce < %s 2>&1 | FileCheck %s

; CHECK: phi <8 x float> [ zeroinitializer, %bb21 ], [ %coalesced_output, %bb22 ]
; CHECK: phi <8 x float> [ zeroinitializer, %bb21 ], [ %tmp83, %bb22 ]


; CHECK:  %coalesced_input = shufflevector <8 x float> %tmp, <8 x float> %tmp46, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK:  %coalesced_input1 = shufflevector <8 x float> %arg10, <8 x float> %tmp63, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK:  %coalesced_intrinsic = call <16 x float> @llvm.maxnum.v16f32(<16 x float> %coalesced_input, <16 x float> %coalesced_input1)
; CHECK:  %coalesced_output2 = shufflevector <16 x float> %coalesced_intrinsic, <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK:  %coalesced_output = shufflevector <16 x float> %coalesced_intrinsic, <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK:  call <8 x float> @llvm.maxnum.v8f32(<8 x float> %coalesced_output2


; ModuleID = 'reduced.ll'
source_filename = "reduced.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @widget(<8 x float> %arg9, <8 x float> %arg10, float %arg11, float %arg12, float %arg13, float %arg14, float %arg15, float %arg16, float %arg17, float %arg18, float %arg19, float %arg20) {
bb:
  br label %bb21

bb21:                                             ; preds = %bb
  br label %bb22

bb22:                                             ; preds = %bb22, %bb21
  %tmp = phi <8 x float> [ zeroinitializer, %bb21 ], [ %tmp47, %bb22 ]
  %tmp23 = phi <8 x float> [ zeroinitializer, %bb21 ], [ %tmp83, %bb22 ]
  %tmp24 = extractelement <8 x float> zeroinitializer, i32 0
  %tmp25 = extractelement <8 x float> zeroinitializer, i32 0
  %tmp26 = extractelement <8 x float> zeroinitializer, i32 0
  %tmp27 = extractelement <8 x float> zeroinitializer, i32 0
  %tmp28 = extractelement <8 x float> zeroinitializer, i32 0
  %tmp29 = extractelement <8 x float> zeroinitializer, i32 0
  %tmp30 = icmp slt i32 0, 0
  %tmp31 = icmp slt i32 0, 0
  %tmp32 = icmp slt i32 0, 0
  %tmp33 = icmp slt i32 0, 0
  %tmp34 = icmp slt i32 0, 0
  %tmp35 = icmp slt i32 0, 0
  %tmp36 = icmp slt i32 0, 0
  %tmp37 = icmp slt i32 0, 0
  %tmp38 = icmp slt i32 0, 0
  %tmp39 = icmp slt i32 0, 0
  %tmp40 = icmp slt i32 0, 0
  %tmp41 = icmp slt i32 0, 0
  %tmp42 = icmp slt i32 0, 0
  %tmp43 = icmp slt i32 0, 0
  %tmp44 = icmp slt i32 0, 0
  %tmp45 = icmp slt i32 0, 0
  %tmp46 = fmul <8 x float> %arg9, zeroinitializer
  %tmp47 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %tmp, <8 x float> %arg10)
  %tmp48 = select i1 %tmp30, float 0xFFF0000000000000, float %tmp24
  %tmp49 = select i1 %tmp31, float 0xFFF0000000000000, float %arg11
  %tmp50 = select i1 %tmp32, float 0xFFF0000000000000, float %tmp25
  %tmp51 = select i1 %tmp33, float 0xFFF0000000000000, float %tmp26
  %tmp52 = select i1 %tmp34, float 0xFFF0000000000000, float %arg12
  %tmp53 = select i1 %tmp35, float 0xFFF0000000000000, float %tmp27
  %tmp54 = select i1 %tmp36, float 0xFFF0000000000000, float %tmp28
  %tmp55 = select i1 %tmp37, float 0xFFF0000000000000, float %arg13
  %tmp56 = insertelement <8 x float> undef, float %tmp48, i32 0
  %tmp57 = insertelement <8 x float> %tmp56, float %tmp49, i32 1
  %tmp58 = insertelement <8 x float> %tmp57, float %tmp50, i32 2
  %tmp59 = insertelement <8 x float> %tmp58, float %tmp51, i32 3
  %tmp60 = insertelement <8 x float> %tmp59, float %tmp52, i32 4
  %tmp61 = insertelement <8 x float> %tmp60, float %tmp53, i32 5
  %tmp62 = insertelement <8 x float> %tmp61, float %tmp54, i32 6
  %tmp63 = insertelement <8 x float> %tmp62, float %tmp55, i32 7
  %tmp64 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %tmp46, <8 x float> %tmp63)
  %tmp65 = select i1 %tmp38, float 0xFFF0000000000000, float %tmp29
  %tmp66 = select i1 %tmp39, float 0xFFF0000000000000, float %arg14
  %tmp67 = select i1 %tmp40, float 0xFFF0000000000000, float %arg15
  %tmp68 = select i1 %tmp41, float 0xFFF0000000000000, float %arg16
  %tmp69 = select i1 %tmp42, float 0xFFF0000000000000, float %arg17
  %tmp70 = select i1 %tmp43, float 0xFFF0000000000000, float %arg18
  %tmp71 = select i1 %tmp44, float 0xFFF0000000000000, float %arg19
  %tmp72 = select i1 %tmp45, float 0xFFF0000000000000, float %arg20
  %tmp73 = insertelement <8 x float> undef, float %tmp65, i32 0
  %tmp74 = insertelement <8 x float> %tmp73, float %tmp66, i32 1
  %tmp75 = insertelement <8 x float> %tmp74, float %tmp67, i32 2
  %tmp76 = insertelement <8 x float> %tmp75, float %tmp68, i32 3
  %tmp77 = insertelement <8 x float> %tmp76, float %tmp69, i32 4
  %tmp78 = insertelement <8 x float> %tmp77, float %tmp70, i32 5
  %tmp79 = insertelement <8 x float> %tmp78, float %tmp71, i32 6
  %tmp80 = insertelement <8 x float> %tmp79, float %tmp72, i32 7
  %tmp81 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %tmp64, <8 x float> %tmp80)
  %tmp82 = call <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float> %tmp81, i8 12, i32 0)
  %tmp83 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %tmp23, <8 x float> %tmp82)
  br label %bb22
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.umin.i32(i32, i32) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>) #0

declare <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float>, i8, i32)

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!igc.functions = !{!0}

!0 = !{ptr @widget, !1}
!1 = !{!2, !29}
!2 = !{!"function_type", i32 0}
!29 = !{!"sub_group_size", i32 16}
