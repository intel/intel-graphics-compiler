; UNSUPPORTED: system-windows

; RUN: igc_opt -S  --igc-vectorizer -dce < %s 2>&1 | FileCheck %s
; CHECK-NOT: %vectorized_phi

; ModuleID = 'reduced.ll'
source_filename = "initial_test.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @_attn_fwd() #0 {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %0
  %1 = phi half [ 0.000000e+00, %0 ], [ %35, %._crit_edge ]
  %2 = phi half [ 0.000000e+00, %0 ], [ %36, %._crit_edge ]
  %3 = phi half [ 0.000000e+00, %0 ], [ %37, %._crit_edge ]
  %4 = phi half [ 0.000000e+00, %0 ], [ %38, %._crit_edge ]
  %5 = phi half [ 0.000000e+00, %0 ], [ %39, %._crit_edge ]
  %6 = phi half [ 0.000000e+00, %0 ], [ %40, %._crit_edge ]
  %7 = phi half [ 0.000000e+00, %0 ], [ %41, %._crit_edge ]
  %8 = phi half [ 0.000000e+00, %0 ], [ %42, %._crit_edge ]
  %9 = call half @llvm.exp2.f32(half 0.000000e+00)
  %10 = call half @llvm.exp2.f32(half 0.000000e+00)
  %11 = call half @llvm.exp2.f32(half 0.000000e+00)
  %12 = call half @llvm.exp2.f32(half 0.000000e+00)
  %13 = call half @llvm.exp2.f32(half 0.000000e+00)
  %14 = call half @llvm.exp2.f32(half 0.000000e+00)
  %15 = call half @llvm.exp2.f32(half 0.000000e+00)
  %16 = call half @llvm.exp2.f32(half 0.000000e+00)
  %17 = fmul fast half %9, %1
  %18 = fmul fast half %10, %2
  %19 = fmul fast half %11, %3
  %20 = fmul fast half %12, %4
  %21 = fmul fast half %13, %5
  %22 = fmul fast half %14, %6
  %23 = fmul fast half %15, %7
  %24 = fmul fast half %16, %8
  %25 = insertelement <8 x half> zeroinitializer, half %17, i64 0
  %26 = insertelement <8 x half> %25, half %18, i64 1
  %27 = insertelement <8 x half> %26, half %19, i64 2
  %28 = insertelement <8 x half> %27, half %20, i64 3
  %29 = insertelement <8 x half> %28, half %21, i64 4
  %30 = insertelement <8 x half> %29, half %22, i64 5
  %31 = insertelement <8 x half> %30, half %23, i64 6
  %32 = insertelement <8 x half> %31, half %24, i64 7
  %33 = call <8 x half> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x half> %32, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %34 = call <8 x half> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x half> %33, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %35 = extractelement <8 x half> %34, i64 0
  %36 = extractelement <8 x half> %34, i64 1
  %37 = extractelement <8 x half> %34, i64 2
  %38 = extractelement <8 x half> %34, i64 3
  %39 = extractelement <8 x half> %34, i64 4
  %40 = extractelement <8 x half> %34, i64 5
  %41 = extractelement <8 x half> %34, i64 6
  %42 = extractelement <8 x half> %34, i64 7
  br label %._crit_edge
}

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x half> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x half>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare half @llvm.exp2.f32(half) #2

; uselistorder directives
uselistorder <8 x half> (<8 x half>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)* @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32, { 1, 0 }
uselistorder half (half)* @llvm.exp2.f32, { 7, 6, 5, 4, 3, 2, 1, 0 }

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }
