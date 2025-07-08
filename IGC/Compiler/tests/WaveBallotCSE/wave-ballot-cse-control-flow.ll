;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -wave-ballot-cse -S %s | FileCheck %s

; Test that CSE does NOT work across basic blocks (due to convergent nature)
define i32 @test_no_cross_block_cse(i1 %cond) {
; CHECK-LABEL: @test_no_cross_block_cse
; CHECK-NEXT:    %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK-NEXT:    br i1 %cond, label %then, label %else
; CHECK:       then:
; CHECK-NEXT:    %mask2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK-NEXT:    %result_then = add i32 %mask1, %mask2
; CHECK-NEXT:    br label %merge
; CHECK:       else:
; CHECK-NEXT:    %mask3 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK-NEXT:    %result_else = add i32 %mask1, %mask3
; CHECK-NEXT:    br label %merge
; CHECK:       merge:
; CHECK-NEXT:    %result = phi i32 [ %result_then, %then ], [ %result_else, %else ]
; CHECK-NEXT:    ret i32 %result
;
  %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  br i1 %cond, label %then, label %else

then:
  %mask2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %result_then = add i32 %mask1, %mask2
  br label %merge

else:
  %mask3 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %result_else = add i32 %mask1, %mask3
  br label %merge

merge:
  %result = phi i32 [ %result_then, %then ], [ %result_else, %else ]
  ret i32 %result
}

; Test within-block CSE works in multiple blocks
define i32 @test_within_block_cse_multiple_blocks(i1 %cond) {
; CHECK-LABEL: @test_within_block_cse_multiple_blocks
; CHECK-NEXT:    %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK-NEXT:    %entry_work = add i32 %mask1, %mask1
; CHECK-NEXT:    br i1 %cond, label %then, label %else
; CHECK:       then:
; CHECK-NEXT:    %mask_then = call i32 @llvm.genx.GenISA.WaveBallot(i1 false, i32 1)
; CHECK-NEXT:    %result_then = add i32 %mask_then, %mask_then
; CHECK-NEXT:    br label %merge
; CHECK:       else:
; CHECK-NEXT:    %mask_else = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 2)
; CHECK-NEXT:    %result_else = add i32 %mask_else, %mask_else
; CHECK-NEXT:    br label %merge
; CHECK:       merge:
; CHECK-NEXT:    %result = phi i32 [ %result_then, %then ], [ %result_else, %else ]
; CHECK-NEXT:    %final = add i32 %result, %entry_work
; CHECK-NEXT:    ret i32 %final
;
  %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %mask1_dup = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %entry_work = add i32 %mask1, %mask1_dup
  br i1 %cond, label %then, label %else

then:
  %mask_then = call i32 @llvm.genx.GenISA.WaveBallot(i1 false, i32 1)
  %mask_then_dup = call i32 @llvm.genx.GenISA.WaveBallot(i1 false, i32 1)
  %result_then = add i32 %mask_then, %mask_then_dup
  br label %merge

else:
  %mask_else = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 2)
  %mask_else_dup = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 2)
  %result_else = add i32 %mask_else, %mask_else_dup
  br label %merge

merge:
  %result = phi i32 [ %result_then, %then ], [ %result_else, %else ]
  %final = add i32 %result, %entry_work
  ret i32 %final
}

; Test complex control flow
define i32 @test_complex_control_flow(i1 %cond1, i1 %cond2) {
; CHECK-LABEL: @test_complex_control_flow
; CHECK-NEXT:    %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK-NEXT:    %entry_result = add i32 %mask1, %mask1
; CHECK-NEXT:    br i1 %cond1, label %branch1, label %branch2
; CHECK:       branch1:
; CHECK-NEXT:    br i1 %cond2, label %nested1, label %nested2
; CHECK:       nested1:
; CHECK-NEXT:    %mask_n1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 false, i32 1)
; CHECK-NEXT:    %n1_result = add i32 %mask_n1, %mask_n1
; CHECK-NEXT:    br label %merge
; CHECK:       nested2:
; CHECK-NEXT:    %mask_n2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 1)
; CHECK-NEXT:    %n2_result = add i32 %mask_n2, %mask_n2
; CHECK-NEXT:    br label %merge
; CHECK:       branch2:
; CHECK-NEXT:    %mask_b2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 2)
; CHECK-NEXT:    %b2_result = add i32 %mask_b2, %mask_b2
; CHECK-NEXT:    br label %merge
; CHECK:       merge:
; CHECK-NEXT:    %result = phi i32 [ %n1_result, %nested1 ], [ %n2_result, %nested2 ], [ %b2_result, %branch2 ]
; CHECK-NEXT:    %final = add i32 %result, %entry_result
; CHECK-NEXT:    ret i32 %final
;
  %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %mask1_dup = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %entry_result = add i32 %mask1, %mask1_dup
  br i1 %cond1, label %branch1, label %branch2

branch1:
  br i1 %cond2, label %nested1, label %nested2

nested1:
  %mask_n1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 false, i32 1)
  %mask_n1_dup = call i32 @llvm.genx.GenISA.WaveBallot(i1 false, i32 1)
  %n1_result = add i32 %mask_n1, %mask_n1_dup
  br label %merge

nested2:
  %mask_n2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 1)
  %mask_n2_dup = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 1)
  %n2_result = add i32 %mask_n2, %mask_n2_dup
  br label %merge

branch2:
  %mask_b2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 2)
  %mask_b2_dup = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 2)
  %b2_result = add i32 %mask_b2, %mask_b2_dup
  br label %merge

merge:
  %result = phi i32 [ %n1_result, %nested1 ], [ %n2_result, %nested2 ], [ %b2_result, %branch2 ]
  %final = add i32 %result, %entry_result
  ret i32 %final
}

declare i32 @llvm.genx.GenISA.WaveBallot(i1, i32)