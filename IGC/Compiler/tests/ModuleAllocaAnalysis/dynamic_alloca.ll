;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt --opaque-pointers --igc-module-alloca-info -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ModuleAllocaAnalysis
; ------------------------------------------------
;
; CHECK: define void @test_dynamic_alloca(i64 %size, i64 %index) #0
; CHECK: attributes #0 = { "hasVLA" }
define void @test_dynamic_alloca(i64 %size, i64 %index) {
entry:
  %alloc_var = alloca i32, i64 %size, align 4
  %0 = getelementptr inbounds i32, ptr %alloc_var, i64 %index
  %1 = load i32, ptr %0, align 4
  call void @use.i32(i32 %1)
  ret void
}

declare void @use.i32(i32)

!igc.functions = !{}
