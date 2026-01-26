;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-gep-lowering -S < %s | FileCheck %s

; Test that GEPLowering looks through addrspacecast to reuse the integer base
; for pointer arithmetic, avoiding redundant ptrtoint operations.
;
; Input pattern:  inttoptr -> addrspacecast -> getelementptr
; Optimized to:   integer arithmetic on base, then inttoptr to final pointer

define spir_kernel void @test_gep_addrspacecast(i64 %base, i64 %idx) {
; CHECK-LABEL: @test_gep_addrspacecast
;
; Verify the GEP is lowered to integer arithmetic using the original base:
; CHECK:    [[SCALE:%.*]] = shl i64 %idx, 2
; CHECK:    [[OFFSET:%.*]] = add i64 %base, [[SCALE]]
; CHECK:    [[RESULT:%.*]] = inttoptr i64 [[OFFSET]] to ptr addrspace(1)
; CHECK:    load i32, ptr addrspace(1) [[RESULT]]
;
; Verify we don't emit ptrtoint on the addrspacecast result:
; CHECK-NOT: ptrtoint {{.*}} addrspace(1)
;
  ; Start with integer, convert to pointer
  %ptr_as4 = inttoptr i64 %base to ptr addrspace(4)

  ; Cast to different address space
  %ptr_as1 = addrspacecast ptr addrspace(4) %ptr_as4 to ptr addrspace(1)

  ; Compute offset pointer (should be lowered to integer arithmetic on %base)
  %ptr_offset = getelementptr inbounds i32, ptr addrspace(1) %ptr_as1, i64 %idx

  ; Use the pointer
  %value = load i32, ptr addrspace(1) %ptr_offset
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @test_gep_addrspacecast, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
