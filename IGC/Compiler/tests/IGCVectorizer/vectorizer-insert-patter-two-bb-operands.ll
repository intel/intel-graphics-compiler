
; REQUIRES: regkeys
; RUN: igc_opt -S  --igc-vectorizer -dce < %s 2>&1 | FileCheck %s

; CHECK-LABEL: ._crit_edge
; CHECK: [[fmul:%.*]] = fmul fast <8 x float>

; CHECK-LABEL: bb123
; CHECK-DAG: [[fdiv0:%.*]] = fdiv fast <8 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, [[fmul]]
; CHECK-DAG: [[fdiv1:%.*]] = fdiv fast <8 x float> [[fdiv0]], [[fmul]]
; CHECK-DAG: [[fadd0:%.*]] = fadd fast <8 x float> [[fdiv0]], [[fdiv1]]
; CHECK-DAG: bitcast <8 x float> [[fdiv0]] to <8 x i32>
; CHECK-DAG: bitcast <8 x float> [[fdiv1]] to <8 x i32>
; CHECK-DAG: bitcast <8 x float> [[fadd0]] to <8 x i32>

; CHECK-LABEL: ret void


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
  %9 = call float @llvm.exp2.f32(float 0.000000e+00)
  %10 = call float @llvm.exp2.f32(float 0.000000e+00)
  %11 = call float @llvm.exp2.f32(float 0.000000e+00)
  %12 = call float @llvm.exp2.f32(float 0.000000e+00)
  %13 = call float @llvm.exp2.f32(float 0.000000e+00)
  %14 = call float @llvm.exp2.f32(float 0.000000e+00)
  %15 = call float @llvm.exp2.f32(float 0.000000e+00)
  %16 = call float @llvm.exp2.f32(float 0.000000e+00)
  %17 = fmul fast float %9, %1
  %18 = fmul fast float %10, %2
  %19 = fmul fast float %11, %3
  %20 = fmul fast float %12, %4
  %21 = fmul fast float %13, %5
  %22 = fmul fast float %14, %6
  %23 = fmul fast float %15, %7
  %24 = fmul fast float %16, %8
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
  br i1 false, label %._crit_edge, label %bb123

bb123:
  ; this vectorized must stay in the same BB, even though one of his operands
  ; is in the another
  %tmp100 = fdiv fast float 1.000000e+00, %17
  %tmp101 = fdiv fast float 1.000000e+00, %18
  %tmp102 = fdiv fast float 1.000000e+00, %19
  %tmp103 = fdiv fast float 1.000000e+00, %20
  %tmp104 = fdiv fast float 1.000000e+00, %21
  %tmp105 = fdiv fast float 1.000000e+00, %22
  %tmp106 = fdiv fast float 1.000000e+00, %23
  %tmp107 = fdiv fast float 1.000000e+00, %24

  %tmp143 = insertelement <8 x float> zeroinitializer, float %tmp100, i64 0
  %tmp144 = insertelement <8 x float> %tmp143, float %tmp101, i64 1
  %tmp145 = insertelement <8 x float> %tmp144, float %tmp102, i64 2
  %tmp146 = insertelement <8 x float> %tmp145, float %tmp103, i64 3
  %tmp147 = insertelement <8 x float> %tmp146, float %tmp104, i64 4
  %tmp148 = insertelement <8 x float> %tmp147, float %tmp105, i64 5
  %tmp149 = insertelement <8 x float> %tmp148, float %tmp106, i64 6
  %tmp150 = insertelement <8 x float> %tmp149, float %tmp107, i64 7
  %tmp151 = bitcast <8 x float> %tmp150 to <8 x i32>

  ; this vectorized must be put directly under one of its operands
  %tmp108 = fdiv fast float %tmp100, %17
  %tmp109 = fdiv fast float %tmp101, %18
  %tmp110 = fdiv fast float %tmp102, %19
  %tmp111 = fdiv fast float %tmp103, %20

  %add0_0 = fadd fast float %tmp100, %tmp108
  %add0_1 = fadd fast float %tmp101, %tmp109
  %add0_2 = fadd fast float %tmp102, %tmp110
  %add0_3 = fadd fast float %tmp103, %tmp111

  %tmp112 = fdiv fast float %tmp104, %21
  %tmp113 = fdiv fast float %tmp105, %22
  %tmp114 = fdiv fast float %tmp106, %23
  %tmp115 = fdiv fast float %tmp107, %24

  ; this vectorized must be put directly under one of its operands
  %add0_4 = fadd fast float %tmp104, %tmp112
  %add0_5 = fadd fast float %tmp105, %tmp113
  %add0_6 = fadd fast float %tmp106, %tmp114
  %add0_7 = fadd fast float %tmp107, %tmp115

  %inst3_0 = insertelement <8 x float> zeroinitializer, float %add0_0, i64 0
  %inst3_1 = insertelement <8 x float> %inst3_0, float %add0_1, i64 1
  %inst3_2 = insertelement <8 x float> %inst3_1, float %add0_2, i64 2
  %inst3_3 = insertelement <8 x float> %inst3_2, float %add0_3, i64 3
  %inst3_4 = insertelement <8 x float> %inst3_3, float %add0_4, i64 4
  %inst3_5 = insertelement <8 x float> %inst3_4, float %add0_5, i64 5
  %inst3_6 = insertelement <8 x float> %inst3_5, float %add0_6, i64 6
  %inst3_7 = insertelement <8 x float> %inst3_6, float %add0_7, i64 7
  %btc3 = bitcast <8 x float> %inst3_7 to <8 x i32>


  %inst2_0 = insertelement <8 x float> zeroinitializer, float %tmp108, i64 0
  %inst2_1 = insertelement <8 x float> %inst2_0, float %tmp109, i64 1
  %inst2_2 = insertelement <8 x float> %inst2_1, float %tmp110, i64 2
  %inst2_3 = insertelement <8 x float> %inst2_2, float %tmp111, i64 3
  %inst2_4 = insertelement <8 x float> %inst2_3, float %tmp112, i64 4
  %inst2_5 = insertelement <8 x float> %inst2_4, float %tmp113, i64 5
  %inst2_6 = insertelement <8 x float> %inst2_5, float %tmp114, i64 6
  %inst2_7 = insertelement <8 x float> %inst2_6, float %tmp115, i64 7

  %btc2 = bitcast <8 x float> %inst2_7 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x i32> %btc3)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x i32> %tmp151)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x i32> %btc2)
  ret void
}

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #2

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
