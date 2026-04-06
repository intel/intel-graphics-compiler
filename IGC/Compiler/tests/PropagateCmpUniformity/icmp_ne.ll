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
; Test: icmp ne
; For ICMP_NE the pass maps trueBranch = successor(1) (the block reached when
; ne is false, i.e. when equality actually holds). Replacement must happen in
; eq_branch, not in ne_branch.
; ============================================================================
; CHECK-LABEL: @test_icmp_ne(
define spir_kernel void @test_icmp_ne(i32 %K, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %cmp = icmp ne i32 %tid, %K
  br i1 %cmp, label %ne_branch, label %eq_branch

ne_branch:
; CHECK-LABEL: ne_branch:
; CHECK: %result = add i32 %tid, 1
  %result = add i32 %tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

eq_branch:
; CHECK-LABEL: eq_branch:
; CHECK: %result2 = add i32 %K, 1
  %result2 = add i32 %tid, 1
  store i32 %result2, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_icmp_ne, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_icmp_ne}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
