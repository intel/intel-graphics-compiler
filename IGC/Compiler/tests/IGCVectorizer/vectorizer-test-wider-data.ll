; RUN: igc_opt -S  --igc-vectorizer -dce < %s 2>&1 | FileCheck %s

; ModuleID = 'reduced.ll'
source_filename = "initial_test.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @quux(< 16 x float>* %ptr) {

  %bulkData = load <16 x float>, <16 x float>* %ptr

  %extractedValue_0 = extractelement <16 x float> %bulkData, i64 0
  %extractedValue_1 = extractelement <16 x float> %bulkData, i64 1
  %extractedValue_2 = extractelement <16 x float> %bulkData, i64 2
  %extractedValue_3 = extractelement <16 x float> %bulkData, i64 3
  %extractedValue_4 = extractelement <16 x float> %bulkData, i64 4
  %extractedValue_5 = extractelement <16 x float> %bulkData, i64 5
  %extractedValue_6 = extractelement <16 x float> %bulkData, i64 6
  %extractedValue_7 = extractelement <16 x float> %bulkData, i64 7

  %extractedValue_8 = extractelement <16 x float> %bulkData, i64 8
  %extractedValue_9 = extractelement <16 x float> %bulkData, i64 9
  %extractedValue_10 = extractelement <16 x float> %bulkData, i64 10
  %extractedValue_11 = extractelement <16 x float> %bulkData, i64 11
  %extractedValue_12 = extractelement <16 x float> %bulkData, i64 12
  %extractedValue_13 = extractelement <16 x float> %bulkData, i64 13
  %extractedValue_14 = extractelement <16 x float> %bulkData, i64 14
  %extractedValue_15 = extractelement <16 x float> %bulkData, i64 15

  br label %._crit_edge

  ; CHECK-LABEL: ._crit_edge:
._crit_edge:                                      ; preds = %._crit_edge, %0

  ; CHECK-NOT: phi <8 x float>{{.*}}
  %1 = phi float [ %extractedValue_8, %0 ],  [ %a35, %._crit_edge ]
  %2 = phi float [ %extractedValue_9, %0 ],  [ %a36, %._crit_edge ]
  %3 = phi float [ %extractedValue_10, %0 ], [ %a37, %._crit_edge ]
  %4 = phi float [ %extractedValue_11, %0 ], [ %a38, %._crit_edge ]
  %5 = phi float [ %extractedValue_12, %0 ], [ %a39, %._crit_edge ]
  %6 = phi float [ %extractedValue_13, %0 ], [ %a40, %._crit_edge ]
  %7 = phi float [ %extractedValue_14, %0 ], [ %a41, %._crit_edge ]
  %8 = phi float [ %extractedValue_15, %0 ], [ %a42, %._crit_edge ]

  ; CHECK-NOT: fmul fast <8 x float>{{.*}}
  %a17 = fmul fast float %1, %extractedValue_0
  %a18 = fmul fast float %2, %extractedValue_1
  %a19 = fmul fast float %3, %extractedValue_2
  %a20 = fmul fast float %4, %extractedValue_3
  %a21 = fmul fast float %5, %extractedValue_4
  %a22 = fmul fast float %6, %extractedValue_5
  %a23 = fmul fast float %7, %extractedValue_6
  %a24 = fmul fast float %8, %extractedValue_7

  %a25 = insertelement <8 x float> zeroinitializer, float %a17, i64 0
  %a26 = insertelement <8 x float> %a25, float %a18, i64 1
  %a27 = insertelement <8 x float> %a26, float %a19, i64 2
  %a28 = insertelement <8 x float> %a27, float %a20, i64 3
  %a29 = insertelement <8 x float> %a28, float %a21, i64 4
  %a30 = insertelement <8 x float> %a29, float %a22, i64 5
  %a31 = insertelement <8 x float> %a30, float %a23, i64 6
  %a32 = insertelement <8 x float> %a31, float %a24, i64 7
  %a33 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %a32, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %a34 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %a33, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %a35 = extractelement <8 x float> %a34, i64 0
  %a36 = extractelement <8 x float> %a34, i64 1
  %a37 = extractelement <8 x float> %a34, i64 2
  %a38 = extractelement <8 x float> %a34, i64 3
  %a39 = extractelement <8 x float> %a34, i64 4
  %a40 = extractelement <8 x float> %a34, i64 5
  %a41 = extractelement <8 x float> %a34, i64 6
  %a42 = extractelement <8 x float> %a34, i64 7
  ; CHECK: br label %._crit_edge
  br label %._crit_edge
}

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #2

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}
!0 = !{void (<16 x float>* )* @quux, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
