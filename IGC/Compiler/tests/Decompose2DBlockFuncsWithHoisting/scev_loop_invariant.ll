;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Tests for SCEV-based loop invariance detection in Decompose2DBlockFuncsWithHoisting.
; These tests verify that values computed inside the loop body but with zero SCEV step
; (effectively loop-invariant) are properly hoisted to payload creation.
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting -dce -platformbmg -S < %s 2>&1 | FileCheck %s

declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

; Test 1: Base address (i64) computed via add inside loop body
; The add instruction is inside the loop but operands are loop-invariant,
; so SCEV analysis should detect zero step and hoist to payload creation.
;
; CHECK-LABEL: define spir_kernel void @test_base_add_inside_loop
; CHECK: entry:
; CHECK: add i64 %{{.*}}, %{{.*}}
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_base_add_inside_loop(i32 %N, i64 %base, i64 %offset, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Add is inside loop but operands are loop-invariant
  %off = add i64 %base, %offset
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 2: Width computed via add inside loop body (loop-invariant operands)
; The add instruction is inside the loop but both operands are loop-invariant.
;
; CHECK-LABEL: define spir_kernel void @test_width_add_inside_loop
; CHECK: entry:
; CHECK: add i32 %{{.*}}, %{{.*}}
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_width_add_inside_loop(i32 %N, i64 %off, i32 %width_base, i32 %width_offset, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Add is inside loop but operands are loop-invariant
  %width = add i32 %width_base, %width_offset
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 3: Multiple payload args computed inside loop (all loop-invariant)
; Width, height, and pitch are all computed inside loop but are loop-invariant.
;
; CHECK-LABEL: define spir_kernel void @test_multiple_args_inside_loop
; CHECK: entry:
; CHECK-DAG: add i32 %width_base, 100
; CHECK-DAG: add i32 %height_base, 200
; CHECK-DAG: shl i32 %pitch_base, 2
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_multiple_args_inside_loop(i32 %N, i64 %off, i32 %width_base, i32 %height_base, i32 %pitch_base, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; All these are inside loop but loop-invariant
  %width = add i32 %width_base, 100
  %height = add i32 %height_base, 200
  %pitch = shl i32 %pitch_base, 2
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 4: Base address computed from ptrtoint inside loop
; ptrtoint is inside the loop but the pointer is loop-invariant.
;
; CHECK-LABEL: define spir_kernel void @test_ptrtoint_base_inside_loop
; CHECK: entry:
; CHECK: ptrtoint ptr %base_ptr to i64
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_ptrtoint_base_inside_loop(i32 %N, ptr %base_ptr, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; ptrtoint is inside loop but ptr is loop-invariant
  %off = ptrtoint ptr %base_ptr to i64
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 5: Zext/sext inside loop for width/height (loop-invariant operands)
;
; CHECK-LABEL: define spir_kernel void @test_zext_sext_inside_loop
; CHECK: entry:
; CHECK-DAG: zext i16 %width16 to i32
; CHECK-DAG: sext i16 %height16 to i32
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_zext_sext_inside_loop(i32 %N, i64 %off, i16 %width16, i16 %height16, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; zext/sext inside loop but operands are loop-invariant
  %width = zext i16 %width16 to i32
  %height = sext i16 %height16 to i32
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 6: Base address with complex chain of operations inside loop
; Multiple operations chained together, all loop-invariant.
;
; CHECK-LABEL: define spir_kernel void @test_complex_base_chain
; CHECK: entry:
; CHECK: shl i64 %stride, 4
; CHECK: add i64 %base
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_complex_base_chain(i32 %N, i64 %base, i64 %stride, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Chain of operations inside loop, all loop-invariant
  %off1 = mul i64 %stride, 16
  %off = add i64 %base, %off1
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 7: Write operation with loop-invariant base computed inside loop
;
; CHECK-LABEL: define spir_kernel void @test_write_base_inside_loop
; CHECK: entry:
; CHECK: ptrtoint ptr %base_ptr to i64
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32

define spir_kernel void @test_write_base_inside_loop(i32 %N, ptr %base_ptr, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; ptrtoint is inside loop but operand is loop-invariant
  %off = ptrtoint ptr %base_ptr to i64
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 8: Multiple LSC calls sharing payload, with SCEV-hoisted base
;
; CHECK-LABEL: define spir_kernel void @test_multiple_lsc_scev_base
; CHECK: entry:
; CHECK: ptrtoint ptr %base_ptr to i64
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_multiple_lsc_scev_base(i32 %N, ptr %base_ptr, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  %offsetX2 = add i32 %offsetX, 16
  %offsetY2 = add i32 %offsetY, 32
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Same ptrtoint used by multiple LSC calls
  %off = ptrtoint ptr %base_ptr to i64
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 9: Bitcast <2 x i32> to i64 inside loop with loop-invariant vector
; This tests the instruction hoisting path where SCEV cannot analyze the bitcast
; but the instruction can be cloned to the preheader because its operand is loop-invariant.
;
; CHECK-LABEL: define spir_kernel void @test_bitcast_v2i32_to_i64
; CHECK: entry:
; CHECK: bitcast <2 x i32> %base_vec to i64
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_bitcast_v2i32_to_i64(i32 %N, <2 x i32> %base_vec, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; bitcast is inside loop but operand %base_vec is loop-invariant
  ; SCEV cannot analyze this but we can hoist via instruction cloning
  %off = bitcast <2 x i32> %base_vec to i64
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 10: Bitcast i64 to <2 x i32> and back to i64 - chained bitcasts
; Chained bitcasts where intermediate result is inside loop are NOT optimized.
; The pass only hoists single bitcast whose operand is already outside the loop.
; This test verifies the pass correctly skips this case (no decomposition).
;
; CHECK-LABEL: define spir_kernel void @test_chained_bitcasts
; CHECK: body:
; CHECK: bitcast i64 %base to <2 x i32>
; CHECK: bitcast <2 x i32> %vec to i64
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead

define spir_kernel void @test_chained_bitcasts(i32 %N, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Chained bitcasts inside loop - second bitcast depends on first which is inside loop
  ; This case is NOT optimized since it would require cloning chains
  %vec = bitcast i64 %base to <2 x i32>
  %off = bitcast <2 x i32> %vec to i64
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 11: Multiple LSC calls with same bitcast base (hoisting shared)
; Tests that the bitcast is hoisted once and shared among multiple LSC calls.
;
; CHECK-LABEL: define spir_kernel void @test_bitcast_multiple_lsc
; CHECK: entry:
; CHECK: bitcast <2 x i32> %base_vec to i64
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_bitcast_multiple_lsc(i32 %N, <2 x i32> %base_vec, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  %offsetX2 = add i32 %offsetX, 16
  %offsetY2 = add i32 %offsetY, 32
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Same bitcast used by multiple LSC calls
  %off = bitcast <2 x i32> %base_vec to i64
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}
