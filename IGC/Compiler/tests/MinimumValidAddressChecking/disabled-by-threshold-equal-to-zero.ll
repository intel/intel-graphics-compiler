;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-minimum-valid-address-checking -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_kernel void @kernel(ptr addrspace(1) %input) nounwind {
  %1 = getelementptr inbounds i32, ptr addrspace(1) %input, i64 2
  %2 = load i32, ptr addrspace(1) %1
  %3 = add i32 %2, 0
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

; CHECK:          define spir_kernel void @kernel(ptr addrspace(1) %input)
; CHECK-NEXT:       %1 = getelementptr inbounds i32, ptr addrspace(1) %input, i64 2
; CHECK-NEXT:       %2 = load i32, ptr addrspace(1) %1
; CHECK-NEXT:       %3 = add i32 %2, 0
; CHECK-NEXT:       ret void
; CHECK-NEXT:     }

; CHECK-NOT:        declare void @__minimumvalidaddresschecking
