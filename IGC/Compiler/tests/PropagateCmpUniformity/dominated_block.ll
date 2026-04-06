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
; Test: Replacement in blocks transitively dominated by trueBranch
; Any block whose dominator chain includes trueBranch sees the replacement.
; Both %dominated and %also_dominated are post-trueBranch, so %tid -> %K.
; ============================================================================
; CHECK-LABEL: @test_dominated_block(
define spir_kernel void @test_dominated_block(i32 %K, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %trueBranch, label %falseBranch

trueBranch:
  %inner_cond = icmp slt i32 %K, 10
  br i1 %inner_cond, label %dominated, label %also_dominated

dominated:
; CHECK-LABEL: dominated:
; CHECK: %result = add i32 %K, 5
  %result = add i32 %tid, 5
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %merge

also_dominated:
; CHECK-LABEL: also_dominated:
; CHECK: %result2 = add i32 %K, 3
  %result2 = add i32 %tid, 3
  store i32 %result2, ptr addrspace(1) %out, align 4
  br label %merge

merge:
  br label %exit

falseBranch:
  br label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_dominated_block, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_dominated_block}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
