
; REQUIRES: regkeys
; RUN: igc_opt -S  --igc-vectorizer -dce --regkey=VectorizerAllowEXP2=1 --regkey=VectorizerAllowMAXNUM=1 < %s 2>&1 | FileCheck %s

; CHECK: %vectorized_phi
; CHECK: %vectorized_intrinsic23 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %vectorized_phi)
; CHECK: %vectorized_intrinsic = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %vectorized_intrinsic23, <8 x float> %vectorized_phi)
; CHECK: %vectorized_binary = fmul fast <8 x float> %vectorized_intrinsic23, %vectorized_intrinsic
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vectorized_binary

; ModuleID = 'reduced.ll'
source_filename = "initial_test.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @quux() {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %0
  %1 = phi float [ 0.000000e+00, %0 ], [ %35, %._crit_edge ]
  %2 = phi float [ 0.000000e+00, %0 ], [ %36, %._crit_edge ]
  %3 = phi float [ 0.000000e+00, %0 ], [ %37, %._crit_edge ]
  %4 = phi float [ 0.000000e+00, %0 ], [ %38, %._crit_edge ]
  %5 = phi float [ 0.000000e+00, %0 ], [ %39, %._crit_edge ]
  %6 = phi float [ 0.000000e+00, %0 ], [ %40, %._crit_edge ]
  %7 = phi float [ 0.000000e+00, %0 ], [ %41, %._crit_edge ]
  %8 = phi float [ 0.000000e+00, %0 ], [ %42, %._crit_edge ]
  %9  = call float @llvm.exp2.f32(float %1)
  %10 = call float @llvm.exp2.f32(float %2)
  %11 = call float @llvm.exp2.f32(float %3)
  %12 = call float @llvm.exp2.f32(float %4)
  %13 = call float @llvm.exp2.f32(float %5)
  %14 = call float @llvm.exp2.f32(float %6)
  %15 = call float @llvm.exp2.f32(float %7)
  %16 = call float @llvm.exp2.f32(float %8)
  %maxnum0 = call float @llvm.maxnum.f32(float %9, float %1)
  %maxnum1 = call float @llvm.maxnum.f32(float %10, float %2)
  %maxnum2 = call float @llvm.maxnum.f32(float %11, float %3)
  %maxnum3 = call float @llvm.maxnum.f32(float %12, float %4)
  %maxnum4 = call float @llvm.maxnum.f32(float %13, float %5)
  %maxnum5 = call float @llvm.maxnum.f32(float %14, float %6)
  %maxnum6 = call float @llvm.maxnum.f32(float %15, float %7)
  %maxnum7 = call float @llvm.maxnum.f32(float %16, float %8)
  %17 = fmul fast float %9,  %maxnum0
  %18 = fmul fast float %10, %maxnum1
  %19 = fmul fast float %11, %maxnum2
  %20 = fmul fast float %12, %maxnum3
  %21 = fmul fast float %13, %maxnum4
  %22 = fmul fast float %14, %maxnum5
  %23 = fmul fast float %15, %maxnum6
  %24 = fmul fast float %16, %maxnum7
  %25 = insertelement <8 x float> zeroinitializer, float %17, i64 0
  %26 = insertelement <8 x float> %25, float %18, i64 1
  %27 = insertelement <8 x float> %26, float %19, i64 2
  %28 = insertelement <8 x float> %27, float %20, i64 3
  %29 = insertelement <8 x float> %28, float %21, i64 4
  %30 = insertelement <8 x float> %29, float %22, i64 5
  %31 = insertelement <8 x float> %30, float %23, i64 6
  %32 = insertelement <8 x float> %31, float %24, i64 7
  %33 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %32, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %34 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %33, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %35 = extractelement <8 x float> %34, i64 0
  %36 = extractelement <8 x float> %34, i64 1
  %37 = extractelement <8 x float> %34, i64 2
  %38 = extractelement <8 x float> %34, i64 3
  %39 = extractelement <8 x float> %34, i64 4
  %40 = extractelement <8 x float> %34, i64 5
  %41 = extractelement <8 x float> %34, i64 6
  %42 = extractelement <8 x float> %34, i64 7
  br label %._crit_edge
}

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #2

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.maxnum.f32(float, float) #6

; uselistorder directives
uselistorder <8 x float> (<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)* @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32, { 1, 0 }
uselistorder float (float)* @llvm.exp2.f32, { 7, 6, 5, 4, 3, 2, 1, 0 }

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}
!0 = !{void ()* @quux, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
