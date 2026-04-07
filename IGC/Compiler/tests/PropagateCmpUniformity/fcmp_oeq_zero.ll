;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -igc-propagate-cmp-uniformity -S < %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; ============================================================================
; Test: fcmp oeq against +/-0.0 — no replacement (sign-bit safety)
;
; When the uniform operand is +/-0.0, the non-uniform value on the equality
; path may still be -0.0 or +0.0. Replacing it with the constant would corrupt
; sign-bit operations such as the bitcast/AND/bitcast sequence.
;
; In the equality branch, %val must NOT be replaced with 0.0:
;   %signbits = bitcast float %val to i32
;   %masked   = and i32 %signbits, -2147483648   ; 0x80000000
;   %sign     = bitcast i32 %masked to float
; If %val were replaced with +0.0, %sign would always be +0.0, incorrectly
; dropping the sign when %val was -0.0.
; ============================================================================
; CHECK-LABEL: @test_fcmp_oeq_zero(
define spir_kernel void @test_fcmp_oeq_zero(ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %val = uitofp i32 %tid to float
  %cmp = fcmp oeq float %val, 0.000000e+00
  br i1 %cmp, label %equal, label %not_equal

equal:
; CHECK-LABEL: equal:
; CHECK: %signbits = bitcast float %val to i32
; CHECK-NOT: bitcast float 0.000000e+00 to i32
  %signbits = bitcast float %val to i32
  %masked   = and i32 %signbits, -2147483648
  %sign     = bitcast i32 %masked to float
  store float %sign, ptr addrspace(1) %out, align 4
  br label %exit

not_equal:
  br label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_fcmp_oeq_zero, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_fcmp_oeq_zero}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
