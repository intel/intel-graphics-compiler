; REQUIRES: regkeys, llvm-15-or-older
; RUN: igc_opt -S  --igc-vectorizer -dce --regkey=VectorizerAllowSelect=1 --regkey=VectorizerAllowCMP=1 --regkey=VectorizerAllowMAXNUM=1 --regkey=VectorizerAllowWAVEALL=1 --regkey=VectorizerDepWindowMultiplier=6 < %s 2>&1 | FileCheck %s

; CHECK: [[dpas_0:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32
; CHECK: [[extract_0_0:%.*]] = extractelement <8 x float> [[dpas_0]], i64 0
; CHECK: [[extract_0_1:%.*]] = extractelement <8 x float> [[dpas_0]], i64 1

; CHECK: [[fmul_0_0:%.*]] = fmul float {{.*}}, 1.250000e-01
; CHECK: [[fmul_0_1:%.*]] = fmul float {{.*}}, 1.250000e-01
; CHECK: [[fmul_0_2:%.*]] = fmul float {{.*}}, 1.250000e-01
; CHECK: [[fmul_0_3:%.*]] = fmul float {{.*}}, 1.250000e-01
; CHECK: [[fmul_0_4:%.*]] = fmul float {{.*}}, 1.250000e-01
; CHECK: [[fmul_0_5:%.*]] = fmul float {{.*}}, 1.250000e-01

; CHECK: [[vector_0_0:%.*]] = insertelement <8 x float> undef, float [[extract_0_0]], i32 0
; CHECK: [[vector_0_1:%.*]] = insertelement <8 x float> [[vector_0_0]], float [[extract_0_1]], i32 1
; CHECK: [[vector_0_2:%.*]] = insertelement <8 x float> [[vector_0_1]], float [[fmul_0_0]], i32 2
; CHECK: [[vector_0_3:%.*]] = insertelement <8 x float> [[vector_0_2]], float [[fmul_0_1]], i32 3
; CHECK: [[vector_0_4:%.*]] = insertelement <8 x float> [[vector_0_3]], float [[fmul_0_2]], i32 4
; CHECK: [[vector_0_5:%.*]] = insertelement <8 x float> [[vector_0_4]], float [[fmul_0_3]], i32 5
; CHECK: [[vector_0_6:%.*]] = insertelement <8 x float> [[vector_0_5]], float [[fmul_0_4]], i32 6
; CHECK: [[vector_0_7:%.*]] = insertelement <8 x float> [[vector_0_6]], float [[fmul_0_5]], i32 7

; CHECK: [[vec_bin_0:%.*]] = fmul <8 x float> [[vector_0_7]], <float 1.250000e-01, float 1.250000e-01, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000>

; CHECK: [[vec_extract_0_0:%.*]] = extractelement <8 x float> %vectorized_binary, i32 0
; CHECK: [[vec_extract_0_1:%.*]] = extractelement <8 x float> %vectorized_binary, i32 1
; CHECK: [[vec_extract_0_2:%.*]] = extractelement <8 x float> %vectorized_binary, i32 2
; CHECK: [[vec_extract_0_3:%.*]] = extractelement <8 x float> %vectorized_binary, i32 3
; CHECK: [[vec_extract_0_4:%.*]] = extractelement <8 x float> %vectorized_binary, i32 4
; CHECK: [[vec_extract_0_5:%.*]] = extractelement <8 x float> %vectorized_binary, i32 5
; CHECK: [[vec_extract_0_6:%.*]] = extractelement <8 x float> %vectorized_binary, i32 6
; CHECK: [[vec_extract_0_7:%.*]] = extractelement <8 x float> %vectorized_binary, i32 7

; CHECK: store float [[vec_extract_0_1]], float* null, align 4
; CHECK: select i1 {{.*}}, float 0xFFF0000000000000, float [[vec_extract_0_0]]
; CHECK: select i1 {{.*}}, float 0xFFF0000000000000, float [[vec_extract_0_1]]
; CHECK: select i1 {{.*}}, float 0xFFF0000000000000, float [[vec_extract_0_2]]
; CHECK: select i1 {{.*}}, float 0xFFF0000000000000, float [[vec_extract_0_3]]
; CHECK: select i1 {{.*}}, float 0xFFF0000000000000, float [[vec_extract_0_4]]
; CHECK: select i1 {{.*}}, float 0xFFF0000000000000, float [[vec_extract_0_5]]
; CHECK: select i1 {{.*}}, float 0xFFF0000000000000, float [[vec_extract_0_6]]
; CHECK: select i1 {{.*}}, float 0xFFF0000000000000, float [[vec_extract_0_7]]

; ModuleID = 'WINDOW_RESCHED.ll'
source_filename = "initial.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @snork(i16 %arg) #0 {
bb:
  %tmp = zext i16 0 to i32
  %tmp1 = and i32 0, 0
  %tmp2 = and i32 0, 0
  %tmp3 = or i32 0, 0
  %tmp4 = or i32 0, 0
  %tmp5 = or i32 0, 0
  br label %bb6

bb6:                                              ; preds = %bb6, %bb
  %tmp7 = phi i32 [ %tmp1, %bb ], [ 0, %bb6 ]
  %tmp8 = phi float [ 0.000000e+00, %bb ], [ %tmp18, %bb6 ]
  %tmp9 = phi float [ 0.000000e+00, %bb ], [ %tmp27, %bb6 ]
  %tmp10 = phi float [ 0.000000e+00, %bb ], [ %tmp20, %bb6 ]
  %tmp11 = phi float [ 0.000000e+00, %bb ], [ %tmp21, %bb6 ]
  %tmp12 = phi float [ 0.000000e+00, %bb ], [ %tmp22, %bb6 ]
  %tmp13 = phi float [ 0.000000e+00, %bb ], [ %tmp23, %bb6 ]
  %tmp14 = phi float [ 0.000000e+00, %bb ], [ %tmp24, %bb6 ]
  %tmp15 = phi float [ 0.000000e+00, %bb ], [ %tmp25, %bb6 ]
  %tmp16 = phi float [ 0.000000e+00, %bb ], [ %tmp112, %bb6 ]
  %tmp17 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp18 = extractelement <8 x float> %tmp17, i64 0
  %tmp19 = extractelement <8 x float> %tmp17, i64 1
  %tmp20 = extractelement <8 x float> %tmp17, i64 2
  %tmp21 = extractelement <8 x float> %tmp17, i64 3
  %tmp22 = extractelement <8 x float> %tmp17, i64 4
  %tmp23 = extractelement <8 x float> %tmp17, i64 5
  %tmp24 = extractelement <8 x float> %tmp17, i64 6
  %tmp25 = extractelement <8 x float> %tmp17, i64 7
  %tmp26 = fmul float %tmp18, 1.250000e-01
  %tmp27 = fmul float %tmp19, 1.250000e-01
  store float %tmp27, float* null, align 4
  %tmp28 = fmul float %tmp20, 1.250000e-01
  %tmp29 = fmul float %tmp21, 1.250000e-01
  %tmp30 = fmul float %tmp22, 1.250000e-01
  %tmp31 = fmul float %tmp23, 1.250000e-01
  %tmp32 = fmul float %tmp24, 1.250000e-01
  %tmp33 = fmul float %tmp25, 1.250000e-01
  %tmp34 = icmp slt i32 %tmp2, %tmp7
  %tmp35 = icmp slt i32 %tmp3, %tmp7
  %tmp36 = icmp slt i32 %tmp4, %tmp7
  %tmp37 = icmp slt i32 %tmp5, %tmp1
  %tmp38 = icmp slt i32 %tmp, 1
  %tmp39 = select i1 %tmp34, float 0xFFF0000000000000, float %tmp26
  %tmp40 = select i1 %tmp35, float 0xFFF0000000000000, float %tmp27
  %tmp41 = fmul float %tmp28, 0x3FF7154760000000
  %tmp42 = select i1 %tmp36, float 0xFFF0000000000000, float %tmp41
  %tmp43 = fmul float %tmp29, 0x3FF7154760000000
  %tmp44 = select i1 %tmp37, float 0xFFF0000000000000, float %tmp43
  %tmp45 = fmul float %tmp30, 0x3FF7154760000000
  %tmp46 = select i1 %tmp38, float 0xFFF0000000000000, float %tmp45
  %tmp47 = fmul float %tmp31, 0x3FF7154760000000
  %tmp48 = select i1 %tmp38, float 0xFFF0000000000000, float %tmp47
  %tmp49 = fmul float %tmp32, 0x3FF7154760000000
  %tmp50 = select i1 %tmp38, float 0xFFF0000000000000, float %tmp49
  %tmp51 = fmul float %tmp33, 0x3FF7154760000000
  %tmp52 = select i1 %tmp38, float 0xFFF0000000000000, float %tmp51
  %tmp53 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i32 0)
  %tmp54 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i32 0)
  %tmp55 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i32 0)
  %tmp56 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i32 0)
  %tmp57 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i32 0)
  %tmp58 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i32 0)
  %tmp59 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i32 0)
  %tmp60 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 0, i32 0)
  %tmp61 = call float @llvm.maxnum.f32(float %tmp8, float %tmp53)
  %tmp62 = call float @llvm.maxnum.f32(float %tmp9, float %tmp54)
  %tmp63 = call float @llvm.maxnum.f32(float %tmp10, float %tmp55)
  %tmp64 = call float @llvm.maxnum.f32(float %tmp11, float %tmp56)
  %tmp65 = call float @llvm.maxnum.f32(float %tmp12, float %tmp57)
  %tmp66 = call float @llvm.maxnum.f32(float %tmp13, float %tmp58)
  %tmp67 = call float @llvm.maxnum.f32(float %tmp14, float %tmp59)
  %tmp68 = call float @llvm.maxnum.f32(float %tmp15, float %tmp60)
  %tmp69 = select i1 false, float 0.000000e+00, float %tmp61
  %tmp70 = select i1 false, float 0.000000e+00, float %tmp62
  %tmp71 = select i1 false, float 0.000000e+00, float %tmp63
  %tmp72 = select i1 false, float 0.000000e+00, float %tmp64
  %tmp73 = select i1 false, float 0.000000e+00, float %tmp65
  %tmp74 = select i1 false, float 0.000000e+00, float %tmp66
  %tmp75 = select i1 false, float 0.000000e+00, float %tmp67
  %tmp76 = select i1 false, float 0.000000e+00, float %tmp68
  %tmp77 = fsub float %tmp39, %tmp69
  %tmp78 = fsub float %tmp40, %tmp70
  %tmp79 = fsub float %tmp42, %tmp71
  %tmp80 = fsub float %tmp44, %tmp72
  %tmp81 = fsub float %tmp46, %tmp73
  %tmp82 = fsub float %tmp48, %tmp74
  %tmp83 = fsub float %tmp50, %tmp75
  %tmp84 = fsub float %tmp52, %tmp76
  %tmp85 = call float @llvm.exp2.f32(float %tmp77)
  %tmp86 = call float @llvm.exp2.f32(float %tmp78)
  %tmp87 = call float @llvm.exp2.f32(float %tmp79)
  %tmp88 = call float @llvm.exp2.f32(float %tmp80)
  %tmp89 = call float @llvm.exp2.f32(float %tmp81)
  %tmp90 = call float @llvm.exp2.f32(float %tmp82)
  %tmp91 = call float @llvm.exp2.f32(float %tmp83)
  %tmp92 = call float @llvm.exp2.f32(float %tmp84)
  %tmp93 = fptrunc float %tmp85 to half
  %tmp94 = fptrunc float %tmp86 to half
  %tmp95 = fptrunc float %tmp87 to half
  %tmp96 = fptrunc float %tmp88 to half
  %tmp97 = fptrunc float %tmp89 to half
  %tmp98 = fptrunc float %tmp90 to half
  %tmp99 = fptrunc float %tmp91 to half
  %tmp100 = fptrunc float %tmp92 to half
  %tmp101 = insertelement <8 x float> zeroinitializer, float %tmp16, i64 0
  %tmp102 = insertelement <8 x half> zeroinitializer, half %tmp93, i64 0
  %tmp103 = insertelement <8 x half> %tmp102, half %tmp94, i64 1
  %tmp104 = insertelement <8 x half> %tmp103, half %tmp95, i64 2
  %tmp105 = insertelement <8 x half> %tmp104, half %tmp96, i64 3
  %tmp106 = insertelement <8 x half> %tmp105, half %tmp97, i64 4
  %tmp107 = insertelement <8 x half> %tmp106, half %tmp98, i64 5
  %tmp108 = insertelement <8 x half> %tmp107, half %tmp99, i64 6
  %tmp109 = insertelement <8 x half> %tmp108, half %tmp100, i64 7
  %tmp110 = bitcast <8 x half> %tmp109 to <8 x i16>
  %tmp111 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp101, <8 x i16> %tmp110, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp112 = extractelement <8 x float> %tmp111, i64 0
  br label %bb6
}

; Function Attrs: convergent inaccessiblememonly nounwind
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i32) #1

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #2

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.maxnum.f32(float, float) #3

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #3

; uselistorder directives
uselistorder float (float, i8, i32)* @llvm.genx.GenISA.WaveAll.f32, { 7, 6, 5, 4, 3, 2, 1, 0 }
uselistorder <8 x float> (<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)* @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32, { 1, 0 }
uselistorder float (float, float)* @llvm.maxnum.f32, { 7, 6, 5, 4, 3, 2, 1, 0 }
uselistorder float (float)* @llvm.exp2.f32, { 7, 6, 5, 4, 3, 2, 1, 0 }

attributes #0 = { convergent nounwind }
attributes #1 = { convergent inaccessiblememonly nounwind }
attributes #2 = { convergent nounwind readnone willreturn }
attributes #3 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}

!0 = !{void (i16)* @snork, !1}
!1 = !{!2, !29}
!2 = !{!"function_type", i32 0}
!29 = !{!"sub_group_size", i32 16}
