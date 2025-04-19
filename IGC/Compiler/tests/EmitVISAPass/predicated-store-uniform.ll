;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -platformpvc -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Verifies that predicated stores are emitted for uniform stores with sub-DW alignment
define spir_kernel void @test(ptr addrspace(1) align 2 %out, i32 %predicate) {
entry:
  %p = icmp ne i32 %predicate, 0
; CHECK: (P1) lsc_store.ugm (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d32
  call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* %out, i32 1, i64 2, i1 %p)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)*, i32, i64, i1)

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
