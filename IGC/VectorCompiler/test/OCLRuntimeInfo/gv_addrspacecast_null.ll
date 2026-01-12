;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeLPG -mattr=+ocl_runtime -vc-analyze=GenXOCLRuntimeInfo \
; RUN: -vc-choose-pass-manager-override=false -o /dev/null 2>&1 | FileCheck %s --check-prefixes=CHECK

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK: Printing analysis 'GenXOCLRuntimeInfo':

; CHECK: Global:
; CHECK: Data:
; CHECK: Buffer: [
@global.casted_null = addrspace(1) global ptr addrspace(1) addrspacecast (ptr addrspace(4) null to ptr addrspace(1)), align 8
; CHECK-COUNT-7: 0,
; CHECK-SAME: 0 ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   global.casted_null
