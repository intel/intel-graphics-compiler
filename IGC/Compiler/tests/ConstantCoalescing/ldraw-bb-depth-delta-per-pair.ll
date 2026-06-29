;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Verify that ConstantCoalescingMaxBBDepthDelta is a per-PAIR check, not a
; global one. Three reads of the same buffer sit at three different dom-tree depths:
;
;   entry  (level 0)   --  load cb[0]    --> chunk1 created here
;     |
;   bb_far (level 4)   --  load cb[16]   --> delta vs chunk1 = 4
;     |
;   bb_deep(level 5)   --  load cb[32]   --> delta vs chunk1 = 5
;                                            delta vs (chunk2 if it exists) = 1
;
; With ConstantCoalescingMaxBBDepthDelta=1:
;   * cb[16] cannot merge into chunk1 (4 > 1) -> becomes its own chunk2 at bb_far
;   * cb[32] cannot merge into chunk1 (5 > 1) BUT can merge into chunk2 (1 <= 1)
;     -> chunk2 grows to span [16..32]
;
; So we expect:
;   * cb[0] survives unmerged as a <4 x float> in entry
;   * cb[16] and cb[32] DO coalesce into one wider load (chunk2) at bb_far

; The chunk-size gate is disabled (ConstantCoalescingDepthCheckMinBytes=0) so the
; depth-delta check fires on every candidate, regardless of merged-chunk size.
; This isolates the per-PAIR behavior we are testing here.

; REQUIRES: llvm-14-plus, regkeys, shader-types
; Run as a compute shader: the guard is shader-type-agnostic, not RT-gated.
; RUN: igc_opt --opaque-pointers %s -S -o - -inputcs -platformdg2 --regkey ConstantCoalescingMaxBBDepthDelta=1,ConstantCoalescingDepthCheckMinBytes=0 -igc-constant-coalescing | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_kernel(ptr addrspace(1) %output, i32 %bindlessOffset, i32 %branch) {
entry:
  %0 = inttoptr i32 %bindlessOffset to ptr addrspace(2490368)
  %1 = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368(ptr addrspace(2490368) %0, i32 0, i32 16, i1 false)
  %cmp = icmp ne i32 %branch, 0
  br i1 %cmp, label %bb1, label %bb_exit

bb1:
  br i1 %cmp, label %bb2, label %bb_exit

bb2:
  br i1 %cmp, label %bb3, label %bb_exit

bb3:
  br i1 %cmp, label %bb_far, label %bb_exit

bb_far:
  ; dom level 4 - delta vs chunk1 (entry, level 0) = 4 -> REJECTED at threshold 1
  %2 = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368(ptr addrspace(2490368) %0, i32 16, i32 16, i1 false)
  br i1 %cmp, label %bb_deep, label %bb_exit

bb_deep:
  ; dom level 5 - delta vs chunk1 = 5 (REJECTED) but delta vs chunk2 (bb_far) = 1 (ACCEPTED)
  %3 = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368(ptr addrspace(2490368) %0, i32 32, i32 16, i1 false)
  %e0 = extractelement <4 x float> %1, i32 0
  %e1 = extractelement <4 x float> %2, i32 0
  %e2 = extractelement <4 x float> %3, i32 0
  %s1 = fadd float %e0, %e1
  %sum = fadd float %s1, %e2
  store float %sum, ptr addrspace(1) %output
  br label %bb_exit

bb_exit:
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_kernel

; chunk1 (cb[0]) stays a <4 x float> in entry - it could not absorb either
; deeper read at threshold 1.
; CHECK: call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 0, i32 16, i1 false)

; The cb[16] and cb[32] reads coalesce into one wider chunk2 at bb_far,
; because their pairwise delta is only 1. We expect either a v8f32 load or
; some other wider form starting at offset 16; the original v4f32 calls for
; offsets 16 and 32 must not remain as separate ldraws.
; CHECK-NOT: call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 16, i32 16, i1 false)
; CHECK-NOT: call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 32, i32 16, i1 false)
; CHECK:     call {{<[0-9]+ x float>|<[0-9]+ x i32>}} @llvm.genx.GenISA.ldrawvector.indexed.{{(v[0-9]+f32|v[0-9]+i32)}}.p2490368({{.*}} i32 16,

; The pathological "merge everything" form must not appear.
; CHECK-NOT: call <16 x float> @llvm.genx.GenISA.ldrawvector.indexed

; Function Attrs: argmemonly nounwind readonly
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368(ptr addrspace(2490368), i32, i32, i1) #0

attributes #0 = { argmemonly nounwind readonly }

!igc.functions = !{!0}
!0 = !{ptr @test_kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
