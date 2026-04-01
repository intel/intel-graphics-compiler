;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys

; Test that emit fence evicts L1 only when default L1 cache policy is write-back.

; RUN: igc_opt --opaque-pointers %s -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s

; CHECK-LABEL: .kernel "test"
; CHECK:       _main_0:
; CHECK-NEXT:     lsc_fence.ugm.evict.gpu
define spir_kernel void @test(i8 addrspace(1)* align 1 %a) {
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 true, i32 3)
  ret void
}

declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1, i1, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (i8 addrspace(1)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{!"ModuleMD", !4, !16}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (i8 addrspace(1)*)* @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"resAllocMD", !8, !9, !10, !11}
!8 = !{!"uavsNumType", i32 0}
!9 = !{!"srvsNumType", i32 0}
!10 = !{!"samplersNumType", i32 0}
!11 = !{!"argAllocMDList", !12}
!12 = !{!"argAllocMDListVec[0]", !13, !14, !15}
!13 = !{!"type", i32 0}
!14 = !{!"extensionType", i32 -1}
!15 = !{!"indexType", i32 -1}

!16 = !{!"compOpt", !17, !18}
!17 = !{!"LoadCacheDefault", i32 4}   ; load  - LSC_L1C_WT_L3C_WB   (4) - L1 cached,     L3 cached
!18 = !{!"StoreCacheDefault", i32 7}  ; store - LSC_L1IAR_WB_L3C_WB (7) - L1 write-back, L3 write-back
