;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -regkey InjectPrintfFlag=1 --inject-printf -S < %s | FileCheck --check-prefixes=LOADS,LOADS-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -regkey InjectPrintfFlag=1 --inject-printf -S < %s | FileCheck --check-prefixes=LOADS,LOADS-TYPED %s %}
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -regkey InjectPrintfFlag=2 --inject-printf -S < %s | FileCheck --check-prefixes=STORES,STORES-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -regkey InjectPrintfFlag=2 --inject-printf -S < %s | FileCheck --check-prefixes=STORES,STORES-TYPED %s %}
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -regkey InjectPrintfFlag=3 --inject-printf -S < %s | FileCheck --check-prefixes=LOADS_AND_STORES,LOADS_AND_STORES-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -regkey InjectPrintfFlag=3 --inject-printf -S < %s | FileCheck --check-prefixes=LOADS_AND_STORES,LOADS_AND_STORES-TYPED %s %}
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
; LOADS-TYPED: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* %ptr, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; LOADS-OPAQUE: call i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) %0, ptr %ptr, ptr @1)
; LOADS-TYPED: load i32, i32* %ptr, align 4
; LOADS-OPAQUE: load i32, ptr %ptr, align 4
; LOADS-TYPED-NOT: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* @global_var, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; LOADS-OPAQUE-NOT: call i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) %0, ptr @global_var, ptr @1)
; LOADS-TYPED: store i32 %val, i32* @global_var, align 4
; LOADS-OPAQUE: store i32 %val, ptr @global_var, align 4
; LOADS: ret void

; STORES-LABEL: @test_function(
; STORES-TYPED-NOT: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* %ptr, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; STORES-OPAQUE-NOT: call i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) %0, ptr %ptr, ptr @1)
; STORES-TYPED: load i32, i32* %ptr, align 4
; STORES-OPAQUE: load i32, ptr %ptr, align 4
; STORES-TYPED: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* @global_var, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; STORES-OPAQUE: call i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) %0, ptr @global_var, ptr @1)
; STORES-TYPED: store i32 %val, i32* @global_var, align 4
; STORES-OPAQUE: store i32 %val, ptr @global_var, align 4
; STORES: ret void

; LOADS_AND_STORES-LABEL: @test_function(
; LOADS_AND_STORES-TYPED: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i32* %ptr, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @1, i32 0, i32 0))
; LOADS_AND_STORES-OPAQUE: call i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) %0, ptr %ptr, ptr @1)
; LOADS_AND_STORES-TYPED: load i32, i32* %ptr, align 4
; LOADS_AND_STORES-OPAQUE: load i32, ptr %ptr, align 4
; LOADS_AND_STORES-TYPED: call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %2, i32* @global_var, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @2, i32 0, i32 0))
; LOADS_AND_STORES-OPAQUE: call i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) %2, ptr @global_var, ptr @2)
; LOADS_AND_STORES-TYPED: store i32 %val, i32* @global_var, align 4
; LOADS_AND_STORES-OPAQUE: store i32 %val, ptr @global_var, align 4
; LOADS_AND_STORES: ret void
