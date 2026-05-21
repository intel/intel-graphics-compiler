; REQUIRES: regkeys
; RUN: igc_opt -S  --igc-vectorizer -dce --regkey=VectorizerEnablePartialVectorization=0 < %s 2>&1 | FileCheck %s

; CHECK: [[mul_0:%.*]] = fmul fast float
; CHECK: [[mul_1:%.*]] = fmul fast float
; CHECK: [[mul_2:%.*]] = fmul fast float
; CHECK: [[mul_3:%.*]] = fmul fast float
; CHECK: [[mul_4:%.*]] = fsub fast float
; CHECK: [[mul_5:%.*]] = fsub fast float
; CHECK: [[mul_6:%.*]] = fsub fast float
; CHECK: [[mul_7:%.*]] = fsub fast float

; CHECK: [[vec_insert0:%.*]] = insertelement <8 x float> undef, float [[mul_4]]
; CHECK: [[vec_insert1:%.*]] = insertelement <8 x float> [[vec_insert0]], float [[mul_5]]
; CHECK: [[vec_insert2:%.*]] = insertelement <8 x float> [[vec_insert1]], float [[mul_6]]
; CHECK: [[vec_insert3:%.*]] = insertelement <8 x float> [[vec_insert2]], float [[mul_7]]
; CHECK: [[vec_insert4:%.*]] = insertelement <8 x float> [[vec_insert3]], float [[mul_0]]
; CHECK: [[vec_insert5:%.*]] = insertelement <8 x float> [[vec_insert4]], float [[mul_1]]
; CHECK: [[vec_insert6:%.*]] = insertelement <8 x float> [[vec_insert5]], float [[mul_2]]
; CHECK: [[vec_insert7:%.*]] = insertelement <8 x float> [[vec_insert6]], float [[mul_3]]

define spir_kernel void @ham() {
bb:
  br label %bb1

bb1:                                              ; preds = %bb1, %bb
  %tmp = phi float [ 0.000000e+00, %bb ], [ %tmp43, %bb1 ]
  %tmp2 = phi float [ 0.000000e+00, %bb ], [ %tmp44, %bb1 ]
  %tmp3 = phi float [ 0.000000e+00, %bb ], [ %tmp45, %bb1 ]
  %tmp4 = phi float [ 0.000000e+00, %bb ], [ %tmp46, %bb1 ]
  %tmp5 = phi float [ 0.000000e+00, %bb ], [ %tmp47, %bb1 ]
  %tmp6 = phi float [ 0.000000e+00, %bb ], [ %tmp48, %bb1 ]
  %tmp7 = phi float [ 0.000000e+00, %bb ], [ %tmp49, %bb1 ]
  %tmp8 = phi float [ 0.000000e+00, %bb ], [ %tmp50, %bb1 ]
  %tmp9 = call float @llvm.exp2.f32(float 0.000000e+00)
  %tmp10 = call float @llvm.exp2.f32(float 1.000000e+00)
  %tmp11 = call float @llvm.exp2.f32(float 2.000000e+00)
  %tmp12 = call float @llvm.exp2.f32(float 3.000000e+00)
  %tmp13 = fmul fast float %tmp9, %tmp
  %tmp14 = fmul fast float %tmp10, %tmp2
  %tmp15 = fmul fast float %tmp11, %tmp3
  %tmp16 = fmul fast float %tmp12, %tmp4
  %tmp17 = call float @llvm.exp2.f32(float 4.000000e+00)
  %tmp18 = call float @llvm.exp2.f32(float 5.000000e+00)
  %tmp19 = call float @llvm.exp2.f32(float 6.000000e+00)
  %tmp20 = call float @llvm.exp2.f32(float 7.000000e+00)
  %tmp21 = fsub fast float %tmp17, %tmp5
  %tmp22 = fsub fast float %tmp18, %tmp6
  %tmp23 = fsub fast float %tmp19, %tmp7
  %tmp24 = fsub fast float %tmp20, %tmp8

  %tmp25 = call float @llvm.exp2.f32(float %tmp21)
  %tmp26 = call float @llvm.exp2.f32(float %tmp22)
  %tmp27 = call float @llvm.exp2.f32(float %tmp23)
  %tmp28 = call float @llvm.exp2.f32(float %tmp24)
  %tmp29 = call float @llvm.exp2.f32(float %tmp13)
  %tmp30 = call float @llvm.exp2.f32(float %tmp14)
  %tmp31 = call float @llvm.exp2.f32(float %tmp15)
  %tmp32 = call float @llvm.exp2.f32(float %tmp16)

  %tmp33 = insertelement <8 x float> zeroinitializer, float %tmp25, i64 0
  %tmp34 = insertelement <8 x float> %tmp33, float %tmp26, i64 1
  %tmp35 = insertelement <8 x float> %tmp34, float %tmp27, i64 2
  %tmp36 = insertelement <8 x float> %tmp35, float %tmp28, i64 3
  %tmp37 = insertelement <8 x float> %tmp36, float %tmp29, i64 4
  %tmp38 = insertelement <8 x float> %tmp37, float %tmp30, i64 5
  %tmp39 = insertelement <8 x float> %tmp38, float %tmp31, i64 6
  %tmp40 = insertelement <8 x float> %tmp39, float %tmp32, i64 7
  %tmp41 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp40, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp42 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp41, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp43 = extractelement <8 x float> %tmp42, i64 0
  %tmp44 = extractelement <8 x float> %tmp42, i64 1
  %tmp45 = extractelement <8 x float> %tmp42, i64 2
  %tmp46 = extractelement <8 x float> %tmp42, i64 3
  %tmp47 = extractelement <8 x float> %tmp42, i64 4
  %tmp48 = extractelement <8 x float> %tmp42, i64 5
  %tmp49 = extractelement <8 x float> %tmp42, i64 6
  %tmp50 = extractelement <8 x float> %tmp42, i64 7
  br label %bb1
}

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #0

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #1

attributes #0 = { convergent nounwind readnone willreturn }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}

!0 = !{void ()* @ham, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
