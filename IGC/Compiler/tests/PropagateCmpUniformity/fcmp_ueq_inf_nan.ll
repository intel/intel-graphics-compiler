;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -igc-propagate-cmp-uniformity -S < %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; ============================================================================
; Test: fcmp ueq against +inf

; Reduced from the atanh BiF special-case path:
;   %abs = fabs(%x)
;   if (%abs ueq +inf)       ; true for +/-inf AND for NaN
;     return isinf ? NaN : %abs
;
; In the true block %abs may be NaN, so it must NOT be replaced with +inf.
; Doing so made atanh(NaN) return +inf.
; ============================================================================
; CHECK-LABEL: @test_fcmp_ueq_inf_nan(
define spir_kernel void @test_fcmp_ueq_inf_nan(ptr addrspace(1) %in, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %idx = zext i32 %tid to i64
  %ptr = getelementptr inbounds float, ptr addrspace(1) %in, i64 %idx
  %x = load float, ptr addrspace(1) %ptr, align 4
  %abs = call float @llvm.fabs.f32(float %x)
  %bits = bitcast float %abs to i32
  %cmp = fcmp ueq float %abs, 0x7FF0000000000000
  br i1 %cmp, label %inf_or_nan, label %exit

inf_or_nan:
; CHECK-LABEL: inf_or_nan:
; CHECK: %res = select i1 %isinf, float {{[^,]+}}, float %abs
  %isinf = icmp eq i32 %bits, 2139095040
  %res = select i1 %isinf, float 0x7FF8000000000000, float %abs
  store float %res, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0
declare float @llvm.fabs.f32(float) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_fcmp_ueq_inf_nan, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_fcmp_ueq_inf_nan}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
