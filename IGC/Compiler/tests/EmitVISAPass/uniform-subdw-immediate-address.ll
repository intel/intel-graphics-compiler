;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Verifies that a sub-DW aligned uniform load with an immediate address
; copies the address to a register before the send.

define spir_kernel void @test(ptr addrspace(1) %out) {
entry:
; CHECK: mov (M1_NM, 1) [[ADDR:V[0-9]+]](0,0)<1> 0x0:uq
; CHECK: lsc_load.ugm (M1_NM, 1)  {{V[0-9]+}}:d32  flat{{\[}}[[ADDR]]{{\]}}:a64
  %0 = load i32, ptr addrspace(2) null, align 2
  store i32 %0, ptr addrspace(1) %out, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!3}

!0 = !{!"ModuleMD", !132}
!3 = !{ptr @test, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!132 = !{!"FuncMD", !133, !134}
!133 = !{!"FuncMDMap[0]", ptr @test}
!134 = !{!"FuncMDValue[0]", !167}
!167 = !{!"resAllocMD", !171}
!171 = !{!"argAllocMDList", !172}
!172 = !{!"argAllocMDListVec[0]", !173}
!173 = !{!"type", i32 0}
