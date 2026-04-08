;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-MergeMemFromBranchOpt -S < %s | FileCheck %s

; MergeMemFromBranchOpt should correctly update existing PHI nodes in the
; successor block when creating an intermediate merge block. The redirected
; predecessor entries must be collapsed into a new PHI in the merge block,
; with a single entry from mergeBB replacing them in the original PHI.

; CHECK-LABEL: @test_merge_typedwrite_extra_pred_phi(
; CHECK:       store_a:
; CHECK-NEXT:    br label %[[MERGE:[^ ]+]]
; CHECK:       store_b:
; CHECK-NEXT:    br label %[[MERGE]]
; CHECK:       [[MERGE]]:
; CHECK:         phi ptr addrspace(10903552)
; CHECK:         phi float
; CHECK:         phi float
; CHECK:         phi float
; CHECK:         phi float
; CHECK:         %[[RESULT_MM:[^ ]+]] = phi i32
; CHECK:         call void @llvm.genx.GenISA.typedwrite.p10903552
; CHECK-NEXT:    br label %common.ret
; CHECK:       common.ret:
; CHECK:         %result = phi i32 [ 0, %early_exit ], [ %[[RESULT_MM]], %[[MERGE]] ]
; CHECK-NEXT:    ret i32 %result

define i32 @test_merge_typedwrite_extra_pred_phi(i32 %res_handle, i32 %idx0, i32 %idx1, i1 %cond1, i1 %cond2, float %val0, float %val1) {
entry:
  %u0_a = inttoptr i32 %res_handle to ptr addrspace(10903552)
  %u0_b = inttoptr i32 %res_handle to ptr addrspace(10903552)
  br i1 %cond1, label %compute, label %early_exit

compute:
  br i1 %cond2, label %store_a, label %store_b

store_a:
  call void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552) %u0_a, i32 %idx0, i32 %idx1, i32 0, i32 0, float 1.0, float 2.0, float 3.0, float 4.0)
  br label %common.ret

store_b:
  call void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552) %u0_b, i32 %idx0, i32 %idx1, i32 0, i32 0, float %val0, float %val1, float 0.0, float 1.0)
  br label %common.ret

early_exit:
  br label %common.ret

common.ret:
  %result = phi i32 [ 1, %store_a ], [ 2, %store_b ], [ 0, %early_exit ]
  ret i32 %result
}

declare void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552), i32, i32, i32, i32, float, float, float, float)
