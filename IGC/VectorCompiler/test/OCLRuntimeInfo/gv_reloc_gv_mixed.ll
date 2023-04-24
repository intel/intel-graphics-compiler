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
@gv.a = addrspace(2) constant [2 x i8] [i8 65, i8 97], align 1
; CHECK-SAME: 65, 97,
; CHECK-NOT: ,
@gv.b = addrspace(2) constant i8 66, align 1
; CHECK-SAME: 66,
; CHECK-NOT: ,
; COM: padding
; CHECK-SAME: 0,
@gv.c = addrspace(1) constant i16 67, align 2
; CHECK-SAME: 67, 0
; CHECK-NOT: ,
; CHECK-SAME: ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   2
; CHECK-NEXT:   s_name:   gv.a
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 2
; CHECK-NEXT:   s_size:   1
; CHECK-NEXT:   s_name:   gv.b
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 4
; CHECK-NEXT:   s_size:   2
; CHECK-NEXT:   s_name:   gv.c
; CHECK-NOT: s_
; CHECK-NOT: r_

; CHECK: Global:
; CHECK: Data:
; CHECK: Buffer: [
; CHECK-NOT: ,
@rel.a.gep.i64 = addrspace(1) global i64 ptrtoint (i8 addrspace(2)* getelementptr inbounds ([2 x i8], [2 x i8] addrspace(2)* @gv.a, i64 0, i64 1) to i64), align 8
; CHECK-SAME: 1, 0, 0, 0, 0, 0, 0, 0,
@rel.b.gep.i64 = addrspace(1) global i64 ptrtoint (i8 addrspace(2)* getelementptr (i8, i8 addrspace(2)* @gv.b, i64 4294967298) to i64), align 8
; CHECK-SAME: 2, 0, 0, 0, 1, 0, 0, 0,
; CHECK-NOT: ,
@rel.a.gep.i32 = addrspace(1) global i32 ptrtoint (i8 addrspace(2)* getelementptr inbounds ([2 x i8], [2 x i8] addrspace(2)* @gv.a, i64 0, i64 1) to i32), align 4
; COM: yaml serializer inserts new line here
; CHECK-NEXT: 1, 0, 0, 0,
@rel.b.gep.i32 = addrspace(1) global i32 ptrtoint (i8 addrspace(2)* getelementptr (i8, i8 addrspace(2)* @gv.b, i64 4294967298) to i32), align 4
; COM: The high bit was truncated out.
; CHECK-SAME: 2, 0, 0, 0,
; CHECK-NOT: ,
@rel.c.bc.asc = addrspace(1) global i8 addrspace(4)* addrspacecast (i8 addrspace(1)* bitcast (i16 addrspace(1)* @gv.c to i8 addrspace(1)*) to i8 addrspace(4)*), align 8
; CHECK-COUNT-8: 0,
; CHECK-NOT: ,
@rel.c.bc.asc.gep.i32 = addrspace(1) global i32 ptrtoint (i8 addrspace(4)* getelementptr (i8, i8 addrspace(4)* addrspacecast (i8 addrspace(1)* bitcast (i16 addrspace(1)* @gv.c to i8 addrspace(1)*) to i8 addrspace(4)*), i64 2) to i32), align 4
; COM: yaml serializer inserts new line here
; CHECK-NEXT: 2, 0, 0, 0
; CHECK-NOT: ,
; CHECK-SAME: ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.a.gep.i64
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 8
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.b.gep.i64
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 16
; CHECK-NEXT:   s_size:   4
; CHECK-NEXT:   s_name:   rel.a.gep.i32
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 20
; CHECK-NEXT:   s_size:   4
; CHECK-NEXT:   s_name:   rel.b.gep.i32
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 24
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.c.bc.asc
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 32
; CHECK-NEXT:   s_size:   4
; CHECK-NEXT:   s_name:   rel.c.bc.asc.gep.i32
; CHECK-NOT: s_
; CHECK: Relocations:
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 0
; CHECK-NEXT:   r_symbol: gv.a
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 8
; CHECK-NEXT:   r_symbol: gv.b
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 16
; CHECK-NEXT:   r_symbol: gv.a
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 20
; CHECK-NEXT:   r_symbol: gv.b
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 24
; CHECK-NEXT:   r_symbol: gv.c
; CHECK-NEXT: - r_type:   R_SYM_ADDR_32
; CHECK-NEXT:   r_offset: 32
; CHECK-NEXT:   r_symbol: gv.c
; CHECK-NOT: r_
