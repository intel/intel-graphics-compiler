;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXModule -GenXDepressurizerWrapper -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXModule -GenXDepressurizerWrapper -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; COM: Sanity checker: nothing should be changed, compilation should not fail.

; COM: Actually, such CFG is considered as an irreducible graph by the pass.
; COM: This is because LLVM Critical Edge Breaker doesn't deal with indirect
; COM: branches, and the pass doesn't deal with critical back edges.

; COM: Pseudo CFG (PCFG) builder removes back edges in PCFG and creates
; COM: edges to loop exits. Originally this test is to check that the pass
; COM: is not stuck inside PCFG builder forever. That happened when loop
; COM: exit for inner loop was a header of the outer one.

define dllexport void @test(i64 %val) "CMGenxMain" {
  br label %outer_loop_header
outer_loop_header:
  %cond = icmp ult i64 %val, 0
  br i1 %cond, label %outer_loop_exit, label %inner_loop_header.crit_edge
inner_loop_header.crit_edge:
  br label %inner_loop_header
inner_loop_header:
  %dst = call i8* (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, i8* blockaddress(@test, %outer_loop_header), i8* blockaddress(@test, %inner_loop_body))
  indirectbr i8* %dst, [label %outer_loop_header, label %inner_loop_body]
inner_loop_body:
  br label %inner_loop_header
outer_loop_exit:
  ret void

; CHECK-LABEL: test
; CHECK-NEXT: br label %outer_loop_header
; CHECK-LABEL: outer_loop_header:
; CHECK-NEXT: %cond = icmp ult i64 %val, 0
; CHECK-NEXT: br i1 %cond, label %outer_loop_exit, label %inner_loop_header.crit_edge
; CHECK-LABEL: inner_loop_header.crit_edge:
; CHECK-NEXT: br label %inner_loop_header
; CHECK-LABEL: inner_loop_header:
; CHECK-TYPED-PTRS-NEXT: %dst = call i8* (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, i8* blockaddress(@test, %outer_loop_header), i8* blockaddress(@test, %inner_loop_body))
; CHECK-TYPED-PTRS-NEXT: indirectbr i8* %dst, [label %outer_loop_header, label %inner_loop_body]
; CHECK-OPAQUE-PTRS-NEXT: %dst = call ptr (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, ptr blockaddress(@test, %outer_loop_header), ptr blockaddress(@test, %inner_loop_body))
; CHECK-OPAQUE-PTRS-NEXT: indirectbr ptr %dst, [label %outer_loop_header, label %inner_loop_body]
; CHECK-LABEL: inner_loop_body:
; CHECK-NEXT: br label %inner_loop_header
; CHECK-LABEL: outer_loop_exit:
; CHECK-NEXT: ret void
}

declare i8* @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8, ...)
