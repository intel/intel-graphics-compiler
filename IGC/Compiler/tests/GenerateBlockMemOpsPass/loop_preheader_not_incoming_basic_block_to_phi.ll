;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -generate-block-mem-ops -platformpvc

; The test passes if `igc_opt` does not crash on this input.
; No FileCheck is needed.

define spir_kernel void @test(float addrspace(1)* %in, i32 %selector, i64 %idx1_init, i64 %idx2_init, i64 %threshold) {
entry:
  br label %loop

loop:                                             ; preds = %back, %entry
  ; PHI nodes for loop indices
  %idx1 = phi i64 [ %idx1_init, %entry ], [ %idx1_next, %back ]
  %idx2 = phi i64 [ %idx2_init, %entry ], [ %idx2_next, %back ]

  %cond = icmp eq i32 %selector, 0
  br i1 %cond, label %compute_ptr, label %alt_compute_ptr

compute_ptr:                                      ; preds = %loop
  %ptr1 = getelementptr float, float addrspace(1)* %in, i64 %idx1
  br label %select_ptr

alt_compute_ptr:                                  ; preds = %loop
  %ptr2 = getelementptr float, float addrspace(1)* %in, i64 %idx2
  br label %select_ptr

select_ptr:                                       ; preds = %alt_compute_ptr, %compute_ptr
  %final_ptr = phi float addrspace(1)* [ %ptr2, %alt_compute_ptr ], [ %ptr1, %compute_ptr ]
  %float_idx1 = sitofp i64 %idx1 to float
  %val = fadd contract float %float_idx1, 1.0
  br label %store

store:                                            ; preds = %select_ptr
  store float %val, float addrspace(1)* %final_ptr, align 4
  br label %back

back:                                             ; preds = %store
  ; Increment indices
  %idx1_next = add i64 %idx1, 1
  %idx2_next = add i64 %idx2, 1

  ; Check if idx1_next reached threshold
  %exit_cond = icmp uge i64 %idx1_next, %threshold
  br i1 %exit_cond, label %exit, label %loop

exit:
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (float addrspace(1)*, i32, i64, i64, i64)* @test, !1}
!1 = !{!2, !11}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (float addrspace(1)*, i32, i64, i64, i64)* @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"workGroupWalkOrder", !8, !9, !10}
!8 = !{!"dim0", i32 0}
!9 = !{!"dim1", i32 1}
!10 = !{!"dim2", i32 2}
!11 = !{!"thread_group_size", i32 256, i32 1, i32 1}
