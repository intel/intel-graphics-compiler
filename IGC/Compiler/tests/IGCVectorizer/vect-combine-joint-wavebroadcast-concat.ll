;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vect-combine --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 -dce < %s 2>&1 | FileCheck %s

; both halves alias the register %src lives in, the concatenation is
; a wider view of %src, so the shuffle collapses into a single broadcast

; CHECK-LABEL: define spir_kernel void @foo
; CHECK: [[WIDE:%.*]] = call <16 x float> @llvm.genx.GenISA.JointWaveBroadcast.v16f32.f32.v16i32(float %src, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, i32 0)
; CHECK-NEXT: [[ADD:%.*]] = fadd fast <16 x float> [[WIDE]], %other
; CHECK-NEXT: call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32({{.*}}, <16 x float> [[ADD]])
; CHECK-NEXT: ret void

; Function Attrs: convergent nounwind
define spir_kernel void @foo(float %src, <16 x float> %other) #0 {
entry:
  %high = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, i32 0)
  %low = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, i32 0)
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
