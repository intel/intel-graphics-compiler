;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-22-plus
; RUN: igc_opt --opaque-pointers -S -igc-wi-analysis -print-wia-check --disable-output --regkey=PrintToConsole=1 < %s 2>&1 | FileCheck %s

; ptrtoaddr takes the dependency of its pointer operand, like ptrtoint.
; CHECK: uniform_global %uniform.int = ptrtoint ptr addrspace(1) %base to i64
; CHECK-NEXT: uniform_global %uniform.addr = ptrtoaddr ptr addrspace(1) %base to i64
; CHECK: random %random.int = ptrtoint ptr addrspace(1) %ptr to i64
; CHECK-NEXT: random %random.addr = ptrtoaddr ptr addrspace(1) %ptr to i64
define spir_kernel void @test_ptrtoaddr(ptr addrspace(1) %base, i16 %localIdX) {
entry:
  %uniform.int = ptrtoint ptr addrspace(1) %base to i64
  %uniform.addr = ptrtoaddr ptr addrspace(1) %base to i64
  %ptr = getelementptr i32, ptr addrspace(1) %base, i16 %localIdX
  %random.int = ptrtoint ptr addrspace(1) %ptr to i64
  %random.addr = ptrtoaddr ptr addrspace(1) %ptr to i64
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_ptrtoaddr, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_ptrtoaddr}
!5 = !{!"FuncMDValue[0]", !7}
!6 = !{!"function_type", i32 0}
!7 = !{!"implicitArgInfoList", !8}
!8 = !{!"implicitArgInfoListVec[0]", !9}
!9 = !{!"argId", i32 8}
