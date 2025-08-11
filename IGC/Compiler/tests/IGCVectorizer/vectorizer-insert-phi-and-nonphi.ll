; REQUIRES: regkeys
; RUN: igc_opt -S  --igc-vectorizer -dce < %s 2>&1 | FileCheck %s

; CHECK: call float @llvm.genx.GenISA.WaveAll.f32
; CHECK: call float @llvm.genx.GenISA.WaveAll.f32
; CHECK-DAG: [[final_val:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32

; CHECK-DAG: [[vec_insert0:%.*]] = insertelement <8 x float> undef
; CHECK-DAG: [[vec_insert1:%.*]] = insertelement <8 x float> [[vec_insert0]]
; CHECK-DAG: [[vec_insert2:%.*]] = insertelement <8 x float> [[vec_insert1]]
; CHECK-DAG: [[vec_insert3:%.*]] = insertelement <8 x float> [[vec_insert2]]
; CHECK-DAG: [[vec_insert4:%.*]] = insertelement <8 x float> [[vec_insert3]]
; CHECK-DAG: [[vec_insert5:%.*]] = insertelement <8 x float> [[vec_insert4]]
; CHECK-DAG: [[vec_insert6:%.*]] = insertelement <8 x float> [[vec_insert5]]
; CHECK-DAG: [[vec_insert7:%.*]] = insertelement <8 x float> [[vec_insert6]], float [[final_val]]
; CHECK-DAG: %vectorized_cast = fptrunc <8 x float> [[vec_insert7]] to <8 x half>


; ModuleID = 'reduced.ll'
source_filename = "initial_test.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @quux() {
.lr.ph:
  br label %._crit_edge333

._crit_edge333:                                   ; preds = %._crit_edge333, %.lr.ph
  %0 = phi float [ 0.000000e+00, %.lr.ph ], [ 1.250000e-01, %._crit_edge333 ]
  %1 = phi float [ 0.000000e+00, %.lr.ph ], [ 0x3FC7154760000000, %._crit_edge333 ]
  %2 = phi float [ 0.000000e+00, %.lr.ph ], [ 0x3FF7154760000000, %._crit_edge333 ]
  %3 = phi float [ 0.000000e+00, %.lr.ph ], [ 0xFFF0000000000000, %._crit_edge333 ]
  %4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %5 = extractelement <8 x float> %4, i64 2
  %6 = extractelement <8 x float> %4, i64 5
  %7 = extractelement <8 x float> %4, i64 6
  %8 = call float @llvm.genx.GenISA.WaveAll.f32(float 1.250000e-01, i8 12, i32 0)
  %9 = call float @llvm.genx.GenISA.WaveAll.f32(float 1.250000e-01, i8 12, i32 0)
  %10 = call float @llvm.genx.GenISA.WaveAll.f32(float 1.250000e-01, i8 12, i32 0)
  %11 = fptrunc float %0 to half
  %12 = fptrunc float %1 to half
  %13 = fptrunc float %5 to half
  %14 = fptrunc float %2 to half
  %15 = fptrunc float %3 to half
  %16 = fptrunc float %6 to half
  %17 = fptrunc float %7 to half
  %18 = fptrunc float %10 to half
  %19 = insertelement <8 x half> zeroinitializer, half %11, i64 0
  %20 = insertelement <8 x half> %19, half %12, i64 1
  %21 = insertelement <8 x half> %20, half %13, i64 2
  %22 = insertelement <8 x half> %21, half %14, i64 3
  %23 = insertelement <8 x half> %22, half %15, i64 4
  %24 = insertelement <8 x half> %23, half %16, i64 5
  %25 = insertelement <8 x half> %24, half %17, i64 6
  %26 = insertelement <8 x half> %25, half %18, i64 7
  %27 = bitcast <8 x half> %26 to <8 x i16>
  %28 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %27, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %._crit_edge333
}

declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i32)

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)


attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}
!0 = !{void ()* @quux, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
