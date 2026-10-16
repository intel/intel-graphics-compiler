;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey DisableHoistImmutableLoadsAboveBarriers=1 %s -S -igc-hoist-immutable-loads-above-barriers | FileCheck %s
; ------------------------------------------------
; HoistImmutableLoadsAboveBarriers
; ------------------------------------------------

; COM: DisableHoistImmutableLoadsAboveBarriers leaves the read after the wait.

define i32 @after_barrier(ptr addrspace(2) %root) {
; CHECK-LABEL: @after_barrier(
; CHECK:         call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    load i32, ptr addrspace(2) %root
entry:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %v = load i32, ptr addrspace(2) %root, align 4
  ret i32 %v
}

declare void @llvm.genx.GenISA.threadgroupbarrier()
