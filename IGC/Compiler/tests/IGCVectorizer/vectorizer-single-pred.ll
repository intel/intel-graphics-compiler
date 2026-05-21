;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerAllowUniformSelect=1 --regkey=VectorizerAllowUniformCMP=1 -dce --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 < %s 2>&1 | FileCheck %s

; CHECK: Slice:   %tmp18 = select i1 %tmpCMP, float 0.000000e+00, float %vector_extract
; CHECK: Operand num: 1 is not vectorized
; CHECK-NEXT:  %tmp18 = select i1 %tmpCMP, float 0.000000e+00, float %vector_extract --> float 0.000000e+00
; CHECK-NEXT:  %tmp19 = select i1 %tmpCMP, float 0.000000e+00, float %vector_extract8 --> float 0.000000e+00
; CHECK-NEXT:  %tmp20 = select i1 %tmpCMP, float 0.000000e+00, float %vector_extract9 --> float 0.000000e+00
; CHECK-NEXT:  %tmp21 = select i1 %tmpCMP, float 0.000000e+00, float %vector_extract10 --> float 0.000000e+00
; CHECK-NEXT:  %tmp22 = select i1 %tmpCMP, float 0.000000e+00, float %vector_extract11 --> float 0.000000e+00
; CHECK-NEXT:  %tmp23 = select i1 %tmpCMP, float 0.000000e+00, float %vector_extract12 --> float 0.000000e+00
; CHECK-NEXT:  %tmp24 = select i1 %tmpCMP, float 0.000000e+00, float %vector_extract13 --> float 0.000000e+00
; CHECK-NEXT:  %tmp25 = select i1 %tmpCMP, float 0.000000e+00, float %vector_extract14 --> float 0.000000e+00
; CHECK-NEXT:New vector created: <8 x float> zeroinitializer
; CHECK-NEXT:Same Predicate!

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @widget(<8 x i32> %arg, <3 x i32> %arg1) #0 {
bb:
  br label %bb2

bb2:                                              ; preds = %bb2, %bb
  %tmp = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i1 true, i32 0)
  %tmp3 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i1 true, i32 0)
  %tmp4 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i1 true, i32 0)
  %tmp5 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i1 true, i32 0)
  %tmp6 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i1 true, i32 0)
  %tmp7 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i1 true, i32 0)
  %tmp8 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i1 true, i32 0)
  %tmp9 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i1 true, i32 0)
  %tmp10 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp)
  %tmp11 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp3)
  %tmp12 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp4)
  %tmp13 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp5)
  %tmp14 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp6)
  %tmp15 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp7)
  %tmp16 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp8)
  %tmp17 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp9)
  %tmpCMP = fcmp oeq float %tmp17, 0xFFF0000000000000
  %tmp18 = select i1 %tmpCMP, float 0.000000e+00, float %tmp10
  %tmp19 = select i1 %tmpCMP, float 0.000000e+00, float %tmp11
  %tmp20 = select i1 %tmpCMP, float 0.000000e+00, float %tmp12
  %tmp21 = select i1 %tmpCMP, float 0.000000e+00, float %tmp13
  %tmp22 = select i1 %tmpCMP, float 0.000000e+00, float %tmp14
  %tmp23 = select i1 %tmpCMP, float 0.000000e+00, float %tmp15
  %tmp24 = select i1 %tmpCMP, float 0.000000e+00, float %tmp16
  %tmp25 = select i1 %tmpCMP, float 0.000000e+00, float %tmp17
  %tmp26 = fsub float 0.000000e+00, %tmp18
  %tmp27 = fsub float 0.000000e+00, %tmp19
  %tmp28 = fsub float 0.000000e+00, %tmp20
  %tmp29 = fsub float 0.000000e+00, %tmp21
  %tmp30 = fsub float 0.000000e+00, %tmp22
  %tmp31 = fsub float 0.000000e+00, %tmp23
  %tmp32 = fsub float 0.000000e+00, %tmp24
  %tmp33 = fsub float 0.000000e+00, %tmp25
  %tmp34 = call float @llvm.exp2.f32(float %tmp26)
  %tmp35 = call float @llvm.exp2.f32(float %tmp27)
  %tmp36 = call float @llvm.exp2.f32(float %tmp28)
  %tmp37 = call float @llvm.exp2.f32(float %tmp29)
  %tmp38 = call float @llvm.exp2.f32(float %tmp30)
  %tmp39 = call float @llvm.exp2.f32(float %tmp31)
  %tmp40 = call float @llvm.exp2.f32(float %tmp32)
  %tmp41 = call float @llvm.exp2.f32(float %tmp33)
  %tmp42 = fmul float 0.000000e+00, %tmp34
  %tmp43 = fmul float 0.000000e+00, %tmp35
  %tmp44 = fmul float 0.000000e+00, %tmp36
  %tmp45 = fmul float 0.000000e+00, %tmp37
  %tmp46 = fmul float 0.000000e+00, %tmp38
  %tmp47 = fmul float 0.000000e+00, %tmp39
  %tmp48 = fmul float 0.000000e+00, %tmp40
  %tmp49 = fmul float 0.000000e+00, %tmp41
  %tmp50 = insertelement <8 x float> zeroinitializer, float %tmp42, i64 0
  %tmp51 = insertelement <8 x float> %tmp50, float %tmp43, i64 1
  %tmp52 = insertelement <8 x float> %tmp51, float %tmp44, i64 2
  %tmp53 = insertelement <8 x float> %tmp52, float %tmp45, i64 3
  %tmp54 = insertelement <8 x float> %tmp53, float %tmp46, i64 4
  %tmp55 = insertelement <8 x float> %tmp54, float %tmp47, i64 5
  %tmp56 = insertelement <8 x float> %tmp55, float %tmp48, i64 6
  %tmp57 = insertelement <8 x float> %tmp56, float %tmp49, i64 7
  %tmp58 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp57, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br i1 false, label %bb59, label %bb2

bb59:                                             ; preds = %bb2
  %tmp60 = fcmp oeq float 0.000000e+00, 0.000000e+00
  %tmp61 = fcmp oeq float 0.000000e+00, 0.000000e+00
  %tmp62 = fcmp oeq float 0.000000e+00, 0.000000e+00
  %tmp63 = fcmp oeq float 0.000000e+00, 0.000000e+00
  %tmp64 = fcmp oeq float 0.000000e+00, 0.000000e+00
  %tmp65 = fcmp oeq float 0.000000e+00, 0.000000e+00
  %tmp66 = fcmp oeq float 0.000000e+00, 0.000000e+00
  %tmp67 = fcmp oeq float 0.000000e+00, 0.000000e+00
  %tmp68 = select i1 %tmp60, float 0.000000e+00, float 0.000000e+00
  %tmp69 = select i1 %tmp61, float 0.000000e+00, float 0.000000e+00
  %tmp70 = select i1 %tmp62, float 0.000000e+00, float 0.000000e+00
  %tmp71 = select i1 %tmp63, float 0.000000e+00, float 0.000000e+00
  %tmp72 = select i1 %tmp64, float 0.000000e+00, float 0.000000e+00
  %tmp73 = select i1 %tmp65, float 0.000000e+00, float 0.000000e+00
  %tmp74 = select i1 %tmp66, float 0.000000e+00, float 0.000000e+00
  %tmp75 = select i1 %tmp67, float 0.000000e+00, float 0.000000e+00
  %tmp76 = fdiv float 0.000000e+00, %tmp68
  %tmp77 = fdiv float 0.000000e+00, %tmp69
  %tmp78 = fdiv float 0.000000e+00, %tmp70
  %tmp79 = fdiv float 0.000000e+00, %tmp71
  %tmp80 = fdiv float 0.000000e+00, %tmp72
  %tmp81 = fdiv float 0.000000e+00, %tmp73
  %tmp82 = fdiv float 0.000000e+00, %tmp74
  %tmp83 = fdiv float 0.000000e+00, %tmp75
  %tmp84 = fptrunc float %tmp76 to half
  %tmp85 = fptrunc float %tmp77 to half
  %tmp86 = fptrunc float %tmp78 to half
  %tmp87 = fptrunc float %tmp79 to half
  %tmp88 = fptrunc float %tmp80 to half
  %tmp89 = fptrunc float %tmp81 to half
  %tmp90 = fptrunc float %tmp82 to half
  %tmp91 = fptrunc float %tmp83 to half
  %tmp92 = insertelement <8 x half> zeroinitializer, half %tmp84, i64 0
  %tmp93 = insertelement <8 x half> %tmp92, half %tmp85, i64 1
  %tmp94 = insertelement <8 x half> %tmp93, half %tmp86, i64 2
  %tmp95 = insertelement <8 x half> %tmp94, half %tmp87, i64 3
  %tmp96 = insertelement <8 x half> %tmp95, half %tmp88, i64 4
  %tmp97 = insertelement <8 x half> %tmp96, half %tmp89, i64 5
  %tmp98 = insertelement <8 x half> %tmp97, half %tmp90, i64 6
  %tmp99 = insertelement <8 x half> %tmp98, half %tmp91, i64 7
  %tmp100 = bitcast <8 x half> %tmp99 to <8 x i16>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x i16> %tmp100)
  ret void
}

; Function Attrs: convergent nounwind
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i1, i32) #0

; Function Attrs: convergent nounwind willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i16>) #2

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.maxnum.f32(float, float) #3

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #3


attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind willreturn }
attributes #2 = { nounwind }
attributes #3 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}

!0 = !{ptr @widget, !1}
!1 = !{!2, !3, !6}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5}
!4 = !{i32 0}
!5 = !{i32 2}
!6 = !{!"sub_group_size", i32 16}
