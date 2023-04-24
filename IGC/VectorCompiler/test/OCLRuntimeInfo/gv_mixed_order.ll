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
; COM: There was a bug when @global.i16 was aligned with @const.i8 requirements
; COM: just because @const.i8 goes after global.i8 in declarations, but they are
; COM: end up in different buffers, so this is irrelevant.
@global.i8 = addrspace(1) global i8 1, align 1
@const.i8 = addrspace(1) constant i8 2, align 1
@global.i16 = addrspace(1) global i16 3, align 2

; CHECK: ModuleInfo:
; CHECK: Constant
; CHECK: Data:
; CHECK: Buffer: [ 2 ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   1
; CHECK-NEXT:   s_name:   const.i8
; CHECK-NOT: s_
; CHECK-NOT: r_

; CHECK: Global:
; CHECK: Data:
; CHECK: Buffer: [ 1, 0, 3, 0 ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   1
; CHECK-NEXT:   s_name:   global.i8
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 2
; CHECK-NEXT:   s_size:   2
; CHECK-NEXT:   s_name:   global.i16
; CHECK-NOT: s_
; CHECK-NOT: r_
