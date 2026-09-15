;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-builtin-import -disable-verify -S < %s | FileCheck %s

; This test checks whether unused BiF functions/globals are removed correctly.
; It ensures that constant expressions (referencing GVs) used in BiF functions (that are removed)
; are handled correctly and GV is actually removed.

; CHECK-NOT: myStruct
; CHECK-NOT: myData
; CHECK-NOT: myFunc

%myStruct = type { [5 x i64] }

@myData = dso_local unnamed_addr addrspace(2) constant %myStruct { [5 x i64] [i64 5,  i64 50, i64 500, i64 5000, i64 50000 ]  }, align 8, !igc_bif !0

define dso_local spir_func void @myFunc(i64 %index) local_unnamed_addr !igc_bif !1 {
  %gep = getelementptr inbounds i64, ptr addrspace(2) getelementptr inbounds (%myStruct, ptr addrspace(2) @myData, i64 2), i64 %index
  ret void
}

!0 = !{!"IGC built-in global"}
!1 = !{!"IGC built-in function"}
