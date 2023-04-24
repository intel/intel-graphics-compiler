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
@gv.a = addrspace(2) constant i16 65, align 2
; CHECK-SAME: 65, 0
; CHECK-NOT: ,
; CHECK-SAME: ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   2
; CHECK-NEXT:   s_name:   gv.a
; CHECK-NOT: s_
; CHECK-NOT: Relocations:

; CHECK: Global:
; CHECK: Data:
; CHECK: Buffer: [
; CHECK-NOT: ,
@rel.bc.a.p2i8 = addrspace(1) global i8 addrspace(2)* bitcast (i16 addrspace(2)* @gv.a to i8 addrspace(2)*), align 8
; CHECK-COUNT-7: 0,
; CHECK-SAME: 0 ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.bc.a.p2i8
; CHECK-NOT: s_
; CHECK: Relocations:
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 0
; CHECK-NEXT:   r_symbol: gv.a
; CHECK-NOT: r_
