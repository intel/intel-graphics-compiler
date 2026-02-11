;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 -dce --instcombine < %s 2>&1 | FileCheck %s

; CHECK-LABEL: define spir_kernel void @foo

; CHECK: [[IN_0:%.*]] = shufflevector <8 x float> %tmp, <8 x float> %tmp17, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK: [[IN_1:%.*]] = shufflevector <8 x float> %tmp34, <8 x float> %tmp38, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK: [[INTR_0:%.*]] = call <16 x float> @llvm.maxnum.v16f32(<16 x float> [[IN_0]], <16 x float> [[IN_1]])
; CHECK: shufflevector <16 x float> [[INTR_0]], <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK: shufflevector <16 x float> [[INTR_0]], <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK: [[IN_2:%.*]] = shufflevector <8 x float> %tmp, <8 x float> %tmp17, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK: [[BIN_0:%.*]] = fsub <16 x float> [[IN_2]], [[INTR_0]]
; CHECK: call <16 x float> @llvm.exp2.v16f32(<16 x float> [[BIN_0]])

; Function Attrs: convergent nounwind
define spir_kernel void @foo() #0 {
bb:
  br label %bb16

bb16:                                             ; preds = %bb16, %bb
  %tmp = phi <8 x float> [ <float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000>, %bb ], [ %tmp35, %bb16 ]
  %tmp17 = phi <8 x float> [ <float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000>, %bb ], [ %tmp39, %bb16 ]
  %tmp18 = phi float [ 0.000000e+00, %bb ], [ %tmp50, %bb16 ]
  %tmp19 = phi float [ 0.000000e+00, %bb ], [ %tmp51, %bb16 ]
  %tmp20 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %tmp21 = fmul contract <8 x float> %tmp20, <float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01>
  %tmp22 = fmul contract <8 x float> %tmp21, <float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000>
  %tmp23 = extractelement <8 x float> %tmp22, i32 4
  %tmp24 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %tmp25 = fmul contract <8 x float> %tmp24, <float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01>
  %tmp26 = fmul contract <8 x float> %tmp25, <float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000>
  %tmp27 = extractelement <8 x float> %tmp26, i32 7
  %tmp28 = insertelement <8 x float> zeroinitializer, float %tmp23, i32 4
  %tmp29 = insertelement <8 x float> %tmp28, float 0.000000e+00, i32 5
  %tmp30 = insertelement <8 x float> %tmp29, float 0.000000e+00, i32 6
  %tmp31 = insertelement <8 x float> %tmp30, float 0.000000e+00, i32 7
  %tmp32 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> zeroinitializer, <8 x float> %tmp31)
  %tmp33 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %tmp32, <8 x float> zeroinitializer)
  %tmp34 = call <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float> %tmp33, i8 12, i32 0)
  %tmp35 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %tmp, <8 x float> %tmp34)
  %tmp36 = insertelement <8 x float> zeroinitializer, float %tmp27, i32 7
  %tmp37 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> zeroinitializer, <8 x float> %tmp36)
  %tmp38 = call <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float> %tmp37, i8 12, i32 0)
  %tmp39 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %tmp17, <8 x float> %tmp38)
  %tmp40 = select <8 x i1> zeroinitializer, <8 x float> zeroinitializer, <8 x float> %tmp35
  %tmp41 = fsub <8 x float> %tmp, %tmp40
  %tmp42 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %tmp41)
  %tmp43 = extractelement <8 x float> %tmp42, i32 0
  %tmp44 = select <8 x i1> zeroinitializer, <8 x float> zeroinitializer, <8 x float> %tmp39
  %tmp45 = fsub <8 x float> %tmp17, %tmp44
  %tmp46 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %tmp45)
  %tmp47 = extractelement <8 x float> %tmp46, i32 0
  %tmp48 = fmul contract float %tmp18, %tmp43
  %tmp49 = fmul contract float %tmp19, %tmp47
  %tmp50 = fadd contract float %tmp48, 0.000000e+00
  %tmp51 = fadd contract float %tmp49, 0.000000e+00
  br label %bb16

bb52:                                             ; No predecessors!
  %tmp53 = call <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float> zeroinitializer, i8 12, i32 0)
  %tmp54 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %tmp35, <8 x float> %tmp53)
  %tmp55 = fsub <8 x float> zeroinitializer, zeroinitializer
  %tmp56 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %tmp55)
  %tmp57 = fsub <8 x float> zeroinitializer, zeroinitializer
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.fma.f32(float, float, float) #1

; Function Attrs: convergent nounwind memory(inaccessiblemem: readwrite)
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i32) #2

; Function Attrs: convergent nounwind willreturn memory(none)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #3

; Function Attrs: nounwind memory(readwrite)
declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #4

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.umin.i32(i32, i32) #1

; Function Attrs: nounwind speculatable willreturn memory(none)
declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32) #5

; Function Attrs: nounwind speculatable willreturn memory(write)
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1) #6

; Function Attrs: nounwind willreturn memory(readwrite)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nounwind willreturn memory(readwrite)
declare <32 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nounwind willreturn memory(readwrite)
declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>) #1

; Function Attrs: convergent nounwind memory(inaccessiblemem: readwrite)
declare <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float>, i8, i32) #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x float> @llvm.exp2.v8f32(<8 x float>) #1

attributes #0 = { convergent nounwind }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { convergent nounwind memory(inaccessiblemem: readwrite) }
attributes #3 = { convergent nounwind willreturn memory(none) }
attributes #4 = { nounwind memory(readwrite) }
attributes #5 = { nounwind speculatable willreturn memory(none) }
attributes #6 = { nounwind speculatable willreturn memory(write) }
attributes #7 = { nounwind willreturn memory(readwrite) }

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2, !29}
!2 = !{!"function_type", i32 0}
!29 = !{!"sub_group_size", i32 16}
