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


; CHECK-LABEL: @test_loop_with_no_simd_lane_id(
; CHECK-NOT: br i1 %loop_exit_compare, label %after_loop, label %header, !llvm.loop !0

; CHECK-NOT: !0 = distinct !{!0, !1}
; CHECK-NOT: !1 = !{!"llvm.licm.disable"}


@.red.__local = internal addrspace(3) global i32 zeroinitializer

define spir_kernel void @test_loop_with_no_simd_lane_id(i32 addrspace(1)* noalias %out, i32 %what_to_add, i32 %condition) {
entry:
  %simdSize = call i32 @llvm.genx.GenISA.simdSize()
  br label %header

header:                                           ; preds = %loop_end, %entry
  %counter = phi i32 [ %incremented_counter, %loop_end ], [ 0, %entry ]
  %loop_compare = icmp eq i32 %counter, %condition
  br i1 %loop_compare, label %loop_body, label %loop_end

loop_body:                                        ; preds = %header
  %loaded_value = load i32, i32 addrspace(3)* @.red.__local
  %added_value = add i32 %loaded_value, %what_to_add
  store i32 %added_value, i32 addrspace(3)* @.red.__local
  br label %loop_end

loop_end:                                         ; preds = %loop_body, %header
  %incremented_counter = add nuw i32 %counter, 1
  %loop_exit_compare = icmp eq i32 %incremented_counter, %simdSize
  br i1 %loop_exit_compare, label %after_loop, label %header

after_loop:                                       ; preds = %loop_end
  %loaded_local_final = load i32, i32 addrspace(3)* @.red.__local
  store i32 %loaded_local_final, i32 addrspace(1)* %out
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare i32 @llvm.genx.GenISA.simdSize()
!igc.functions = !{}
