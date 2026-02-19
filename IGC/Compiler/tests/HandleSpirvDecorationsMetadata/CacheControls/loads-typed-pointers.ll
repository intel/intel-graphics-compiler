;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -platformpvc -S -o - -igc-handle-spirv-decoration-metadata -igc-serialize-metadata | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i32 addrspace(1)* %buffer) {
entry:
  %arrayidx0 = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1, !spirv.Decorations !7
  ; CHECK: %l0 = load i32, i32 addrspace(1)* %arrayidx0, align 4, !lsc.cache.ctrl [[L0_CC:!.*]]
  %l0 = load i32, i32 addrspace(1)* %arrayidx0, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1, !spirv.Decorations !10
  ; CHECK: %l1 = load i32, i32 addrspace(1)* %arrayidx1, align 4, !lsc.cache.ctrl [[L1_CC:!.*]]
  %l1 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1, !spirv.Decorations !13
  ; CHECK: %l2 = load i32, i32 addrspace(1)* %arrayidx2, align 4, !lsc.cache.ctrl [[L2_CC:!.*]]
  %l2 = load i32, i32 addrspace(1)* %arrayidx2, align 4
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1, !spirv.Decorations !16
  ; CHECK: load i32, i32 addrspace(1)* %arrayidx3, align 4, !lsc.cache.ctrl
  %l3 = load i32, i32 addrspace(1)* %arrayidx3, align 4
  %arrayidx4 = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1, !spirv.Decorations !19
  ; CHECK: %l4 = load i32, i32 addrspace(1)* %arrayidx4, align 4, !lsc.cache.ctrl [[L4_CC:!.*]]
  %l4 = load i32, i32 addrspace(1)* %arrayidx4, align 4
  %arrayidx5 = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1, !spirv.Decorations !22
  ; CHECK: %l5 = load i32, i32 addrspace(1)* %arrayidx5, align 4, !lsc.cache.ctrl [[L5_CC:!.*]]
  %l5 = load i32, i32 addrspace(1)* %arrayidx5, align 4
  %arrayidx6 = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1, !spirv.Decorations !25
  ; CHECK: %l6 = load i32, i32 addrspace(1)* %arrayidx6, align 4, !lsc.cache.ctrl [[L6_CC:!.*]]
  %l6 = load i32, i32 addrspace(1)* %arrayidx6, align 4
  ret void
}

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!IGCMetadata = !{!3}

; CHECK: [[L0_CC]] = !{i32 1}
; CHECK: [[L1_CC]] = !{i32 2}
; CHECK: [[L2_CC]] = !{i32 3}
; CHECK: [[L4_CC]] = !{i32 5}
; CHECK: [[L5_CC]] = !{i32 6}
; CHECK: [[L6_CC]] = !{i32 7}

; Above literals represent the following enums:
; 1 - LSC_L1UC_L3UC
; 2 - LSC_L1UC_L3C_WB
; 3 - LSC_L1C_WT_L3UC
; 4 - LSC_L1C_WT_L3C_WB
; 5 - LSC_L1S_L3UC
; 6 - LSC_L1S_L3C_WB
; 7 - LSC_L1IAR_WB_L3C_WB

!0 = !{i32 2, i32 2}
!1 = !{i32 3, i32 102000}
!2 = !{i16 6, i16 14}
!3 = !{!"ModuleMD", !4}
!4 = !{!"compOpt", !5, !6}
; Below values represent the default cache controls that in real compilation are passed
; from OCL/L0 Runtime as internal options and may vary depending on the target platform.
; These are the default values for PVC:
!5 = !{!"LoadCacheDefault", i32 4}   ; LSC_L1C_WT_L3C_WB - loads:  L1 cached,   L3 cached
!6 = !{!"StoreCacheDefault", i32 2}  ; LSC_L1UC_L3C_WB   - stores: L1 uncached, L3 write-back
!7 = !{!8, !9}
!8 = !{i32 6442, i32 0, i32 0}    ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!9 = !{i32 6442, i32 1, i32 0}    ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}
!10 = !{!11, !12}
!11 = !{i32 6442, i32 0, i32 0}    ; {CacheControlLoadINTEL, CacheLevel=0, Uncached}
!12 = !{i32 6442, i32 1, i32 1}    ; {CacheControlLoadINTEL, CacheLevel=1, Cached}
!13 = !{!14, !15}
!14 = !{i32 6442, i32 0, i32 1}    ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!15 = !{i32 6442, i32 1, i32 0}    ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}
!16 = !{!17, !18}
!17 = !{i32 6442, i32 0, i32 1}    ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!18 = !{i32 6442, i32 1, i32 1}    ; {CacheControlLoadINTEL, CacheLevel=1, Cached}
!19 = !{!20, !21}
!20 = !{i32 6442, i32 0, i32 2}    ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!21 = !{i32 6442, i32 1, i32 0}    ; {CacheControlLoadINTEL, CacheLevel=1, Uncached}
!22 = !{!23, !24}
!23 = !{i32 6442, i32 0, i32 2}    ; {CacheControlLoadINTEL, CacheLevel=0, Streaming}
!24 = !{i32 6442, i32 1, i32 1}    ; {CacheControlLoadINTEL, CacheLevel=1, Cached}
!25 = !{!26, !27}
!26 = !{i32 6442, i32 0, i32 3}    ; {CacheControlLoadINTEL, CacheLevel=0, InvalidateAfterRead}
!27 = !{i32 6442, i32 1, i32 1}    ; {CacheControlLoadINTEL, CacheLevel=1, Cached}
