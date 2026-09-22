;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -platformCri -igc-emit-visa -regkey DumpVISAASMToConsole -disable-output < %s | FileCheck %s

; Check that constant-vector pooling does not assert on ConstantData use lists.
; CHECK: addr_add {{.*}} &[[VECTOR:[a-zA-Z0-9_]+]]
; CHECK: addr_add {{.*}} &[[VECTOR]]
; CHECK: lsc_store.ugm {{.*}} sum:d32t
define spir_kernel void @test(ptr addrspace(1) %out, i32 %index) {
  %first = extractelement <2 x i32> <i32 11, i32 22>, i32 %index
  %other = xor i32 %index, 1
  %second = extractelement <2 x i32> <i32 11, i32 22>, i32 %other
  %sum = add i32 %first, %second
  store i32 %sum, ptr addrspace(1) %out
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!IGCMetadata = !{!3}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !10}
!9 = !{!"argAllocMDListVec[0]", !11}
!10 = !{!"argAllocMDListVec[1]", !11}
!11 = !{!"type", i32 0}
