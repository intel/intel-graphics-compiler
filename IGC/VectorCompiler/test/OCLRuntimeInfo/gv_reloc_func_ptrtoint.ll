;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 -mattr=+ocl_runtime -vc-analyze=GenXOCLRuntimeInfo \
; RUN: -vc-choose-pass-manager-override=false -o /dev/null 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK: Printing analysis 'GenXOCLRuntimeInfo':

; CHECK: ModuleInfo:
; CHECK: Constant
; CHECK: Data:
; CHECK: Buffer: [
; CHECK-NOT: ,
@rel.i64 = addrspace(2) constant i64 ptrtoint (void ()* @foo to i64), align 8
; CHECK-SAME: 0, 0, 0, 0, 0, 0, 0, 0,
; CHECK-NOT: ,
@rel.i32 = addrspace(2) constant i32 ptrtoint (void ()* @foo to i32), align 4
; CHECK-SAME: 0, 0, 0, 0
; CHECK-NOT: ,
; CHECK-SAME: ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.i64
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 8
; CHECK-NEXT:   s_size:   4
; CHECK-NEXT:   s_name:   rel.i32
; CHECK-NOT: s_
; CHECK: Relocations:
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 0
; CHECK-NEXT:   r_symbol: foo
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 8
; CHECK-NEXT:   r_symbol: foo
; CHECK-NOT: r_

; CHECK: Global:
; CHECK: Data:
; CHECK: Buffer: [
; CHECK-NOT: ,
@rel.v2i64 = addrspace(1) global <2 x i64> <i64 ptrtoint (void ()* @bar to i64), i64 ptrtoint (void ()* @foo to i64)>, align 8
@rel.v2i32 = addrspace(1) global <2 x i32> <i32 ptrtoint (void ()* @bar to i32), i32 ptrtoint (void ()* @foo to i32)>, align 4
; CHECK-COUNT-23: 0,
; CHECK-SAME: 0 ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   16
; CHECK-NEXT:   s_name:   rel.v2i64
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 16
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.v2i32
; CHECK-NOT: s_
; CHECK: Relocations:
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 0
; CHECK-NEXT:   r_symbol: bar
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 8
; CHECK-NEXT:   r_symbol: foo
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 16
; CHECK-NEXT:   r_symbol: bar
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 20
; CHECK-NEXT:   r_symbol: foo
; CHECK-NOT: r_

define spir_func void @foo() #0 {
  ret void
}

define spir_func void @bar() #0 {
  ret void
}

attributes #0 = { "CMStackCall" }
