; REQUIRES: regkeys
; RUN: igc_opt --igc-vectorizer -S -dce --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 < %s 2>&1 | FileCheck %s

; CHECK: degenerate insert of the type <1 x float> -> rejected
; CHECK: degenerate insert of the type <1 x float> -> rejected

; Function Attrs: convergent nounwind
define spir_kernel void @widget() #0 {
bb:
  br label %bb1

bb1:                                              ; preds = %bb1, %bb
  %tmp = phi float [ 1.000000e+00, %bb ], [ 0.000000e+00, %bb1 ]
  %tmp2 = phi float [ 0.000000e+00, %bb ], [ %tmp18, %bb1 ]
  %tmp3 = phi float [ 0.000000e+00, %bb ], [ %tmp20, %bb1 ]
  %tmp5 = call float @llvm.exp2.f32(float %tmp)
  %tmp6 = fmul contract float %tmp2, %tmp5
  %tmp7 = fmul contract float %tmp3, %tmp5
  %tmp9 = insertelement <1 x float> zeroinitializer, float %tmp6, i64 0
  %tmp10 = insertelement <1 x float> zeroinitializer, float %tmp7, i64 0
  %tmp11 = call <1 x float> @llvm.genx.GenISA.sub.group.dpas.v1f32.v1f32.v1i16.v8i32(<1 x float> %tmp9, <1 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp12 = call <1 x float> @llvm.genx.GenISA.sub.group.dpas.v1f32.v1f32.v1i16.v8i32(<1 x float> %tmp10, <1 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp18 = extractelement <1 x float> %tmp11, i64 0
  %tmp20 = extractelement <1 x float> %tmp12, i64 0
  br label %bb1
}

; Function Attrs: convergent nounwind readnone willreturn
declare <1 x float> @llvm.genx.GenISA.sub.group.dpas.v1f32.v1f32.v1i16.v8i32(<1 x float>, <1 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #2

; uselistorder directives
uselistorder <1 x float> (<1 x float>, <1 x i16>, <8 x i32>, i32, i32, i32, i32, i1)* @llvm.genx.GenISA.sub.group.dpas.v1f32.v1f32.v1i16.v8i32, { 1, 0 }

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}

!0 = distinct !{void ()* @widget, !1}
!1 = distinct !{!2}
!2 = distinct !{!"sub_group_size", i32 16}
