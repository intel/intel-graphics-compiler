;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-MergeMemFromBranchOpt -S < %s | FileCheck %s

; MergeMemFromBranchOpt should merge typedwrite calls from branch predecessors
; even when the successor block has additional predecessors that don't contain
; a typedwrite. It does this by creating an intermediate merge block.

; CHECK-LABEL: @test_merge_typedwrite_extra_pred(
; CHECK:       store_a:
; CHECK-NEXT:    br label %[[MERGE:[^ ]+]]
; CHECK:       store_b:
; CHECK-NEXT:    br label %[[MERGE]]
; CHECK:       store_c:
; CHECK-NEXT:    br label %[[MERGE]]
; CHECK:       [[MERGE]]:
; CHECK:         phi ptr addrspace(10903552)
; CHECK:         phi float
; CHECK:         phi float
; CHECK:         phi float
; CHECK:         phi float
; CHECK:         call void @llvm.genx.GenISA.typedwrite.p10903552
; CHECK-NEXT:    br label %common.ret
; CHECK:       common.ret:
; CHECK-NEXT:    ret void

define void @test_merge_typedwrite_extra_pred(i32 %res_handle, i32 %idx0, i32 %idx1, i1 %cond1, i1 %cond2, float %val_b0, float %val_b1, float %val_c1, float %val_c2) {
entry:
  %u0_a = inttoptr i32 %res_handle to ptr addrspace(10903552)
  %u0_b = inttoptr i32 %res_handle to ptr addrspace(10903552)
  %u0_c = inttoptr i32 %res_handle to ptr addrspace(10903552)
  br i1 %cond1, label %compute, label %early_exit

compute:
  br i1 %cond2, label %store_a, label %path_bc

path_bc:
  %flag = icmp eq i32 %idx0, 0
  br i1 %flag, label %store_b, label %store_c

store_a:
  call void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552) %u0_a, i32 %idx0, i32 %idx1, i32 0, i32 0, float 1.0, float 2.0, float 3.0, float 4.0)
  br label %common.ret

store_b:
  call void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552) %u0_b, i32 %idx0, i32 %idx1, i32 0, i32 0, float %val_b0, float %val_b1, float 0.0, float 1.0)
  br label %common.ret

store_c:
  call void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552) %u0_c, i32 %idx0, i32 %idx1, i32 0, i32 0, float 0.0, float %val_c1, float %val_c2, float 1.0)
  br label %common.ret

early_exit:
  br label %common.ret

common.ret:
  ret void
}

declare void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552), i32, i32, i32, i32, float, float, float, float)
