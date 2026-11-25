;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Private memory of exactly the scratch space limit per work item (256 KB / SIMD32) leaves no scratch space for
; spills, so it is placed in stateless memory.
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers --ocl --platformbmg --igc-private-mem-resolution -S %s 2>&1 | FileCheck %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers --ocl --platformbmg --igc-private-mem-resolution -S %s 2>&1 | FileCheck %s %}

define spir_kernel void @test(float addrspace(1)* %out, i32 %i, i8* %privateBase) {
; CHECK-LABEL: @test(
; CHECK:         [[SIMD_SIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:         call i32 @llvm.genx.GenISA.hw.thread.id.alloca.i32()
; CHECK:         mul i32 [[SIMD_SIZE]], 8192
; CHECK:         %privateBase
  %a = alloca [2048 x float], align 4
  %p = getelementptr [2048 x float], [2048 x float]* %a, i32 0, i32 %i
  store float 1.0, float* %p, align 4
  %q = getelementptr [2048 x float], [2048 x float]* %a, i32 0, i32 0
  %v = load float, float* %q, align 4
  store float %v, float addrspace(1)* %out, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !11}
!1 = !{void (float addrspace(1)*, i32, i8*)* @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
!6 = !{!"argId", i32 13}
!7 = !{!"implicitArgInfoListVec[0]", !6}
!8 = !{!"implicitArgInfoList", !7}
!9 = !{!"FuncMDMap[0]", void (float addrspace(1)*, i32, i8*)* @test}
!10 = !{!"FuncMDValue[0]", !8}
!11 = !{!"FuncMD", !9, !10}
