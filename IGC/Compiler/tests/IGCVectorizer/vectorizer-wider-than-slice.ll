;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; RUN: igc_opt --igc-vectorizer -S -dce < %s 2>&1 | FileCheck %s

define spir_kernel void @wibble(<64 x float> %arg) {
bb:
  br label %bb2

bb1:                                              ; No predecessors!
  br label %bb2

bb2:                                              ; preds = %bb1, %bb
  %tmp = phi <64 x float> [ zeroinitializer, %bb ], [ zeroinitializer, %bb1 ]
  %tmp3 = extractelement <64 x float> %arg, i64 0
  %tmp4 = extractelement <64 x float> %arg, i64 1
  %tmp5 = extractelement <64 x float> %arg, i64 2
  %tmp6 = extractelement <64 x float> %arg, i64 3
  %tmp7 = extractelement <64 x float> %arg, i64 4
  %tmp8 = extractelement <64 x float> %arg, i64 5
  %tmp9 = extractelement <64 x float> %arg, i64 6
  %tmp10 = extractelement <64 x float> %arg, i64 7
  %tmp11 = extractelement <64 x float> %arg, i64 8
  %tmp12 = extractelement <64 x float> %arg, i64 9
  %tmp13 = extractelement <64 x float> %arg, i64 10
  %tmp14 = extractelement <64 x float> %arg, i64 11
  %tmp15 = extractelement <64 x float> %arg, i64 12
  %tmp16 = extractelement <64 x float> %arg, i64 13
  %tmp17 = extractelement <64 x float> %arg, i64 14
  %tmp18 = extractelement <64 x float> %arg, i64 15
  br label %bb19

bb19:                                             ; preds = %bb36, %bb2
  ; CHECK-LABEL: bb19:
  ; CHECK-NOT: phi <8 x float>{{.*}}[ %arg, %bb2 ]
  ; CHECK: ret void
  %tmp20 = phi float [ %tmp18, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp21 = phi float [ %tmp17, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp22 = phi float [ %tmp16, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp23 = phi float [ %tmp15, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp24 = phi float [ %tmp14, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp25 = phi float [ %tmp13, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp26 = phi float [ %tmp12, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp27 = phi float [ %tmp11, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp28 = phi float [ %tmp10, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp29 = phi float [ %tmp9, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp30 = phi float [ %tmp8, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp31 = phi float [ %tmp7, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp32 = phi float [ %tmp6, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp33 = phi float [ %tmp5, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp34 = phi float [ %tmp4, %bb2 ], [ 0.000000e+00, %bb36 ]
  %tmp35 = phi float [ %tmp3, %bb2 ], [ 0.000000e+00, %bb36 ]
  ret void

bb36:                                             ; No predecessors!
  %tmp37 = insertelement <8 x float> zeroinitializer, float %tmp35, i64 0
  %tmp38 = insertelement <8 x float> %tmp37, float %tmp34, i64 0
  %tmp39 = insertelement <8 x float> %tmp38, float %tmp33, i64 0
  %tmp40 = insertelement <8 x float> %tmp39, float %tmp32, i64 0
  %tmp41 = insertelement <8 x float> %tmp40, float %tmp31, i64 0
  %tmp42 = insertelement <8 x float> %tmp41, float %tmp30, i64 0
  %tmp43 = insertelement <8 x float> %tmp42, float %tmp29, i64 0
  %tmp44 = insertelement <8 x float> %tmp43, float %tmp28, i64 0
  %tmp45 = insertelement <8 x float> zeroinitializer, float %tmp27, i64 0
  %tmp46 = insertelement <8 x float> %tmp45, float %tmp26, i64 0
  %tmp47 = insertelement <8 x float> %tmp46, float %tmp25, i64 0
  %tmp48 = insertelement <8 x float> %tmp47, float %tmp24, i64 0
  %tmp49 = insertelement <8 x float> %tmp48, float %tmp23, i64 0
  %tmp50 = insertelement <8 x float> %tmp49, float %tmp22, i64 0
  %tmp51 = insertelement <8 x float> %tmp50, float %tmp21, i64 0
  %tmp52 = insertelement <8 x float> %tmp51, float %tmp20, i64 0
  %tmp53 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp44, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp54 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp52, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %bb19
}

; Function Attrs: inaccessiblememonly nofree nosync nounwind willreturn
declare void @llvm.assume(i1 noundef) #0

declare i16 @llvm.genx.GenISA.simdLaneId()

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

attributes #0 = { inaccessiblememonly nofree nosync nounwind willreturn }

!igc.functions = !{}
