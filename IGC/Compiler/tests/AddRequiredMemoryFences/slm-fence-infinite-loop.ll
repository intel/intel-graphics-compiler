;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; FIXME: make this test work without shader type
; REQUIRES: llvm-14-plus, shader-types
; RUN: igc_opt --opaque-pointers %s -S --inputcs --igc-add-required-memory-fences | FileCheck %s

; Test placement of memory fence in infinite loop with SLM store

; CHECK-LABEL: define spir_kernel void @infinite_loop_reduced
; CHECK-LABEL: ._crit_edge:
; CHECK:      store ptr null, ptr addrspace(3) %L4
; CHECK:      store i16 13, ptr %A1, align 8
; CHECK:      call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK:      br label %._crit_edge
define spir_kernel void @infinite_loop_reduced(ptr %A1) {
  %A11 = alloca i32, i32 0, align 8
  br label %._crit_edge

._crit_edge:
  %L4 = phi ptr addrspace(3) [ %L4.pre, %._crit_edge ], [ null, %0 ]
  store ptr null, ptr addrspace(3) %L4, align 8
  store i16 13, ptr %A1, align 8
  %L4.pre = load ptr addrspace(3), ptr %A1, align 8
  br label %._crit_edge
}



; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32) #1

attributes #1 = { convergent nounwind }
