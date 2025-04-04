;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers --inputps --platformskl --BlendToDiscard -igc-serialize-metadata -S < %s | FileCheck %s
; ------------------------------------------------
; BlendToDiscard
; ------------------------------------------------

declare void @llvm.genx.GenISA.OUTPUT.f32(float, float, float, float, i32, i32, i32)
declare < 2 x float > @llvm.genx.GenISA.sampleKillPix.2f32.f32(float, float, float, float addrspace(1)*, float addrspace(1)*, i32, i32, i32)

define spir_kernel void @test_func(float addrspace(1)* %idx, float addrspace(1)* %smp) {
; CHECK-LABEL: @test_func(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = call <2 x float> @llvm.genx.GenISA.sampleKillPix.2f32.f32(float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float addrspace(1)* [[IDX:%.*]], float addrspace(1)* [[SMP:%.*]], i32 0, i32 0, i32 0)
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x float> [[TMP0]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = fcmp oeq float [[TMP1]], 0.000000e+00
; CHECK-NEXT:    call void @llvm.genx.GenISA.discard(i1 [[TMP2]])
; CHECK-NEXT:    [[TMP3:%.*]] = fmul float 1.000000e+00, [[TMP1]]
; CHECK-NEXT:    call void @llvm.genx.GenISA.OUTPUT.f32(float undef, float undef, float undef, float [[TMP3]], i32 0, i32 0, i32 15)
; CHECK-NEXT:    ret void
;
entry:
  %0 = call < 2 x float > @llvm.genx.GenISA.sampleKillPix.2f32.f32(float 1.0, float 1.0, float 1.0, float addrspace(1)* %idx, float addrspace(1)* %smp, i32 0, i32 0, i32 0)
  %1 = extractelement < 2 x float > %0, i32 0

  %2 = fmul float 1.0, %1
  call void @llvm.genx.GenISA.OUTPUT.f32(float undef, float undef, float undef, float %2, i32 0, i32 0, i32 15)

  ret void
}

; CHECK-DAG: declare void @llvm.genx.GenISA.discard(i1)
; CHECK-DAG: !{{[0-9]*}} = !{!"forceEarlyZ", i1 true}

!IGCMetadata = !{!0}
!igc.functions = !{!9}

!0 = !{!"ModuleMD", !1, !4}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (float addrspace(1)*, float addrspace(1)*)* @test_func}
!3 = !{!"FuncMDValue[0]"}
!4 = !{!"psInfo", !5, !8}
!5 = !{!"blendOptimizationMode", !6, !7}
!6 = !{!"blendOptimizationModeVec[0]", i32 1} ; BLEND_OPTIMIZATION_SRC_ALPHA
!7 = !{!"blendOptimizationModeVec[1]", i32 2} ; BLEND_OPTIMIZATION_INV_SRC_ALPHA
!8 = !{!"forceEarlyZ", i1 false}
!9 = !{void (float addrspace(1)*, float addrspace(1)*)* @test_func, !10}
!10 = !{!11}
!11 = !{!"function_type", i32 0}
