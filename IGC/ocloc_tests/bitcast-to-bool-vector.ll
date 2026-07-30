;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; On LLVM 22, InstCombine folds a trunc-to-i1 of an extracted element into
; a bitcast to a bool vector plus an extractelement. IGC represents i1 as
; a HW flag and cannot alias a wide vector as a bool vector.
; Legalization now rewrites the extract into an element extract + shift + mask + trunc.

; REQUIRES: regkeys, llvm-22-plus

; RUN: llvm-as < %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

; CHECK-NOT: boolean cannot have alias

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-G1"
target triple = "spir64-unknown-unknown"

; CHECK: .kernel "kernel"
; CHECK: and ({{.*}}) [[MASKED:V[0-9]+]](0,0){{.*}} 0x1:d
; CHECK: cmp.ne ({{.*}}) {{P[0-9]+}} [[MASKED]](0,0){{.*}} 0x0:d

define spir_kernel void @kernel() {
entry:
  %0 = call spir_func i32 @_Z25__spirv_BuiltInSubgroupIdv()
  %1 = trunc i32 %0 to i1
  %idx.ext = select i1 %1, i64 1, i64 0
  %add.ptr = getelementptr i8, ptr addrspace(3) null, i64 %idx.ext
  %call = load volatile i8, ptr addrspace(3) %add.ptr, align 1
  ret void
}

declare spir_func i32 @_Z25__spirv_BuiltInSubgroupIdv()
