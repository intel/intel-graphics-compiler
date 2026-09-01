;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; On LLVM 22, LoopVersioning computes the backedge-taken count in twice the
; induction variable width, which can result in LLVM producing i128.
; Make sure the expected legalization survives through the emitter.

; REQUIRES: regkeys, bmg-supported, llvm-22-plus

; RUN: llvm-as < %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options "-igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-G1"

; CHECK: .kernel "kernel"

; The add's carry out is a value added into the chunk above, not a predicate.
; CHECK: cmp.lt ({{.*}}) V{{[0-9]+}}

; A compare degenerated to the low chunk would be a lone cmp; the chunk-equality
; test and the predicate combine are the signature of the lexicographic form.
; CHECK: cmp.gt ({{.*}}) [[HI:P[0-9]+]]
; CHECK: cmp.eq ({{.*}}) [[EQ:P[0-9]+]]
; CHECK: and ({{.*}}) [[EQ]] [[EQ]] [[HI]]
; CHECK: or ({{.*}}) {{P[0-9]+}} {{P[0-9]+}} [[EQ]]

define spir_kernel void @kernel() {
entry:
  %n = load volatile i64, ptr addrspace(3) null, align 8
  %wide = zext i64 %n to i128
  %dec = add nsw i128 %wide, -1
  %ovf = icmp ugt i128 %dec, 18446744073709551615
  %z = zext i1 %ovf to i64
  store volatile i64 %z, ptr addrspace(3) null, align 8
  %lo = trunc i128 %dec to i64
  store volatile i64 %lo, ptr addrspace(3) null, align 8
  ret void
}
