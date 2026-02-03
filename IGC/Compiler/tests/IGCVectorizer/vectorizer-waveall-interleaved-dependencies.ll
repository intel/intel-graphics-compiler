; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S -opaque-pointers  --igc-vectorizer -dce --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 --regkey=VectorizerDepWindowMultiplier=4 --regkey=VectorizerAllowWAVEALLJoint=1 --regkey=VectorizerAllowWAVEALL=1 < %s 2>&1 | FileCheck %s



; CHECK: Slice:   %tmp34 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp26, i8 12, i1 true, i32 0)
; CHECK: Slice:   %tmp35 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp27, i8 12, i1 true, i32 0)
; CHECK: Slice:   %tmp36 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp28, i8 12, i1 true, i32 0)
; CHECK: Slice:   %tmp37 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp34, i8 12, i1 true, i32 0)
; CHECK: Slice:   %tmp38 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp30, i8 12, i1 true, i32 0)
; CHECK: Slice:   %tmp39 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp31, i8 12, i1 true, i32 0)
; CHECK: Slice:   %tmp40 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp32, i8 12, i1 true, i32 0)
; CHECK: Slice:   %tmp41 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp33, i8 12, i1 true, i32 0)
; CHECK:   %tmp34 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp26, i8 12, i1 true, i32 0)
; CHECK:   %tmp41 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp33, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp34 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp26, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp35 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp27, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp36 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp28, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp37 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp34, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp30 = select i1 %tmp22, float 0xFFF0000000000000, float 0.000000e+00
; CHECK: Slice Scope:   %tmp31 = select i1 %tmp23, float 0xFFF0000000000000, float 0.000000e+00
; CHECK: Slice Scope:   %tmp32 = select i1 %tmp24, float 0xFFF0000000000000, float 0.000000e+00
; CHECK: Slice Scope:   %tmp33 = select i1 %tmp25, float 0xFFF0000000000000, float 0.000000e+00
; CHECK: Slice Scope:   %tmp38 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp30, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp39 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp31, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp40 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp32, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp41 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp33, i8 12, i1 true, i32 0)
; CHECK: %tmp34 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp26, i8 12, i1 true, i32 0) ---   %tmp37 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp34, i8 12, i1 true, i32 0)  <-- operands inside the slice depend on slice results

;CHECK-NOT: %vectorized_joint_waveall = call <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float> %{{.*}}, i8 12, i1 true, i32 0)
;CHECK-NOT: %{{vectorized_joint_waveall.*}} = call <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float> %{{.*}}, i8 12, i1 true, i32 0)

; CHECK:   %tmp113 = fmul float %tmp105, 1.250000e-01
; CHECK:   %tmp120 = fmul float %tmp112, 1.250000e-01
; CHECK: Slice Scope:   %tmp113 = fmul float %tmp105, 1.250000e-01
; CHECK: Slice Scope:   %tmp121 = fmul float %tmp113, 0x3FF7154760000000
; CHECK: Slice Scope:   %tmp129 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp121, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp137 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp129)
; CHECK: Slice Scope:   %tmp114 = fmul float %tmp106, 1.250000e-01
; CHECK: Slice Scope:   %tmp122 = fmul float %tmp114, 0x3FF7154760000000
; CHECK: Slice Scope:   %tmp130 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp122, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp138 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp130)
; CHECK: Slice Scope:   %tmp115 = fmul float %tmp107, 1.250000e-01
; CHECK: Slice Scope:   %tmp123 = fmul float %tmp115, 0x3FF7154760000000
; CHECK: Slice Scope:   %tmp131 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp123, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp139 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp131)
; CHECK: Slice Scope:   %tmp116 = fmul float %tmp139, 1.250000e-01
; CHECK: Slice Scope:   %tmp124 = fmul float %tmp116, 0x3FF7154760000000
; CHECK: Slice Scope:   %tmp132 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp124, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp117 = fmul float %tmp109, 1.250000e-01
; CHECK: Slice Scope:   %tmp125 = fmul float %tmp117, 0x3FF7154760000000
; CHECK: Slice Scope:   %tmp133 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp125, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp118 = fmul float %tmp110, 1.250000e-01
; CHECK: Slice Scope:   %tmp126 = fmul float %tmp118, 0x3FF7154760000000
; CHECK: Slice Scope:   %tmp134 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp126, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp119 = fmul float %tmp111, 1.250000e-01
; CHECK: Slice Scope:   %tmp127 = fmul float %tmp119, 0x3FF7154760000000
; CHECK: Slice Scope:   %tmp135 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp127, i8 12, i1 true, i32 0)
; CHECK: Slice Scope:   %tmp120 = fmul float %tmp112, 1.250000e-01
; CHECK:   %tmp139 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp131) ---   %tmp116 = fmul float %tmp139, 1.250000e-01  <-- operands inside the slice depend on slice results

; ModuleID = 'reduced.ll'
source_filename = "initial_waveall.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @widget() #0 {
bb:
  %tmp = load i32, i32 addrspace(1)* null, align 4
  %tmp1 = shl i32 %tmp, 7
  %tmp2 = load i32, i32 addrspace(1)* null, align 4
  %tmp3 = shl i32 %tmp2, 3
  %tmp4 = icmp sgt i32 %tmp3, 0
  br i1 %tmp4, label %bb5, label %bb100

bb5:                                              ; preds = %bb
  %tmp6 = or i32 %tmp1, 0
  br label %bb7

bb7:                                              ; preds = %bb7, %bb5
  %tmp8 = phi i32 [ %tmp6, %bb5 ], [ 0, %bb7 ]
  %tmp9 = phi float [ 0xFFF0000000000000, %bb5 ], [ %tmp42, %bb7 ]
  %tmp10 = phi float [ 0xFFF0000000000000, %bb5 ], [ %tmp43, %bb7 ]
  %tmp11 = phi float [ 0xFFF0000000000000, %bb5 ], [ %tmp44, %bb7 ]
  %tmp12 = phi float [ 0xFFF0000000000000, %bb5 ], [ %tmp45, %bb7 ]
  %tmp13 = phi float [ 0xFFF0000000000000, %bb5 ], [ %tmp46, %bb7 ]
  %tmp14 = phi float [ 0xFFF0000000000000, %bb5 ], [ %tmp47, %bb7 ]
  %tmp15 = phi float [ 0xFFF0000000000000, %bb5 ], [ %tmp48, %bb7 ]
  %tmp16 = phi float [ 0xFFF0000000000000, %bb5 ], [ %tmp49, %bb7 ]
  %tmp17 = phi float [ 0.000000e+00, %bb5 ], [ %tmp99, %bb7 ]
  %tmp18 = icmp slt i32 0, %tmp8
  %tmp19 = icmp slt i32 0, %tmp8
  %tmp20 = icmp slt i32 0, %tmp8
  %tmp21 = icmp slt i32 0, %tmp8
  %tmp22 = icmp slt i32 0, %tmp8
  %tmp23 = icmp slt i32 0, %tmp8
  %tmp24 = icmp slt i32 0, %tmp8
  %tmp25 = icmp slt i32 0, %tmp8
  %tmp26 = select i1 %tmp18, float 0xFFF0000000000000, float 0.000000e+00
  %tmp27 = select i1 %tmp19, float 0xFFF0000000000000, float 0.000000e+00
  %tmp28 = select i1 %tmp20, float 0xFFF0000000000000, float 0.000000e+00
  %tmp29 = select i1 %tmp21, float 0xFFF0000000000000, float 0.000000e+00
  %tmp34 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp26, i8 12, i1 true, i32 0)
  %tmp35 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp27, i8 12, i1 true, i32 0)
  %tmp36 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp28, i8 12, i1 true, i32 0)
  ; tmp37 depends on tmp34 --> inherent non virtual hazard
  %tmp37 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp34, i8 12, i1 true, i32 0)
  %tmp30 = select i1 %tmp22, float 0xFFF0000000000000, float 0.000000e+00
  %tmp31 = select i1 %tmp23, float 0xFFF0000000000000, float 0.000000e+00
  %tmp32 = select i1 %tmp24, float 0xFFF0000000000000, float 0.000000e+00
  %tmp33 = select i1 %tmp25, float 0xFFF0000000000000, float 0.000000e+00
  %tmp38 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp30, i8 12, i1 true, i32 0)
  %tmp39 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp31, i8 12, i1 true, i32 0)
  %tmp40 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp32, i8 12, i1 true, i32 0)
  %tmp41 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp33, i8 12, i1 true, i32 0)
  %tmp42 = call float @llvm.maxnum.f32(float %tmp9, float %tmp34)
  %tmp43 = call float @llvm.maxnum.f32(float %tmp10, float %tmp35)
  %tmp44 = call float @llvm.maxnum.f32(float %tmp11, float %tmp36)
  %tmp45 = call float @llvm.maxnum.f32(float %tmp12, float %tmp37)
  %tmp46 = call float @llvm.maxnum.f32(float %tmp13, float %tmp38)
  %tmp47 = call float @llvm.maxnum.f32(float %tmp14, float %tmp39)
  %tmp48 = call float @llvm.maxnum.f32(float %tmp15, float %tmp40)
  %tmp49 = call float @llvm.maxnum.f32(float %tmp16, float %tmp41)
  %tmp50 = fcmp oeq float %tmp42, 0xFFF0000000000000
  %tmp51 = fcmp oeq float %tmp43, 0xFFF0000000000000
  %tmp52 = fcmp oeq float %tmp44, 0xFFF0000000000000
  %tmp53 = fcmp oeq float %tmp45, 0xFFF0000000000000
  %tmp54 = fcmp oeq float %tmp46, 0xFFF0000000000000
  %tmp55 = fcmp oeq float %tmp47, 0xFFF0000000000000
  %tmp56 = fcmp oeq float %tmp48, 0xFFF0000000000000
  %tmp57 = fcmp oeq float %tmp49, 0xFFF0000000000000
  %tmp58 = select i1 %tmp50, float 0.000000e+00, float %tmp42
  %tmp59 = select i1 %tmp51, float 0.000000e+00, float %tmp43
  %tmp60 = select i1 %tmp52, float 0.000000e+00, float %tmp44
  %tmp61 = select i1 %tmp53, float 0.000000e+00, float %tmp45
  %tmp62 = select i1 %tmp54, float 0.000000e+00, float %tmp46
  %tmp63 = select i1 %tmp55, float 0.000000e+00, float %tmp47
  %tmp64 = select i1 %tmp56, float 0.000000e+00, float %tmp48
  %tmp65 = select i1 %tmp57, float 0.000000e+00, float %tmp49
  %tmp66 = fsub float %tmp9, %tmp58
  %tmp67 = fsub float %tmp10, %tmp59
  %tmp68 = fsub float %tmp11, %tmp60
  %tmp69 = fsub float %tmp12, %tmp61
  %tmp70 = fsub float %tmp13, %tmp62
  %tmp71 = fsub float %tmp14, %tmp63
  %tmp72 = fsub float %tmp15, %tmp64
  %tmp73 = fsub float %tmp16, %tmp65
  %tmp74 = call float @llvm.exp2.f32(float %tmp66)
  %tmp75 = call float @llvm.exp2.f32(float %tmp67)
  %tmp76 = call float @llvm.exp2.f32(float %tmp68)
  %tmp77 = call float @llvm.exp2.f32(float %tmp69)
  %tmp78 = call float @llvm.exp2.f32(float %tmp70)
  %tmp79 = call float @llvm.exp2.f32(float %tmp71)
  %tmp80 = call float @llvm.exp2.f32(float %tmp72)
  %tmp81 = call float @llvm.exp2.f32(float %tmp73)
  %tmp82 = fmul float %tmp17, %tmp74
  %tmp83 = fmul float 0.000000e+00, %tmp75
  %tmp84 = fmul float 0.000000e+00, %tmp76
  %tmp85 = fmul float 0.000000e+00, %tmp77
  %tmp86 = fmul float 0.000000e+00, %tmp78
  %tmp87 = fmul float 0.000000e+00, %tmp79
  %tmp88 = fmul float 0.000000e+00, %tmp80
  %tmp89 = fmul float 0.000000e+00, %tmp81
  %tmp90 = insertelement <8 x float> undef, float %tmp82, i64 0
  %tmp91 = insertelement <8 x float> %tmp90, float %tmp83, i64 1
  %tmp92 = insertelement <8 x float> %tmp91, float %tmp84, i64 2
  %tmp93 = insertelement <8 x float> %tmp92, float %tmp85, i64 3
  %tmp94 = insertelement <8 x float> %tmp93, float %tmp86, i64 4
  %tmp95 = insertelement <8 x float> %tmp94, float %tmp87, i64 5
  %tmp96 = insertelement <8 x float> %tmp95, float %tmp88, i64 6
  %tmp97 = insertelement <8 x float> %tmp96, float %tmp89, i64 7
  %tmp98 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp97, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %tmp99 = extractelement <8 x float> %tmp98, i64 0
  br i1 false, label %bb100, label %bb7

bb100:                                            ; preds = %bb7, %bb
  br label %bb101

bb101:                                            ; preds = %bb100
  br label %bb102

bb102:                                            ; preds = %bb102, %bb101
  %tmp103 = phi float [ 0.000000e+00, %bb101 ], [ %tmp186, %bb102 ]
  %tmp104 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %tmp105 = extractelement <8 x float> %tmp104, i64 0
  %tmp106 = extractelement <8 x float> %tmp104, i64 1
  %tmp107 = extractelement <8 x float> %tmp104, i64 2
  %tmp108 = extractelement <8 x float> %tmp104, i64 3
  %tmp109 = extractelement <8 x float> %tmp104, i64 4
  %tmp110 = extractelement <8 x float> %tmp104, i64 5
  %tmp111 = extractelement <8 x float> %tmp104, i64 6
  %tmp112 = extractelement <8 x float> %tmp104, i64 7
  %tmp113 = fmul float %tmp105, 1.250000e-01
  %tmp121 = fmul float %tmp113, 0x3FF7154760000000
  %tmp129 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp121, i8 12, i1 true, i32 0)
  %tmp137 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp129)
  %tmp114 = fmul float %tmp106, 1.250000e-01
  %tmp122 = fmul float %tmp114, 0x3FF7154760000000
  %tmp130 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp122, i8 12, i1 true, i32 0)
  %tmp138 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp130)
  %tmp115 = fmul float %tmp107, 1.250000e-01
  %tmp123 = fmul float %tmp115, 0x3FF7154760000000
  %tmp131 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp123, i8 12, i1 true, i32 0)
  %tmp139 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp131)
  %tmp116 = fmul float %tmp139, 1.250000e-01
  %tmp124 = fmul float %tmp116, 0x3FF7154760000000
  %tmp132 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp124, i8 12, i1 true, i32 0)
  %tmp117 = fmul float %tmp109, 1.250000e-01
  %tmp125 = fmul float %tmp117, 0x3FF7154760000000
  %tmp133 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp125, i8 12, i1 true, i32 0)
  %tmp118 = fmul float %tmp110, 1.250000e-01
  %tmp126 = fmul float %tmp118, 0x3FF7154760000000
  %tmp134 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp126, i8 12, i1 true, i32 0)
  %tmp119 = fmul float %tmp111, 1.250000e-01
  %tmp127 = fmul float %tmp119, 0x3FF7154760000000
  %tmp135 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp127, i8 12, i1 true, i32 0)
  %tmp120 = fmul float %tmp112, 1.250000e-01
  %tmp128 = fmul float %tmp120, 0x3FF7154760000000
  %tmp136 = call float @llvm.genx.GenISA.WaveAll.f32(float %tmp128, i8 12, i1 true, i32 0)
  %tmp140 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp132)
  %tmp141 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp133)
  %tmp142 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp134)
  %tmp143 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp135)
  %tmp144 = call float @llvm.maxnum.f32(float 0.000000e+00, float %tmp136)
  %tmp145 = select i1 false, float 0.000000e+00, float %tmp137
  %tmp146 = select i1 false, float 0.000000e+00, float %tmp138
  %tmp147 = select i1 false, float 0.000000e+00, float %tmp139
  %tmp148 = select i1 false, float 0.000000e+00, float %tmp140
  %tmp149 = select i1 false, float 0.000000e+00, float %tmp141
  %tmp150 = select i1 false, float 0.000000e+00, float %tmp142
  %tmp151 = select i1 false, float 0.000000e+00, float %tmp143
  %tmp152 = select i1 false, float 0.000000e+00, float %tmp144
  %tmp153 = fsub float 0.000000e+00, %tmp145
  %tmp154 = fsub float 0.000000e+00, %tmp146
  %tmp155 = fsub float 0.000000e+00, %tmp147
  %tmp156 = fsub float 0.000000e+00, %tmp148
  %tmp157 = fsub float 0.000000e+00, %tmp149
  %tmp158 = fsub float 0.000000e+00, %tmp150
  %tmp159 = fsub float 0.000000e+00, %tmp151
  %tmp160 = fsub float 0.000000e+00, %tmp152
  %tmp161 = call float @llvm.exp2.f32(float %tmp153)
  %tmp162 = call float @llvm.exp2.f32(float %tmp154)
  %tmp163 = call float @llvm.exp2.f32(float %tmp155)
  %tmp164 = call float @llvm.exp2.f32(float %tmp156)
  %tmp165 = call float @llvm.exp2.f32(float %tmp157)
  %tmp166 = call float @llvm.exp2.f32(float %tmp158)
  %tmp167 = call float @llvm.exp2.f32(float %tmp159)
  %tmp168 = call float @llvm.exp2.f32(float %tmp160)
  %tmp169 = fmul float %tmp103, %tmp161
  %tmp170 = fmul float 0.000000e+00, %tmp162
  %tmp171 = fmul float 0.000000e+00, %tmp163
  %tmp172 = fmul float 0.000000e+00, %tmp164
  %tmp173 = fmul float 0.000000e+00, %tmp165
  %tmp174 = fmul float 0.000000e+00, %tmp166
  %tmp175 = fmul float 0.000000e+00, %tmp167
  %tmp176 = fmul float 0.000000e+00, %tmp168
  %tmp177 = insertelement <8 x float> undef, float %tmp169, i64 0
  %tmp178 = insertelement <8 x float> %tmp177, float %tmp170, i64 1
  %tmp179 = insertelement <8 x float> %tmp178, float %tmp171, i64 2
  %tmp180 = insertelement <8 x float> %tmp179, float %tmp172, i64 3
  %tmp181 = insertelement <8 x float> %tmp180, float %tmp173, i64 4
  %tmp182 = insertelement <8 x float> %tmp181, float %tmp174, i64 5
  %tmp183 = insertelement <8 x float> %tmp182, float %tmp175, i64 6
  %tmp184 = insertelement <8 x float> %tmp183, float %tmp176, i64 7
  %tmp185 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp184, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %tmp186 = extractelement <8 x float> %tmp185, i64 0
  br label %bb102
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.fma.f32(float, float, float) #1

; Function Attrs: convergent inaccessiblememonly nounwind
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i1, i32) #2

; Function Attrs: convergent nounwind readnone willreturn
declare i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8, i32, i32) #3

; Function Attrs: convergent nounwind readnone willreturn
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #3

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #3

; Function Attrs: nounwind
declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #4

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #4

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i16>) #4

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.maxnum.f32(float, float) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.umin.i32(i32, i32) #1

; Function Attrs: nounwind readnone speculatable willreturn
declare i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32) #5

; Function Attrs: argmemonly nounwind speculatable willreturn writeonly
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32*, i32, i32, i1) #6

; Function Attrs: nounwind willreturn
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nounwind willreturn
declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nounwind readnone willreturn
declare i32 @llvm.genx.GenISA.imulH.i32(i32, i32) #8

attributes #0 = { convergent nounwind }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { convergent inaccessiblememonly nounwind }
attributes #3 = { convergent nounwind readnone willreturn }
attributes #4 = { nounwind }
attributes #5 = { nounwind readnone speculatable willreturn }
attributes #6 = { argmemonly nounwind speculatable willreturn writeonly }
attributes #7 = { nounwind willreturn }
attributes #8 = { nounwind readnone willreturn }


!igc.functions = !{!0}

!0 = !{ptr @widget, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
