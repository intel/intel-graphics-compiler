;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; Test that emit fence skips L1 evict only when the default L1 store policy is known
; to leave no dirty data in L1, and keeps it when the default leaves L1 to state or is unknown.

; Write-through does not leave dirty data in L1.
; RUN: igc_opt --opaque-pointers %s -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=NONE
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 22/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=NONE
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 25/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=NONE
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 4/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=NONE

; Write-streaming with L2 uncached, and legacy write-streaming with L3 uncached.
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 26/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=EVICT
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 27/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=EVICT
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 5/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=EVICT

; Defaults that leave L1 to state, and an unknown value, keep the evict as if no default was given.
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 16/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=EVICT
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 0/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=EVICT
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 17/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=EVICT
; RUN: sed -e 's/"StoreCacheDefault", i32 24/"StoreCacheDefault", i32 -1/' %s | igc_opt --opaque-pointers -S --platformCri -simd-mode 32 -igc-emit-visa -regkey EnableEfficient64b,DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=EVICT

; NONE-LABEL: .kernel "test"
; NONE:       _main_0:
; NONE-NEXT:     lsc_fence.ugm.none.gpu

; EVICT-LABEL: .kernel "test"
; EVICT:       _main_0:
; EVICT-NEXT:     lsc_fence.ugm.evict.gpu
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
!17 = !{!"LoadCacheDefault", i32 25}   ; load  - LSC_LDCC_L1C_L2C_L3C    (25) - L1 cached,        L2 cached,     L3 cached
!18 = !{!"StoreCacheDefault", i32 24}  ; store - LSC_STCC_L1WT_L2WB_L3UC (24) - L1 write-through, L2 write-back, L3 uncached
