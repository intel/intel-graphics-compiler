;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Verify the chunk's "deepest absorbed BB" semantics of the BB-depth-delta
; guard. The check measures the dom-tree depth delta from the chunk's
; deepestLvl (deepest BB the chunk has ever absorbed) to the candidate load's
; BB, NOT from the chunk's original chunkIO BB.
;
; CFG (loads of the same bindless buffer at adjacent offsets):
;
;   entry  (depth 0)   load cb[ 0]   <-- creates chunk
;     |
;   bb1    (depth 1)
;     |
;   bb2    (depth 2)   load cb[16]   <-- candidate vs chunk
;     |
;   bb3    (depth 3)
;     |
;   bb4    (depth 4)   load cb[32]   <-- candidate vs chunk (or vs intermediate)
;
; Threshold MaxBBDepthDelta = 2:
;   each merge measured against the chunk's deepest absorbed BB.
;   cb[16] delta = 2-0 = 2 -> ALLOW. deepestLvl bumped to 2.
;   cb[32] delta = 4-2 = 2 -> ALLOW. deepestLvl bumped to 4.
;   Result: chunk = cb[0..32], no narrow remainders.

; REQUIRES: llvm-14-plus, regkeys, shader-types

; Expect ONE wide ldraw merge spanning all three reads.
; Run as a pixel shader: the guard is shader-type-agnostic, not RT-gated.
; RUN: igc_opt --opaque-pointers %s -S -o - -inputps -platformdg2 --regkey ConstantCoalescingMaxBBDepthDelta=2,ConstantCoalescingDepthCheckMinBytes=0 -igc-constant-coalescing | FileCheck %s

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
  ; dom level 2 - delta vs chunk = 2 -> ALLOWED at threshold 2
  %2 = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368(ptr addrspace(2490368) %0, i32 16, i32 16, i1 false)
  br i1 %cmp, label %bb3, label %bb_exit

bb3:
  br i1 %cmp, label %bb4, label %bb_exit

bb4:
  ; dom level 4. v1 delta vs chunkIO (entry, level 0) = 4 -> REJECTED.
  ; v2 delta vs deepestLvl (= 2 after bb2 absorbed) = 2 -> ALLOWED.
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

; All three original v4f32 reads must have been absorbed into a single wider
; chunk -- none of them should survive as a v4f32 ldraw call.
; CHECK-NOT: call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 0, i32 16, i1 false)
; CHECK-NOT: call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 16, i32 16, i1 false)
; CHECK-NOT: call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368({{.*}} i32 32, i32 16, i1 false)
; CHECK:     call {{<[0-9]+ x float>|<[0-9]+ x i32>}} @llvm.genx.GenISA.ldrawvector.indexed.{{(v[0-9]+f32|v[0-9]+i32)}}.p2490368({{.*}} i32 0,

; Function Attrs: argmemonly nounwind readonly
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2490368(ptr addrspace(2490368), i32, i32, i1) #0

attributes #0 = { argmemonly nounwind readonly }

!igc.functions = !{!0}
!0 = !{ptr @test_kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
