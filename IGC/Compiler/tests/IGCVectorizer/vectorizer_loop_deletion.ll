; NOTE: this test just checks that we can process such IR
; previously we had issues with processing discarded instructions for
; vectorization chain
; RUN: igc_opt -S %s --igc-vectorizer -dce

; ModuleID = 'reduced.ll'
source_filename = "reduced.ll"

define spir_kernel void @widget() {
bb:
  %tmp = fmul reassoc nsz arcp contract float 0.000000e+00, 0.000000e+00
  br i1 false, label %bb1, label %bb12

bb1:                                              ; preds = %bb
  br label %bb12

bb2:                                              ; No predecessors!
  %tmp3 = fmul float %tmp, %tmp
  %tmp4 = fmul float %tmp3, %tmp
  %tmp5 = fmul float %tmp3, %tmp4
  %tmp6 = fmul float %tmp5, 0.000000e+00
  %tmp7 = fadd float 0.000000e+00, %tmp6
  %tmp8 = fadd float %tmp7, 0.000000e+00
  %tmp9 = fdiv float %tmp8, 0.000000e+00
  br label %bb12

bb10:                                             ; No predecessors!
  br label %bb12

bb11:                                             ; No predecessors!
  br label %bb12

bb12:                                             ; preds = %bb11, %bb10, %bb2, %bb1, %bb
  %tmp13 = phi float [ %tmp9, %bb2 ], [ 0.000000e+00, %bb10 ], [ 0.000000e+00, %bb11 ], [ 0.000000e+00, %bb ], [ 0.000000e+00, %bb1 ]
  %tmp14 = fmul reassoc nsz arcp contract float %tmp13, 0.000000e+00
  %tmp15 = fadd reassoc nsz arcp contract float %tmp14, 0.000000e+00
  %tmp16 = fmul reassoc nsz arcp contract float %tmp15, 0.000000e+00
  %tmp17 = insertelement <8 x float> zeroinitializer, float %tmp16, i64 0
  %tmp18 = fmul reassoc nsz arcp contract float 0.000000e+00, 0.000000e+00
  br i1 false, label %bb19, label %bb30

bb19:                                             ; preds = %bb12
  br label %bb30

bb20:                                             ; No predecessors!
  br label %bb30

bb21:                                             ; No predecessors!
  br label %bb30

bb22:                                             ; No predecessors!
  %tmp23 = fmul float %tmp18, %tmp18
  %tmp24 = fmul float %tmp18, %tmp18
  %tmp25 = fmul float %tmp23, %tmp24
  %tmp26 = fmul float %tmp25, 0.000000e+00
  %tmp27 = fadd float 0.000000e+00, %tmp26
  %tmp28 = fadd float %tmp27, 0.000000e+00
  %tmp29 = fdiv float %tmp28, 0.000000e+00
  br label %bb30

bb30:                                             ; preds = %bb22, %bb21, %bb20, %bb19, %bb12
  %tmp31 = phi float [ %tmp29, %bb22 ], [ 0.000000e+00, %bb21 ], [ 0.000000e+00, %bb20 ], [ 0.000000e+00, %bb12 ], [ 0.000000e+00, %bb19 ]
  %tmp32 = fmul reassoc nsz arcp contract float %tmp31, 0.000000e+00
  %tmp33 = fadd reassoc nsz arcp contract float %tmp32, 0.000000e+00
  %tmp34 = fmul reassoc nsz arcp contract float %tmp33, 0.000000e+00
  %tmp35 = insertelement <8 x float> %tmp17, float %tmp34, i64 0
  %tmp36 = fmul reassoc nsz arcp contract float 0.000000e+00, 0.000000e+00
  br i1 false, label %bb37, label %bb48

bb37:                                             ; preds = %bb30
  br label %bb48

bb38:                                             ; No predecessors!
  br label %bb48

bb39:                                             ; No predecessors!
  br label %bb48

bb40:                                             ; No predecessors!
  %tmp41 = fmul float %tmp36, %tmp36
  %tmp42 = fmul float %tmp36, %tmp36
  %tmp43 = fmul float %tmp41, %tmp42
  %tmp44 = fmul float %tmp43, 0.000000e+00
  %tmp45 = fadd float 0.000000e+00, %tmp44
  %tmp46 = fadd float %tmp45, 0.000000e+00
  %tmp47 = fdiv float %tmp46, 0.000000e+00
  br label %bb48

bb48:                                             ; preds = %bb40, %bb39, %bb38, %bb37, %bb30
  %tmp49 = phi float [ %tmp47, %bb40 ], [ 0.000000e+00, %bb39 ], [ 0.000000e+00, %bb38 ], [ 0.000000e+00, %bb30 ], [ 0.000000e+00, %bb37 ]
  %tmp50 = fmul reassoc nsz arcp contract float %tmp49, 0.000000e+00
  %tmp51 = fadd reassoc nsz arcp contract float %tmp50, 0.000000e+00
  %tmp52 = fmul reassoc nsz arcp contract float %tmp51, 0.000000e+00
  %tmp53 = insertelement <8 x float> %tmp35, float %tmp52, i64 0
  %tmp54 = fmul reassoc nsz arcp contract float 0.000000e+00, 0.000000e+00
  br i1 false, label %bb55, label %bb66

bb55:                                             ; preds = %bb48
  br label %bb66

bb56:                                             ; No predecessors!
  br label %bb66

bb57:                                             ; No predecessors!
  br label %bb66

bb58:                                             ; No predecessors!
  %tmp59 = fmul float %tmp54, %tmp54
  %tmp60 = fmul float %tmp54, %tmp54
  %tmp61 = fmul float %tmp59, %tmp60
  %tmp62 = fmul float %tmp61, 0.000000e+00
  %tmp63 = fadd float 0.000000e+00, %tmp62
  %tmp64 = fadd float %tmp63, 0.000000e+00
  %tmp65 = fdiv float %tmp64, 0.000000e+00
  br label %bb66

bb66:                                             ; preds = %bb58, %bb57, %bb56, %bb55, %bb48
  %tmp67 = phi float [ %tmp65, %bb58 ], [ 0.000000e+00, %bb57 ], [ 0.000000e+00, %bb56 ], [ 0.000000e+00, %bb48 ], [ 0.000000e+00, %bb55 ]
  %tmp68 = fmul reassoc nsz arcp contract float %tmp67, 0.000000e+00
  %tmp69 = fadd reassoc nsz arcp contract float %tmp68, 0.000000e+00
  %tmp70 = fmul reassoc nsz arcp contract float %tmp69, 0.000000e+00
  %tmp71 = insertelement <8 x float> %tmp53, float %tmp70, i64 0
  %tmp72 = fmul reassoc nsz arcp contract float 0.000000e+00, 0.000000e+00
  br i1 false, label %bb73, label %bb84

bb73:                                             ; preds = %bb66
  br label %bb84

bb74:                                             ; No predecessors!
  br label %bb84

bb75:                                             ; No predecessors!
  br label %bb84

bb76:                                             ; No predecessors!
  %tmp77 = fmul float %tmp72, %tmp72
  %tmp78 = fmul float %tmp72, %tmp72
  %tmp79 = fmul float %tmp77, %tmp78
  %tmp80 = fmul float %tmp79, 0.000000e+00
  %tmp81 = fadd float 0.000000e+00, %tmp80
  %tmp82 = fadd float %tmp81, 0.000000e+00
  %tmp83 = fdiv float %tmp82, 0.000000e+00
  br label %bb84

bb84:                                             ; preds = %bb76, %bb75, %bb74, %bb73, %bb66
  %tmp85 = phi float [ %tmp83, %bb76 ], [ 0.000000e+00, %bb75 ], [ 0.000000e+00, %bb74 ], [ 0.000000e+00, %bb66 ], [ 0.000000e+00, %bb73 ]
  %tmp86 = fmul reassoc nsz arcp contract float %tmp85, 0.000000e+00
  %tmp87 = fadd reassoc nsz arcp contract float %tmp86, 0.000000e+00
  %tmp88 = fmul reassoc nsz arcp contract float %tmp87, 0.000000e+00
  %tmp89 = insertelement <8 x float> %tmp71, float %tmp88, i64 0
  %tmp90 = fmul reassoc nsz arcp contract float 0.000000e+00, 0.000000e+00
  br i1 false, label %bb91, label %bb102

bb91:                                             ; preds = %bb84
  br label %bb102

bb92:                                             ; No predecessors!
  br label %bb102

bb93:                                             ; No predecessors!
  br label %bb102

bb94:                                             ; No predecessors!
  %tmp95 = fmul float %tmp90, %tmp90
  %tmp96 = fmul float %tmp90, %tmp90
  %tmp97 = fmul float %tmp95, %tmp96
  %tmp98 = fmul float %tmp97, 0.000000e+00
  %tmp99 = fadd float 0.000000e+00, %tmp98
  %tmp100 = fadd float %tmp99, 0.000000e+00
  %tmp101 = fdiv float %tmp100, 0.000000e+00
  br label %bb102

bb102:                                            ; preds = %bb94, %bb93, %bb92, %bb91, %bb84
  %tmp103 = phi float [ %tmp101, %bb94 ], [ 0.000000e+00, %bb93 ], [ 0.000000e+00, %bb92 ], [ 0.000000e+00, %bb84 ], [ 0.000000e+00, %bb91 ]
  %tmp104 = fmul reassoc nsz arcp contract float %tmp103, 0.000000e+00
  %tmp105 = fadd reassoc nsz arcp contract float %tmp104, 0.000000e+00
  %tmp106 = fmul reassoc nsz arcp contract float %tmp105, 0.000000e+00
  %tmp107 = insertelement <8 x float> %tmp89, float %tmp106, i64 0
  %tmp108 = fmul reassoc nsz arcp contract float 0.000000e+00, 0.000000e+00
  br i1 false, label %bb109, label %bb120

bb109:                                            ; preds = %bb102
  br label %bb120

bb110:                                            ; No predecessors!
  br label %bb120

bb111:                                            ; No predecessors!
  br label %bb120

bb112:                                            ; No predecessors!
  %tmp113 = fmul float %tmp108, %tmp108
  %tmp114 = fmul float %tmp108, %tmp108
  %tmp115 = fmul float %tmp113, %tmp114
  %tmp116 = fmul float %tmp115, 0.000000e+00
  %tmp117 = fadd float 0.000000e+00, %tmp116
  %tmp118 = fadd float %tmp117, 0.000000e+00
  %tmp119 = fdiv float %tmp118, 0.000000e+00
  br label %bb120

bb120:                                            ; preds = %bb112, %bb111, %bb110, %bb109, %bb102
  %tmp121 = phi float [ %tmp119, %bb112 ], [ 0.000000e+00, %bb111 ], [ 0.000000e+00, %bb110 ], [ 0.000000e+00, %bb102 ], [ 0.000000e+00, %bb109 ]
  %tmp122 = fmul reassoc nsz arcp contract float %tmp121, 0.000000e+00
  %tmp123 = fadd reassoc nsz arcp contract float %tmp122, 0.000000e+00
  %tmp124 = fmul reassoc nsz arcp contract float %tmp123, 0.000000e+00
  %tmp125 = insertelement <8 x float> %tmp107, float %tmp124, i64 0
  %tmp126 = fmul reassoc nsz arcp contract float 0.000000e+00, 0.000000e+00
  br i1 false, label %bb127, label %bb138

bb127:                                            ; preds = %bb120
  br label %bb138

bb128:                                            ; No predecessors!
  br label %bb138

bb129:                                            ; No predecessors!
  br label %bb138

bb130:                                            ; No predecessors!
  %tmp131 = fmul float %tmp126, %tmp126
  %tmp132 = fmul float %tmp126, %tmp126
  %tmp133 = fmul float %tmp131, %tmp132
  %tmp134 = fmul float %tmp133, 0.000000e+00
  %tmp135 = fadd float 0.000000e+00, %tmp134
  %tmp136 = fadd float %tmp135, 0.000000e+00
  %tmp137 = fdiv float %tmp136, 0.000000e+00
  br label %bb138

bb138:                                            ; preds = %bb130, %bb129, %bb128, %bb127, %bb120
  %tmp139 = phi float [ %tmp137, %bb130 ], [ 0.000000e+00, %bb129 ], [ 0.000000e+00, %bb128 ], [ 0.000000e+00, %bb120 ], [ 0.000000e+00, %bb127 ]
  %tmp140 = fmul reassoc nsz arcp contract float %tmp139, 0.000000e+00
  %tmp141 = fadd reassoc nsz arcp contract float %tmp140, 0.000000e+00
  %tmp142 = fmul reassoc nsz arcp contract float %tmp141, 0.000000e+00
  %tmp143 = insertelement <8 x float> %tmp125, float %tmp142, i64 0
  %tmp144 = bitcast <8 x float> %tmp143 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x i32> %tmp144)
  ret void
}

; Function Attrs: inaccessiblememonly nofree nosync nounwind willreturn
declare void @llvm.assume(i1 noundef) #0

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.fma.f32(float, float, float) #1

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.floor.f32(float) #1

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #1

attributes #0 = { inaccessiblememonly nofree nosync nounwind willreturn }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{}
