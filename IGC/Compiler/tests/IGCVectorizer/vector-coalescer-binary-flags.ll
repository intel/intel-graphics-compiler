;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 -dce < %s 2>&1 | FileCheck %s

; Both halves come from %w and the first fsub reads the higher lanes, so the
; slice is rotated and the second (fast) fsub becomes the front. The coalesced
; fsub must keep only the flags shared by both halves (nnan), not take all
; flags from the front instruction.

; CHECK: ROTATE!
; CHECK: [[VECT_0:%.*]] = fsub nnan <16 x float>
; CHECK-NOT: fsub fast <16 x float>

; Function Attrs: convergent nounwind
define spir_kernel void @foo(<16 x float> %w, <8 x float> %in2, <8 x float> %in4) #0 {
entry:
  %hi = shufflevector <16 x float> %w, <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %lo = shufflevector <16 x float> %w, <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %sub_hi = fsub nnan <8 x float> %hi, %in2
  %sub_lo = fsub fast <8 x float> %lo, %in4
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %sub_hi)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %sub_lo)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x float>)

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
