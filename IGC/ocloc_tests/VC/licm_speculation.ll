;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, ptl-supported

; LICM hoists the invariant mul out of the conditional block, so after the
; full unroll one mul remains in the entry block instead of one per iteration.

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc -device ptl -llvm_input -options "-vc-codegen -igc_opts 'EnableOpaquePointersBackend=1, ShaderDumpEnable=1, PrintToConsole=1'" -file %t.bc 2>&1 | FileCheck %s

; CHECK-LABEL: _optimized.ll
; CHECK:       define dllexport spir_kernel void @test(
; CHECK-NOT:   br
; CHECK:       = mul i32
; CHECK-NOT:   = mul i32
; CHECK:       ret void

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

define dllexport spir_kernel void @test(ptr addrspace(1) "VCArgumentIOKind"="0" %out, ptr addrspace(1) "VCArgumentIOKind"="0" %flags, i32 "VCArgumentIOKind"="0" %x, i32 "VCArgumentIOKind"="0" %stride) #0 {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %latch ]
  %fp = getelementptr inbounds i32, ptr addrspace(1) %flags, i32 %i
  %f = load i32, ptr addrspace(1) %fp, align 4
  %c = icmp ne i32 %f, 0
  br i1 %c, label %then, label %latch

then:
  %m = mul i32 %x, %stride
  %v = add i32 %m, %i
  %op = getelementptr inbounds i32, ptr addrspace(1) %out, i32 %i
  store i32 %v, ptr addrspace(1) %op, align 4
  br label %latch

latch:
  %i.next = add nuw nsw i32 %i, 1
  %done = icmp eq i32 %i.next, 8
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

attributes #0 = { noinline nounwind "VCFunction" }

!spirv.Source = !{!0}
!0 = !{i32 0, i32 100000}
