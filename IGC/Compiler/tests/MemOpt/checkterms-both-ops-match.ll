;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that MemOpt does not crash when checkTerms encounters two BinaryOperators
; with both operands identical (the "EarlyExit" path).

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-memopt | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

; CHECK-LABEL: define void @checkterms_both_ops_match
; CHECK: ret void
;
define void @checkterms_both_ops_match(i64 %src, i32 %val1, i32 %val2) {
entry:
  %srcptr = inttoptr i64 %src to ptr addrspace(4)

  %sub1 = sub nsw i32 %val1, %val2
  %sext1 = sext i32 %sub1 to i64
  %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
  %addr1 = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
  %ld1 = load float, ptr addrspace(1) %addr1, align 4

  %sub2 = sub nsw i32 %val1, %val2
  %sext2 = sext i32 %sub2 to i64
  %gep2 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext2
  %addr2 = addrspacecast ptr addrspace(4) %gep2 to ptr addrspace(1)
  %ld2 = load float, ptr addrspace(1) %addr2, align 4

  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @checkterms_both_ops_match, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
