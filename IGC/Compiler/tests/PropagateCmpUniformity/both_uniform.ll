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
; Test: Both operands uniform - no transformation
; getUniformNonUniformPair returns false when op0Uniform == op1Uniform.
; Neither kernel argument is replaced.
; ============================================================================
; CHECK-LABEL: @test_both_uniform(
define spir_kernel void @test_both_uniform(i32 %K1, i32 %K2, ptr addrspace(1) %out) {
entry:
  %cmp = icmp eq i32 %K1, %K2
  br i1 %cmp, label %equal, label %not_equal

equal:
; CHECK-LABEL: equal:
; CHECK: %result = add i32 %K1, 1
  %result = add i32 %K1, 1
  store i32 %result, ptr addrspace(1) %out, align 4
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
!1 = !{ptr @test_both_uniform, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_both_uniform}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
