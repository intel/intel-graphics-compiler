;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -regkey InjectPrintfFlag=1 --inject-printf -S < %s | FileCheck %s --check-prefix=LOADS
; RUN: igc_opt --typed-pointers -regkey InjectPrintfFlag=2 --inject-printf -S < %s | FileCheck %s --check-prefix=STORES
; RUN: igc_opt --typed-pointers -regkey InjectPrintfFlag=3 --inject-printf -S < %s | FileCheck %s --check-prefix=LOADS_AND_STORES
;
; ------------------------------------------------
; InjectPrintf
; ------------------------------------------------

@global_var = global i32 0

define spir_kernel void @test_function() {
entry:
    %ptr = alloca i32, align 4
    %val = load i32, i32* %ptr, align 4
    store i32 %val, i32* @global_var, align 4
    ret void
}

; LOADS-LABEL: @test_function(
; LOADS: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* %ptr, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; LOADS: load i32, i32* %ptr, align 4
; LOADS-NOT: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* @global_var, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; LOADS: store i32 %val, i32* @global_var, align 4
; LOADS: ret void

; STORES-LABEL: @test_function(
; STORES-NOT: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* %ptr, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; STORES: load i32, i32* %ptr, align 4
; STORES: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* @global_var, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; STORES: store i32 %val, i32* @global_var, align 4
; STORES: ret void

; LOADS_AND_STORES-LABEL: @test_function(
; LOADS_AND_STORES: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* %ptr, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; LOADS_AND_STORES: load i32, i32* %ptr, align 4
; LOADS_AND_STORES: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %2, i32* @global_var, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @2, i32 0, i32 0))
; LOADS_AND_STORES: store i32 %val, i32* @global_var, align 4
; LOADS_AND_STORES: ret void
