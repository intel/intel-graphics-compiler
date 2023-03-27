;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-disable-licm-for-specific-loops -S < %s | FileCheck %s
; ------------------------------------------------
; DisableLICMForSpecificLoops
; ------------------------------------------------


; CHECK-LABEL: @test_loop(

; CHECK:        br i1 %loop_exit_compare, label %after_loop1, label %header1, !llvm.loop !0

; CHECK:        br i1 %loop_exit_compare2, label %after_loop2, label %header2, !llvm.loop !2

; CHECK:        !0 = distinct !{!0, !1}
; CHECK:        !1 = !{!"llvm.licm.disable"}
; CHECK:        !2 = distinct !{!2, !1}


@.red.__local = internal addrspace(3) global i32 zeroinitializer

define spir_kernel void @test_loop(i32 addrspace(1)* noalias %out, i32 %what_to_add) {
entry:
  %simdSize = call i32 @llvm.genx.GenISA.simdSize()
  %simdLaneId_org = call i16 @llvm.genx.GenISA.simdLaneId()
  %simdLaneId = zext i16 %simdLaneId_org to i32
  br label %header1

header1:                                           ; preds = %loop_end1, %entry
  %counter = phi i32 [ %incremented_counter, %loop_end1 ], [ 0, %entry ]
  %loop_compare = icmp eq i32 %counter, %simdLaneId
  br i1 %loop_compare, label %loop_body1, label %loop_end1

loop_body1:                                        ; preds = %header1
  %loaded_value = load i32, i32 addrspace(3)* @.red.__local
  %added_value = add i32 %loaded_value, %what_to_add
  store i32 %added_value, i32 addrspace(3)* @.red.__local
  br label %loop_end1

loop_end1:                                         ; preds = %loop_body1, %header1
  %incremented_counter = add nuw i32 %counter, 1
  %loop_exit_compare = icmp eq i32 %incremented_counter, %simdSize
  br i1 %loop_exit_compare, label %after_loop1, label %header1

after_loop1:                                       ; preds = %loop_end
  %loaded_local_final = load i32, i32 addrspace(3)* @.red.__local
  store i32 %loaded_local_final, i32 addrspace(1)* %out
  br label %header2

header2:                                           ; preds = %loop_end2, %after_loop1
  %counter2 = phi i32 [ %incremented_counter2, %loop_end2 ], [ 0, %after_loop1 ]
  %loop_compare2 = icmp eq i32 %counter2, %simdLaneId
  br i1 %loop_compare2, label %loop_body2, label %loop_end2

loop_body2:                                        ; preds = %header2
  %loaded_value2 = load i32, i32 addrspace(3)* @.red.__local
  %added_value2 = add i32 %loaded_value2, %what_to_add
  store i32 %added_value2, i32 addrspace(3)* @.red.__local
  br label %loop_end2

loop_end2:                                         ; preds = %loop_body2, %header2
  %incremented_counter2 = add nuw i32 %counter2, 1
  %loop_exit_compare2 = icmp eq i32 %incremented_counter2, %simdSize
  br i1 %loop_exit_compare2, label %after_loop2, label %header2

after_loop2:                                       ; preds = %loop_end
  %loaded_local_final2 = load i32, i32 addrspace(3)* @.red.__local
  store i32 %loaded_local_final2, i32 addrspace(1)* %out
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare i32 @llvm.genx.GenISA.simdSize()
!igc.functions = !{}
