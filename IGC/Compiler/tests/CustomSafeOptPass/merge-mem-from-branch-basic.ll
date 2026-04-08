;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-MergeMemFromBranchOpt -S < %s | FileCheck %s

; MergeMemFromBranchOpt should merge typedwrite calls when all predecessors of
; the successor block have matching typedwrite (the original behavior).
; No intermediate block is needed here.

; CHECK-LABEL: @test_merge_typedwrite_basic(
; CHECK:       store_a:
; CHECK-NEXT:    br label %common.ret
; CHECK:       store_b:
; CHECK-NEXT:    br label %common.ret
; CHECK:       common.ret:
; CHECK-NEXT:    [[P0:%.*]] = phi ptr addrspace(10903552)
; CHECK-NEXT:    [[P5:%.*]] = phi float
; CHECK-NEXT:    [[P6:%.*]] = phi float
; CHECK-NEXT:    [[P7:%.*]] = phi float
; CHECK-NEXT:    [[P8:%.*]] = phi float
; CHECK-NEXT:    call void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552) [[P0]], i32 %idx0, i32 %idx1, i32 0, i32 0, float [[P5]], float [[P6]], float [[P7]], float [[P8]])
; CHECK-NEXT:    ret void

define void @test_merge_typedwrite_basic(i32 %res_handle, i32 %idx0, i32 %idx1, i1 %cond, float %val0, float %val1) {
entry:
  %u0_a = inttoptr i32 %res_handle to ptr addrspace(10903552)
  %u0_b = inttoptr i32 %res_handle to ptr addrspace(10903552)
  br i1 %cond, label %store_a, label %store_b

store_a:
  call void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552) %u0_a, i32 %idx0, i32 %idx1, i32 0, i32 0, float 1.0, float 2.0, float 3.0, float 4.0)
  br label %common.ret

store_b:
  call void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552) %u0_b, i32 %idx0, i32 %idx1, i32 0, i32 0, float %val0, float %val1, float 0.0, float 1.0)
  br label %common.ret

common.ret:
  ret void
}

declare void @llvm.genx.GenISA.typedwrite.p10903552(ptr addrspace(10903552), i32, i32, i32, i32, float, float, float, float)
