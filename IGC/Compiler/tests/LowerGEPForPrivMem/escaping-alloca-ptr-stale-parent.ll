;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------
;
; An alloca whose own pointer is the stored value escapes into memory and must not be
; promoted; that check used to miss it because a nested checkUsers left parentLevelInst
; pointing at a deeper GEP.

%struct.jm = type { <8 x i32> }

; The alloca escapes into element 2 of itself. %esc precedes %e0, so the escaping store is
; visited only after visitGetElementPtrInst(%e0) has clobbered parentLevelInst.
;
; CHECK-LABEL: @escaping_ptr_after_gep_recursion(
; CHECK-NOT: alloca <
; CHECK-NOT: ptrtoint ptr %arr
; CHECK: %arr = alloca [4 x %struct.jm]
; CHECK: store ptr %arr, ptr %esc
define spir_kernel void @escaping_ptr_after_gep_recursion(ptr addrspace(1) %out, <8 x i32> %val) {
entry:
  %arr = alloca [4 x %struct.jm], align 32
  %esc = getelementptr inbounds %struct.jm, ptr %arr, i64 2
  store ptr %arr, ptr %esc, align 32
  %e0 = getelementptr inbounds [4 x %struct.jm], ptr %arr, i64 0, i64 0
  store <8 x i32> %val, ptr %e0, align 32
  %e1 = getelementptr inbounds [4 x %struct.jm], ptr %arr, i64 0, i64 1
  %ld = load <8 x i32>, ptr %e1, align 32
  store <8 x i32> %ld, ptr addrspace(1) %out, align 32
  ret void
}

; A derived pointer written through itself: the address operand does match the traversed
; value, so only the second half of the check catches this one.
;
; CHECK-LABEL: @escaping_ptr_stored_through_itself(
; CHECK-NOT: alloca <
; CHECK: %arr = alloca [4 x %struct.jm]
; CHECK: store ptr %esc, ptr %esc
define spir_kernel void @escaping_ptr_stored_through_itself(ptr addrspace(1) %out, <8 x i32> %val) {
entry:
  %arr = alloca [4 x %struct.jm], align 32
  %esc = getelementptr inbounds %struct.jm, ptr %arr, i64 2
  store ptr %esc, ptr %esc, align 32
  %e0 = getelementptr inbounds [4 x %struct.jm], ptr %arr, i64 0, i64 0
  store <8 x i32> %val, ptr %e0, align 32
  ret void
}

; Same shape, no escape: must still promote, guarding against over-rejection.
;
; CHECK-LABEL: @no_escape_still_promoted(
; CHECK: alloca <32 x i32>
; CHECK-NOT: alloca [4 x %struct.jm]
define spir_kernel void @no_escape_still_promoted(ptr addrspace(1) %out, <8 x i32> %val) {
entry:
  %arr = alloca [4 x %struct.jm], align 32
  %e2 = getelementptr inbounds %struct.jm, ptr %arr, i64 2
  store <8 x i32> %val, ptr %e2, align 32
  %e0 = getelementptr inbounds [4 x %struct.jm], ptr %arr, i64 0, i64 0
  store <8 x i32> %val, ptr %e0, align 32
  %e1 = getelementptr inbounds [4 x %struct.jm], ptr %arr, i64 0, i64 1
  %ld = load <8 x i32>, ptr %e1, align 32
  store <8 x i32> %ld, ptr addrspace(1) %out, align 32
  ret void
}

!igc.functions = !{!0, !3, !4}

!0 = !{ptr @escaping_ptr_after_gep_recursion, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @escaping_ptr_stored_through_itself, !1}
!4 = !{ptr @no_escape_still_promoted, !1}
