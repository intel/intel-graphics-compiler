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
; Test: Both operands non-uniform - no transformation
; getUniformNonUniformPair returns false when both values are non-uniform.
; %tid_x is non-uniform; %tid_y = %tid_x + 1 is also non-uniform.
; ============================================================================
; CHECK-LABEL: @test_both_nonuniform(
define spir_kernel void @test_both_nonuniform(ptr addrspace(1) %out) {
entry:
  %tid_x = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %tid_y = add i32 %tid_x, 1
  %cmp = icmp eq i32 %tid_x, %tid_y
  br i1 %cmp, label %equal, label %not_equal

equal:
; CHECK-LABEL: equal:
; CHECK: %result = add i32 %tid_x, 1
  %result = add i32 %tid_x, 1
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
!1 = !{ptr @test_both_nonuniform, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_both_nonuniform}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
