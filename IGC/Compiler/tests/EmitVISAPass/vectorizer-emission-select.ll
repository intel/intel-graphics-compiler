;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt -S %s --opaque-pointers -dce -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; CHECK: .decl i32_argument v_type=G type=d num_elts=8 align=dword
; CHECK: cmp.le (M1_NM, 1) P1 globalOffset(0,0)<0;1,0> 0x40000000:f
; CHECK: cmp.eq (M1_NM, 1) P2 globalOffset(0,0)<0;1,0> 0x3f800000:f
; CHECK: and (M1_NM, 1) P2 P2 P1
; CHECK: (P2) sel (M1_NM, 8) vectorized_select(0,0)<1> V0032(0,0)<1;1,0> vectorized_intrinsic(0,0)<1;1,0>

; CHECK: cmp.eq (M1_NM, 8) [[PRED:P.*]] i32_argument(0,0)<1;1,0> V0033(0,0)<1;1,0>
; CHECK: ([[PRED]]) sel (M1_NM, 8) vectorized_select917(0,0)<1>

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo(<8 x i32> %i32_argument, <8 x float> %vector651, <8 x float> %vectorized_binary612, <8 x float> %vectorized_phi1059, float %scalar_argument) {
._crit_edge:
  br label %._crit_edge575

._crit_edge575:                                   ; preds = %._crit_edge575, %._crit_edge
  %vectorized_phi = phi <8 x float> [ zeroinitializer, %._crit_edge ], [ %vectorized_intrinsic721, %._crit_edge575 ]
  %vectorized_intrinsic = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %vectorized_phi, <8 x float> %vector651)
  %vectorized_cmp = fcmp oeq float %scalar_argument, 1.0
  %vectorized_cmp2 = fcmp ole float %scalar_argument, 2.0
  %vectorized_cmp_and = and i1 %vectorized_cmp, %vectorized_cmp2
  %vectorized_select = select i1 %vectorized_cmp_and, <8 x float> zeroinitializer, <8 x float> %vectorized_intrinsic
  %vectorized_binary712 = fsub <8 x float> %vectorized_binary612, %vectorized_select
  %vectorized_intrinsic721 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %vectorized_binary712)
  br i1 true, label %._crit_edge90, label %._crit_edge575

._crit_edge90:                                    ; preds = %._crit_edge575
  %vectorized_cmp908 = icmp eq <8 x i32> %i32_argument, <i32 1, i32 1,i32 1,i32 1,i32 1,i32 1,i32 1,i32 1>
  %vectorized_select917 = select <8 x i1> %vectorized_cmp908, <8 x float> zeroinitializer, <8 x float> zeroinitializer
  %vectorized_binary1068 = fdiv <8 x float> %vectorized_phi1059, %vectorized_select917
  %vectorized_cast1077 = fptrunc <8 x float> %vectorized_binary1068 to <8 x half>
  %.assembled.vect567 = bitcast <8 x half> %vectorized_cast1077 to <8 x i16>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 16, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i16> %.assembled.vect567)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i16>)

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x float> @llvm.exp2.v8f32(<8 x float>) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2, !3, !6, !7}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5}
!4 = !{i32 0}
!5 = !{i32 2}
!6 = !{!"sub_group_size", i32 16}
!7 = !{!"max_reg_pressure", i32 151}
