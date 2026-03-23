;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Regression test: payload creation must be hoisted to the innermost loop's
; preheader only, NOT to an outer loop's preheader.
;
; When ImageOffset is computed in the outer loop body (invariant w.r.t. the
; inner loop but variant w.r.t. the outer loop), hoisting the payload to the
; outer loop's preheader would create a use-before-def (dominance violation).
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --igc-restore-genisa-intrinsics --opaque-pointers -decompose-2d-block-funcs-with-hoisting -dce -platformbmg -S < %s 2>&1 | FileCheck %s

declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

; Test 1: Nested loop where ImageOffset depends on the outer loop induction
; variable. The payload must be created in the inner loop's preheader
; (outer.body), not the outer loop's preheader (entry).
;
; CHECK-LABEL: define spir_kernel void @test_nested_loop_base_from_outer
; CHECK-LABEL: entry:
; CHECK-NOT: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK-LABEL: outer.body:
; CHECK: %image_offset = add i64 %base, %outer_offset
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %image_offset,
; CHECK-LABEL: inner.body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_nested_loop_base_from_outer(i32 %M, i32 %N, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %outer.body

outer.body:
  %outer_iv = phi i32 [ 0, %entry ], [ %outer_next, %inner.exit ]
  %outer_offset = sext i32 %outer_iv to i64
  ; ImageOffset computed from outer loop IV - invariant w.r.t. inner loop,
  ; but variant w.r.t. outer loop
  %image_offset = add i64 %base, %outer_offset
  br label %inner.body

inner.body:
  %inner_iv = phi i32 [ 0, %outer.body ], [ %inner_next, %inner.body ]
  %inner_x = phi i32 [ %offsetX, %outer.body ], [ %next_x, %inner.body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %image_offset, i32 %width, i32 %height, i32 %pitch, i32 %inner_x, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %next_x = add i32 %inner_x, 32
  %inner_next = add i32 %inner_iv, 1
  %inner_done = icmp eq i32 %inner_next, %N
  br i1 %inner_done, label %inner.exit, label %inner.body

inner.exit:
  %outer_next = add i32 %outer_iv, 64
  %outer_done = icmp eq i32 %outer_next, %M
  br i1 %outer_done, label %exit, label %outer.body

exit:
  ret void
}

; Test 2: Two block reads in inner loop, both using ImageOffset from the outer
; loop. Both payloads must be placed in the inner preheader.
;
; CHECK-LABEL: define spir_kernel void @test_nested_loop_two_reads_outer_base
; CHECK-LABEL: entry:
; CHECK-NOT: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK-LABEL: outer.body:
; CHECK-DAG: %img_off1 = add i64 %base1, %outer_off
; CHECK-DAG: %img_off2 = add i64 %base2, %outer_off
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %img_off1,
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %img_off2,
; CHECK-LABEL: inner.body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_nested_loop_two_reads_outer_base(i32 %M, i32 %N, i64 %base1, i64 %base2, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %outer.body

outer.body:
  %outer_iv = phi i32 [ 0, %entry ], [ %outer_next, %inner.exit ]
  %outer_off = sext i32 %outer_iv to i64
  %img_off1 = add i64 %base1, %outer_off
  %img_off2 = add i64 %base2, %outer_off
  br label %inner.body

inner.body:
  %inner_iv = phi i32 [ 0, %outer.body ], [ %inner_next, %inner.body ]
  %ix = phi i32 [ %offsetX, %outer.body ], [ %next_ix, %inner.body ]
  %r1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %img_off1, i32 %width, i32 %height, i32 %pitch, i32 %ix, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %r2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %img_off2, i32 %width, i32 %height, i32 %pitch, i32 %ix, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %next_ix = add i32 %ix, 32
  %inner_next = add i32 %inner_iv, 1
  %inner_done = icmp eq i32 %inner_next, %N
  br i1 %inner_done, label %inner.exit, label %inner.body

inner.exit:
  %outer_next = add i32 %outer_iv, 128
  %outer_done = icmp eq i32 %outer_next, %M
  br i1 %outer_done, label %exit, label %outer.body

exit:
  ret void
}

; Test 3: All payload args are truly loop-invariant for both loops (constants
; and function args). The payload should still be placed in the innermost
; loop's preheader (outer.body), not hoisted further out.
;
; CHECK-LABEL: define spir_kernel void @test_nested_all_invariant_stays_inner
; CHECK-LABEL: outer.body:
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off,
; CHECK-LABEL: inner.body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_nested_all_invariant_stays_inner(i32 %M, i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %outer.body

outer.body:
  %outer_iv = phi i32 [ 0, %entry ], [ %outer_next, %inner.exit ]
  br label %inner.body

inner.body:
  %inner_iv = phi i32 [ 0, %outer.body ], [ %inner_next, %inner.body ]
  %ix = phi i32 [ 0, %outer.body ], [ %next_ix, %inner.body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %ix, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %next_ix = add i32 %ix, 32
  %inner_next = add i32 %inner_iv, 1
  %inner_done = icmp eq i32 %inner_next, %N
  br i1 %inner_done, label %inner.exit, label %inner.body

inner.exit:
  %outer_next = add i32 %outer_iv, 1
  %outer_done = icmp eq i32 %outer_next, %M
  br i1 %outer_done, label %exit, label %outer.body

exit:
  ret void
}
