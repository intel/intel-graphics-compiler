;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test for LSC Load Block 2D with cache controls {L1: CA, L2: DF, L3: CA}.
; Verifies that the Unified path (used by CRI/Xe3p) correctly handles the L2 Default
; without asserting in LSCComputeCachingEncodingLoad - "invalid caching option".

; REQUIRES: llvm-16-plus, regkeys

; RUN: igc_opt --opaque-pointers --CheckInstrTypes --igc-update-instrtypes-on-run -inputocl --neo \
; RUN:         -platformCri -igc-emit-visa -regkey DumpVISAASMToConsole,EnableEfficient64b -simd-mode 16 %s \
; RUN: | FileCheck %s

; CHECK-LABEL: .function
; CHECK: lsc_load_block2d.ugm.ca.ca
; CHECK: lsc_load_block2d.ugm.ca.ca
; CHECK: ret

define spir_kernel void @test_no_crash(i8 addrspace(1)* align 1 %a) {
entry:
  call void asm sideeffect "lsc_load_block2d.ugm.ca.ca (M1, 1) %null:d$1.$2x$3nn flat[$0+(0,0)]", "rw.u,P,P,P"(i8 addrspace(1)* %a, i32 16, i32 32, i32 8)
  call void asm sideeffect "lsc_load_block2d.ugm.ca.ca (M1, 1) %null:d$1.$2x$3nn flat[$0,15,31,7,0,0]", "rw.u,P,P,P"(i8 addrspace(1)* %a, i32 16, i32 32, i32 8)
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!13}

!0 = !{void (i8 addrspace(1)*)* @test_no_crash, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!13 = !{!"ModuleMD", !14}
!14 = !{!"FuncMD", !15, !16}
!15 = !{!"FuncMDMap[0]", void (i8 addrspace(1)*)* @test_no_crash}
!16 = !{!"FuncMDValue[0]", !100}
!100 = !{!"resAllocMD", !183, !184, !185, !186}
!183 = !{!"uavsNumType", i32 0}
!184 = !{!"srvsNumType", i32 0}
!185 = !{!"samplersNumType", i32 0}
!186 = !{!"argAllocMDList", !187}
!187 = !{!"argAllocMDListVec[0]", !188, !189, !190}
!188 = !{!"type", i32 0}
!189 = !{!"extensionType", i32 -1}
!190 = !{!"indexType", i32 -1}
