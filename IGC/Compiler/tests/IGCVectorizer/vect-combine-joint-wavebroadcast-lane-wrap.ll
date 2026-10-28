;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vect-combine --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 -dce < %s 2>&1 | FileCheck %s

; the low half broadcasts lanes -8..-1, which are out of bounds for the simd
; line. Lane indices are compared as unsigned, so without the first lane
; check -8 + 16 would wrap to 8 and the merged range would pass the bounds
; check. The shuffle has to stay as it is: a merged broadcast over lanes
; -8..7 would be emitted as an alias that reads src(255,8) without any
; diagnostic

; CHECK-LABEL: define spir_kernel void @foo
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: [[CAT:%.*]] = shufflevector <8 x float> %low, <8 x float> %high
; CHECK: fadd fast <16 x float> [[CAT]], %other
; CHECK: ret void

; Function Attrs: convergent nounwind
define spir_kernel void @foo(float %src, <16 x float> %other) #0 {
entry:
  %low = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 -8, i32 -7, i32 -6, i32 -5, i32 -4, i32 -3, i32 -2, i32 -1>, i32 0)
  %high = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, i32 0)
  %coalesced_input = shufflevector <8 x float> %low, <8 x float> %high, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %coalesced_binary = fadd fast <16 x float> %coalesced_input, %other
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <16 x float> %coalesced_binary)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float, <8 x i32>, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <16 x float>)

attributes #0 = { convergent nounwind }

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{!"requiredSubGroupSize", i32 16}
!4 = !{!"FuncMDValue[0]", !3}
!5 = !{!"FuncMDMap[0]", ptr @foo}
!6 = !{!"FuncMD", !5, !4}
!7 = !{!"ModuleMD", !6}
!IGCMetadata = !{!7}
