;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -igc-handle-spirv-decoration-metadata --igc-error-check -igc-serialize-metadata < %s 2>&1 | FileCheck %s

; The below test tries to set { L1 cached, L3 streaming } cache controls configuration that is not supported on PVC.

; Here are all cache controls combinations that are supported on PVC:
; LSC_L1UC_L3UC
; LSC_L1UC_L3C_WB
; LSC_L1C_WT_L3UC
; LSC_L1C_WT_L3C_WB
; LSC_L1S_L3UC
; LSC_L1S_L3C_WB
; LSC_L1IAR_WB_L3C_WB

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_func void @test(i32 addrspace(1)* %buffer) {
entry:
  %arrayidx0 = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1, !spirv.Decorations !7
  ; CHECK: warning: Unsupported cache controls configuration requested. Applying default configuration.
  %l0 = load i32, i32 addrspace(1)* %arrayidx0, align 4

  ret void
}

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!IGCMetadata = !{!3}

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
!7 = !{!8}
!8 = !{i32 6442, i32 1, i32 2}
