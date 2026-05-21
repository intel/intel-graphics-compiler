;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -S -igc-propagate-cmp-uniformity -print-wia-check --disable-output --regkey=PrintToConsole=1,EnableWIPhiStructuralEquivalence=1 < %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; ============================================================================
; Even with EnableWIPhiStructuralEquivalence=1, the predicate must NOT
; over-trigger. Each kernel below sets up a divergent join whose phi must
; still be classified RANDOM because shape-equivalence does not hold.
; ============================================================================

; Case 1: second operand differs (%K2 vs %K3) — recursion on the
; mismatched leaves hits non-equal arguments and returns false.
define spir_kernel void @test_neg_diff_operand(i32 %K1, i32 %K2, i32 %K3, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %div_cmp = icmp ult i32 %tid, 32
  br i1 %div_cmp, label %A, label %B
A:
  %a = add i32 %K1, %K2
  br label %join
B:
  %b = add i32 %K1, %K3
  br label %join
join:
  %phi = phi i32 [%a, %A], [%b, %B]
  store i32 %phi, ptr addrspace(1) %out, align 4
  ret void
}

; Case 2: same operands but different opcode (add vs sub) —
; isSameOperationAs returns false at the top level.
define spir_kernel void @test_neg_diff_opcode(i32 %K1, i32 %K2, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %div_cmp = icmp ult i32 %tid, 32
  br i1 %div_cmp, label %A, label %B
A:
  %a = add i32 %K1, %K2
  br label %join
B:
  %b = sub i32 %K1, %K2
  br label %join
join:
  %phi = phi i32 [%a, %A], [%b, %B]
  store i32 %phi, ptr addrspace(1) %out, align 4
  ret void
}

; Case 3: memory op in the operand chain — rejected by
; mayReadOrWriteMemory even though both loads come from the same pointer.
; The values may be identical at runtime, but the predicate is
; deliberately conservative about memory.
define spir_kernel void @test_neg_memory_in_chain(ptr addrspace(1) %src, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %div_cmp = icmp ult i32 %tid, 32
  br i1 %div_cmp, label %A, label %B
A:
  %la = load i32, ptr addrspace(1) %src, align 4
  %a = add i32 %la, 1
  br label %join
B:
  %lb = load i32, ptr addrspace(1) %src, align 4
  %b = add i32 %lb, 1
  br label %join
join:
  %phi = phi i32 [%a, %A], [%b, %B]
  store i32 %phi, ptr addrspace(1) %out, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1, !2, !3}

!0 = !{!"ModuleMD", !20}
!1 = !{ptr @test_neg_diff_operand, !30}
!2 = !{ptr @test_neg_diff_opcode, !30}
!3 = !{ptr @test_neg_memory_in_chain, !30}
!20 = !{!"FuncMD", !21, !22, !23, !24, !25, !26}
!21 = !{!"FuncMDMap[0]", ptr @test_neg_diff_operand}
!22 = !{!"FuncMDValue[0]", !40, !41, !42, !43}
!23 = !{!"FuncMDMap[1]", ptr @test_neg_diff_opcode}
!24 = !{!"FuncMDValue[1]", !40, !41, !42, !43}
!25 = !{!"FuncMDMap[2]", ptr @test_neg_memory_in_chain}
!26 = !{!"FuncMDValue[2]", !40, !41, !42, !43}
!30 = !{!31}
!31 = !{!"function_type", i32 0}
!40 = !{!"localOffsets"}
!41 = !{!"workGroupWalkOrder", !50, !51, !52}
!42 = !{!"funcArgs"}
!43 = !{!"functionType", !"KernelFunction"}
!50 = !{!"dim0", i32 0}
!51 = !{!"dim1", i32 1}
!52 = !{!"dim2", i32 2}

; Case 1 ---------------------------------------------------------------------
; CHECK-LABEL: define spir_kernel void @test_neg_diff_operand
; CHECK: random {{.*}}%phi = phi i32

; Case 2 ---------------------------------------------------------------------
; CHECK-LABEL: define spir_kernel void @test_neg_diff_opcode
; CHECK: random {{.*}}%phi = phi i32

; Case 3 ---------------------------------------------------------------------
; CHECK-LABEL: define spir_kernel void @test_neg_memory_in_chain
; CHECK: random {{.*}}%phi = phi i32
