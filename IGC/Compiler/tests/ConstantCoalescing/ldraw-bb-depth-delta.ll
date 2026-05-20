;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; ConstantCoalescingMaxBBDepthDelta threshold sweep.
;
; Two ldrawvector.indexed reads of the same bindless buffer at adjacent
; offsets (0 and 16) sit in basic blocks at different dominator-tree depths.
; The first read is in @entry (dom level 0); the second is in @bb3 (dom level
; 3), so the dom-tree depth delta between them is 3.
;
;   entry  (level 0)  -- load cb[0]   --> chunk created here
;     |
;   bb1    (level 1)
;     |
;   bb2    (level 2)
;     |
;   bb3    (level 3)  -- load cb[16]  --> merge candidate vs chunk
;
; Depending on ConstantCoalescingMaxBBDepthDelta, ConstantCoalescing either
; merges the two reads into a single <8 x float> load (low/disabled threshold
; below delta-3, or threshold >= 3) or leaves them as two separate <4 x float>
; reads (threshold in (0, 3)).

; REQUIRES: llvm-14-plus, regkeys

; delta = 0 disables the check -> merge as before
; RUN: igc_opt --opaque-pointers %s -S -o - -ocl -platformdg2 --regkey ConstantCoalescingMaxBBDepthDelta=0 -igc-constant-coalescing | FileCheck %s --check-prefixes=CHECK,MERGE

; delta = 1 (< 3) -> reject the merge, leave both loads separate
; RUN: igc_opt --opaque-pointers %s -S -o - -ocl -platformdg2 --regkey ConstantCoalescingMaxBBDepthDelta=1 -igc-constant-coalescing | FileCheck %s --check-prefixes=CHECK,SPLIT

; delta = 2 (< 3) -> reject the merge as well
; RUN: igc_opt --opaque-pointers %s -S -o - -ocl -platformdg2 --regkey ConstantCoalescingMaxBBDepthDelta=2 -igc-constant-coalescing | FileCheck %s --check-prefixes=CHECK,SPLIT

; delta = 3 (== 3) -> allow the merge (the check rejects only delta > maxDepthDelta)
; RUN: igc_opt --opaque-pointers %s -S -o - -ocl -platformdg2 --regkey ConstantCoalescingMaxBBDepthDelta=3 -igc-constant-coalescing | FileCheck %s --check-prefixes=CHECK,MERGE

; delta = 10 (> 3) -> allow the merge
; RUN: igc_opt --opaque-pointers %s -S -o - -ocl -platformdg2 --regkey ConstantCoalescingMaxBBDepthDelta=10 -igc-constant-coalescing | FileCheck %s --check-prefixes=CHECK,MERGE

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
  ; depth delta from entry: 3
  %2 = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368(ptr addrspace(2490368) %0, i32 16, i32 16, i1 false)
  %e0 = extractelement <4 x float> %1, i32 0
  %e1 = extractelement <4 x float> %2, i32 0
  %sum = fadd float %e0, %e1
  store float %sum, ptr addrspace(1) %output
  br label %bb_exit

bb_exit:
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_kernel

; MERGE path: ConstantCoalescing merged the two v4f32 reads into one wider
; chunk. Coalescing typically rounds up to the next supported transposed-load
; size; the original v4f32 calls disappear.
; MERGE-NOT: call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 0, i32 16, i1 false)
; MERGE-NOT: call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 16, i32 16, i1 false)
; MERGE:     call {{<[0-9]+ x float>|<[0-9]+ x i32>}} @llvm.genx.GenISA.ldrawvector.indexed.{{(v[0-9]+f32|v[0-9]+i32)}}.p2490368({{.*}} i32 0,

; SPLIT path: both original v4f32 reads survive in their original blocks and
; no wider ldraw is created.
; SPLIT:     call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 0, i32 16, i1 false)
; SPLIT:     call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 16, i32 16, i1 false)
; SPLIT-NOT: call <8 x float> @llvm.genx.GenISA.ldrawvector.indexed
; SPLIT-NOT: call <16 x float> @llvm.genx.GenISA.ldrawvector.indexed

; Function Attrs: argmemonly nounwind readonly
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368(ptr addrspace(2490368), i32, i32, i1) #0

attributes #0 = { argmemonly nounwind readonly }

!igc.functions = !{!0}
!0 = !{ptr @test_kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
