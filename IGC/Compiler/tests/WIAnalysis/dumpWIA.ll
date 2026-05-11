;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -S -igc-propagate-cmp-uniformity -print-wia-check --disable-output --regkey=PrintToConsole=1 < %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; ============================================================================
; Test: Both operands uniform, but result uses non-uniform LocalID_X (random).
; K1 and K2 are uniform_global; LocalID_X (SystemValue 17) is random.
; Instructions depending on LocalID_X are also random.
; ============================================================================
define spir_kernel void @test_both_uniform(i32 %K1, i32 %K2, ptr addrspace(1) %out) {
entry:
  %LocalID_X = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %cmp = icmp eq i32 %K1, %K2
  br i1 %cmp, label %equal, label %not_equal

equal:
  %result = add i32 %K1, %LocalID_X
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

; CHECK: Args:
; CHECK-NEXT: uniform_global i32 %K1
; CHECK-NEXT: uniform_global i32 %K2
; CHECK-NEXT: uniform_global ptr addrspace(1) %out
; CHECK-EMPTY:
; CHECK-NEXT: define spir_kernel void @test_both_uniform(i32 %K1, i32 %K2, ptr addrspace(1) %out) {
; CHECK-NEXT: entry:
; CHECK-NEXT: BB:0 entry       ; preds =[ uniform_global ]
; CHECK-NEXT:   random   %LocalID_X = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
; CHECK-NEXT:   uniform_global  %cmp = icmp eq i32 %K1, %K2
; CHECK-NEXT:   uniform_global  br i1 %cmp, label %equal, label %not_equal [ BB:1 BB:2 ]
; CHECK-EMPTY:
; CHECK-NEXT: equal:                                            ; preds = %entry
; CHECK-NEXT: BB:1 equal       ; preds = BB:0  entry[ uniform_global ]
; CHECK-NEXT:   random   %result = add i32 %K1, %LocalID_X
; CHECK-NEXT:   random   store i32 %result, ptr addrspace(1) %out, align 4
; CHECK-NEXT:   uniform_global  br label %exit [ BB:3 ]
; CHECK-EMPTY:
; CHECK-NEXT: not_equal:                                        ; preds = %entry
; CHECK-NEXT: BB:2 not_equal       ; preds = BB:0  entry[ uniform_global ]
; CHECK-NEXT:   uniform_global  br label %exit [ BB:3 ]
; CHECK-EMPTY:
; CHECK-NEXT: exit:                                             ; preds = %not_equal, %equal
; CHECK-NEXT: BB:3 exit       ; preds = BB:2  not_equal, BB:1  equal[ uniform_global ]
; CHECK-NEXT:   uniform_global  ret void [ ]
; CHECK-NEXT: }
