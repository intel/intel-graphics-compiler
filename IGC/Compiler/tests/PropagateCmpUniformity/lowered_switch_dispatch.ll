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
; Test: dispatch tree produced by LowerSwitch
;
; This is the IR shape the pass receives now that it is scheduled AFTER
; createLowerSwitchPass() in OptimizeIR. LowerSwitch expands a switch into a
; binary search tree of Pivot (icmp slt) and Leaf (icmp eq) blocks; the eq
; leaves are exactly the equality branches this pass exists to exploit.
;
; Each case body has its leaf test as its single predecessor, so uses of the
; non-uniform selector inside a case body are replaced by that case's constant.
; The default block is reached without any equality guarantee and must keep the
; selector untouched.
;
; Scheduling the pass BEFORE LowerSwitch instead leaves the switch intact, and
; a SwitchInst is not a conditional branch, so the pass would see nothing to do
; on this shape at all.
; ============================================================================
; CHECK-LABEL: @test_lowered_switch_dispatch(
define spir_kernel void @test_lowered_switch_dispatch(ptr addrspace(1) %out) {
entry:
  %sel = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %Pivot = icmp slt i32 %sel, 2
  br i1 %Pivot, label %NodeBlock0, label %NodeBlock1

NodeBlock0:
  %Pivot0 = icmp eq i32 %sel, 0
  br i1 %Pivot0, label %case0, label %LeafBlock1

LeafBlock1:
  %Pivot1 = icmp eq i32 %sel, 1
  br i1 %Pivot1, label %case1, label %default

NodeBlock1:
  %Pivot2 = icmp eq i32 %sel, 2
  br i1 %Pivot2, label %case2, label %default

case0:
; CHECK-LABEL: case0:
; CHECK: %r0 = add i32 0, 100
  %r0 = add i32 %sel, 100
  store i32 %r0, ptr addrspace(1) %out, align 4
  br label %exit

case1:
; CHECK-LABEL: case1:
; CHECK: %r1 = add i32 1, 100
  %r1 = add i32 %sel, 100
  store i32 %r1, ptr addrspace(1) %out, align 4
  br label %exit

case2:
; CHECK-LABEL: case2:
; CHECK: %r2 = mul i32 2, 7
  %r2 = mul i32 %sel, 7
  store i32 %r2, ptr addrspace(1) %out, align 4
  br label %exit

default:
; CHECK-LABEL: default:
; CHECK: %rd = add i32 %sel, 1
  %rd = add i32 %sel, 1
  store i32 %rd, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_lowered_switch_dispatch, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_lowered_switch_dispatch}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
