;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer -dce --instcombine < %s | FileCheck %s

; two uniform 1/x <8 x float> fdivs are coalesced into a single <16 x float> fdiv,
; the coalescer concatenates the numerators with a shufflevector of two constant vectors,
; instcombine (run in AddLegalizationPasses before EmitPass) has to fold it back into
; a constant vector of ones, otherwise EmitPass::FDiv can't recognize it via isVectorOfOnes
; and emits div instead of inv

; inst combine provides that fold (we do have it later in the pipeline)

; vector constants are printed as an element list before LLVM 22 and as splat since then
; CHECK-LABEL: define spir_kernel void @foo(
; CHECK-NOT: shufflevector <8 x float> {{<float 1.000000e\+00|splat \(float 1.000000e\+00\)}}
; CHECK: [[DEN:%.*]] = shufflevector <8 x float> %in2, <8 x float> %in4, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK: [[VECT:%.*]] = fdiv fast <16 x float> {{<float 1.000000e\+00(, float 1.000000e\+00){15}>|splat \(float 1.000000e\+00\)}}, [[DEN]]
; CHECK: [[OUT_0:%.*]] = shufflevector <16 x float> [[VECT]], <16 x float> {{poison|undef}}, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK: [[OUT_1:%.*]] = shufflevector <16 x float> [[VECT]], <16 x float> {{poison|undef}}, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> [[OUT_0]])
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> [[OUT_1]])

; Function Attrs: convergent nounwind
define spir_kernel void @foo(<8 x float> %in2, <8 x float> %in4) #0 {
entry:
  %inv0 = fdiv fast <8 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %in2
  %inv1 = fdiv fast <8 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %in4
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %inv0)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %inv1)
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
