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
@gv.a = addrspace(2) constant i8 65, align 1
; CHECK-SAME: 65,
; CHECK-NOT: ,
@gv.b = addrspace(2) constant i8 66, align 1
; CHECK-SAME: 66
; CHECK-NOT: ,
; CHECK-SAME: ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   1
; CHECK-NEXT:   s_name:   gv.a
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 1
; CHECK-NEXT:   s_size:   1
; CHECK-NEXT:   s_name:   gv.b
; CHECK-NOT: s_
; CHECK-NOT: Relocations:

; CHECK: Global:
; CHECK: Data:
; CHECK: Buffer: [
; CHECK-NOT: ,
@rel.i64 = addrspace(1) global i64 ptrtoint (i8 addrspace(2)* @gv.a to i64), align 8
@rel.a2i64 = addrspace(1) global [2 x i64] [i64 ptrtoint (i8 addrspace(2)* @gv.a to i64), i64 ptrtoint (i8 addrspace(2)* @gv.b to i64)], align 8
@rel.i32 = addrspace(1) global i32 ptrtoint (i8 addrspace(2)* @gv.a to i32), align 4
@rel.a2i32 = addrspace(1) global [2 x i32] [i32 ptrtoint (i8 addrspace(2)* @gv.a to i32), i32 ptrtoint (i8 addrspace(2)* @gv.b to i32)], align 4
; CHECK-COUNT-35: 0,
; CHECK-SAME: 0 ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.i64
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 8
; CHECK-NEXT:   s_size:   16
; CHECK-NEXT:   s_name:   rel.a2i64
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 24
; CHECK-NEXT:   s_size:   4
; CHECK-NEXT:   s_name:   rel.i32
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 28
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.a2i32
; CHECK-NOT: s_
; CHECK: Relocations:
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 0
; CHECK-NEXT:   r_symbol: gv.a
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 8
; CHECK-NEXT:   r_symbol: gv.a
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 16
; CHECK-NEXT:   r_symbol: gv.b
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 24
; CHECK-NEXT:   r_symbol: gv.a
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 28
; CHECK-NEXT:   r_symbol: gv.a
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 32
; CHECK-NEXT:   r_symbol: gv.b
; CHECK-NOT: r_
