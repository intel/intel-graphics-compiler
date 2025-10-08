;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-convert-user-semantic-decorator-on-functions -regkey DumpVISAASMToConsole -S < %s | FileCheck %s

; The test checks if @llvm.global.annotations struct is properly processed with opaque pointers.
; We don't need more than one getOpernad() method on an annotation_struct,
; otherwise the test should crash with opaque pointers

@gVar.1 = private unnamed_addr constant [20 x i8] c"num-thread-per-eu 8\00", section "llvm.metadata"
@gVar.2 = private unnamed_addr constant [20 x i8] c"num-thread-per-eu 8\00", section "llvm.metadata"
@llvm.global.annotations = appending global [2 x { ptr, ptr, ptr, i32, ptr }] [{ ptr, ptr, ptr, i32, ptr } { ptr @_testKernel, ptr @gVar.1, ptr undef, i32 undef, ptr undef }, { ptr, ptr, ptr, i32, ptr } { ptr @_testKernel, ptr @gVar.2, ptr undef, i32 undef, ptr undef }], section "llvm.metadata"

; Function Attrs: noinline nounwind optnone
define spir_kernel void @_testKernel(ptr addrspace(1) align 4 %0) {
    ret void;
}

; CHECK: @gVar.1
; CHECK: @gVar.2
; CHECK: @llvm.global.annotations
