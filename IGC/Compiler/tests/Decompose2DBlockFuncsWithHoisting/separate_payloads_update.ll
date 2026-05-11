;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Tests that each payload in a separate-payload group receives its own
; SetAddrPayloadField for every loop-variant field it uses.
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting -dce -platformbmg -S < %s 2>&1 | FileCheck %s

declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

; Two reads with different loop-invariant x-offsets but the SAME loop-variant
; y-offset. Each read gets its own AddrPayload and its own SetAddrPayloadField
; for the y-field (field=6).
;
; CHECK-LABEL: define spir_kernel void @test_separate_payloads_same_y
; CHECK-LABEL: entry:
; CHECK: [[P1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[P2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; BLOCKX is loop-invariant and baked into CreateAddrPayload — no SetField for field 5.
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P1]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P1]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[P1]], i32 0, i32 0,
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P2]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P2]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[P2]], i32 0, i32 0,

define spir_kernel void @test_separate_payloads_same_y(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetX2, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 true, i1 false, i32 0)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 true, i1 false, i32 0)
  %phi_y_next = add i32 %phi_y, 32
  %Nm1_dec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Nm1_dec, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Two reads with different loop-invariant y-offsets but the SAME loop-variant
; x-offset. Each read gets its own AddrPayload and its own SetAddrPayloadField
; for the x-field (field=5).
;
; CHECK-LABEL: define spir_kernel void @test_separate_payloads_same_x
; CHECK-LABEL: entry:
; CHECK: [[P3:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 %offsetY1, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[P4:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 %offsetY2, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; BLOCKY is loop-invariant and baked into CreateAddrPayload — no SetField for field 6.
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P3]], i32 6,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P3]], i32 5, i32 %phi_x, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[P3]], i32 0, i32 0,
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P4]], i32 6,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P4]], i32 5, i32 %phi_x, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[P4]], i32 0, i32 0,

define spir_kernel void @test_separate_payloads_same_x(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY1, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 32
  %Nm1_dec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Nm1_dec, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Three reads with different loop-invariant x-offsets and the SAME loop-variant
; y-offset. Each of the three payloads gets its own SetAddrPayloadField for
; the y-field (field=6).
;
; CHECK-LABEL: define spir_kernel void @test_three_separate_payloads_same_y
; CHECK-LABEL: entry:
; CHECK: [[Q1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[Q2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[Q3:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX3, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; BLOCKX is loop-invariant — no SetField for field 5 on any payload.
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Q1]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Q1]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Q1]], i32 0, i32 0,
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Q2]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Q2]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Q2]], i32 0, i32 0,
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Q3]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Q3]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Q3]], i32 0, i32 0,

define spir_kernel void @test_three_separate_payloads_same_y(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetX2, i32 %offsetX3, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX3, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_y_next = add i32 %phi_y, 16
  %Nm1_dec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Nm1_dec, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; -----------------------------------------------------------------------
; Tests for BLOCKX/BLOCKY offset sharing across different payload groups.
;
; When two reads land in separate payload groups (e.g., different widths),
; their BLOCKX/BLOCKY fields must each get their own SetAddrPayloadField.
; Offset sharing between entries of different groups is invalid because
; the immediate offset would reference a different payload's base value.
;
; These tests use non-zero constant offsets between the two reads'
; BLOCKX/BLOCKY values, verifying that both payloads still receive
; independent SetAddrPayloadField calls.
; -----------------------------------------------------------------------

; Two reads with different loop-invariant x-offsets (separate groups) and
; loop-variant y-offsets that differ by a constant (+16). The second read's
; y has a valid non-zero constant offset (16) to Entry[0], but since the
; reads are in DIFFERENT groups (different x), offset sharing for y is
; invalid. Each payload must get its own SetAddrPayloadField for y.
;
; CHECK-LABEL: define spir_kernel void @test_nonzero_y_offset_separate_groups
; CHECK-LABEL: entry:
; CHECK: [[S1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[S2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; BLOCKX is loop-invariant — no SetField for field 5 on either payload.
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[S1]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[S1]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[S1]], i32 0, i32 0,
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[S2]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[S2]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[S2]], i32 0, i32 0,

define spir_kernel void @test_nonzero_y_offset_separate_groups(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetX2, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  %y_plus_16 = add i32 %phi_y, 16
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 true, i1 false, i32 0)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %y_plus_16, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 true, i1 false, i32 0)
  %phi_y_next = add i32 %phi_y, 32
  %Nm1_dec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Nm1_dec, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Both BLOCKX and BLOCKY are loop-variant. Width differs between the two
; reads, putting them in different payload groups. The second read's X
; has a constant offset (+16) relative to the first read's X. Despite
; this valid-looking offset, the reads belong to different groups, so
; offset sharing is invalid. Both payloads must get their own
; SetAddrPayloadField for both BLOCKX and BLOCKY.
;
; CHECK-LABEL: define spir_kernel void @test_cross_entry_x_offset_width_differs
; CHECK-LABEL: entry:
; CHECK: [[T1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width1, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[T2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width2, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[T1]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[T1]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[T1]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[T2]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[T2]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[T2]], i32 0, i32 0,

define spir_kernel void @test_cross_entry_x_offset_width_differs(i32 %N, i64 %off, i32 %width1, i32 %width2, i32 %height, i32 %pitch, i32 %startX, i32 %startY1, i32 %startY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %startX, %entry ], [ %phi_x_next, %body ]
  %phi_y1 = phi i32 [ %startY1, %entry ], [ %phi_y1_next, %body ]
  %phi_y2 = phi i32 [ %startY2, %entry ], [ %phi_y2_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  %x_plus_16 = add i32 %phi_x, 16
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width1, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width2, i32 %height, i32 %pitch, i32 %x_plus_16, i32 %phi_y2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 32
  %phi_y1_next = add i32 %phi_y1, 16
  %phi_y2_next = add i32 %phi_y2, 16
  %Nm1_dec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Nm1_dec, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Same as above but with BLOCKY having the constant offset (+16) instead
; of BLOCKX. Width differs between the two reads (different groups). The
; second read's Y has a valid constant offset to the first, but since
; the reads are in different payload groups, offset sharing is invalid.
; Both payloads must get their own SetAddrPayloadField for both fields.
;
; CHECK-LABEL: define spir_kernel void @test_cross_entry_y_offset_width_differs
; CHECK-LABEL: entry:
; CHECK: [[U1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width1, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[U2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width2, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[U1]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[U1]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[U1]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[U2]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[U2]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[U2]], i32 0, i32 0,

define spir_kernel void @test_cross_entry_y_offset_width_differs(i32 %N, i64 %off, i32 %width1, i32 %width2, i32 %height, i32 %pitch, i32 %startX1, i32 %startX2, i32 %startY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x1 = phi i32 [ %startX1, %entry ], [ %phi_x1_next, %body ]
  %phi_x2 = phi i32 [ %startX2, %entry ], [ %phi_x2_next, %body ]
  %phi_y = phi i32 [ %startY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  %y_plus_16 = add i32 %phi_y, 16
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width1, i32 %height, i32 %pitch, i32 %phi_x1, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width2, i32 %height, i32 %pitch, i32 %phi_x2, i32 %y_plus_16, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x1_next = add i32 %phi_x1, 32
  %phi_x2_next = add i32 %phi_x2, 32
  %phi_y_next = add i32 %phi_y, 16
  %Nm1_dec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Nm1_dec, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; -----------------------------------------------------------------------
; Test that two reads with different WIDTH variance land in different
; payload groups and each gets its own BLOCKX/BLOCKY SetAddrPayloadField.
;
; LSC1 has loop-invariant WIDTH (function parameter), which is baked
; directly into the payload at creation time. LSC2 has loop-variant
; WIDTH (phi+add), which requires a SetAddrPayloadField at each
; iteration. Despite sharing the same loop-variant BLOCKX and BLOCKY,
; the reads belong to different groups due to the WIDTH difference.
; Both payloads must get their own BLOCKX/BLOCKY SetAddrPayloadField
; calls; sharing offsets between different groups is invalid.
; -----------------------------------------------------------------------
;
; CHECK-LABEL: define spir_kernel void @test_asymmetric_scev_width_hasstep
; CHECK-LABEL: entry:
; P1: width baked into payload
; CHECK: [[V1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width1, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; P2: width not baked (hasStep → SetField instead)
; CHECK: [[V2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 0, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; P1: BLOCKX and BLOCKY SetFields (WIDTH is loop-invariant, already baked):
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[V1]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[V1]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[V1]], i32 0, i32 0,
; P2: WIDTH, BLOCKX, AND BLOCKY SetFields — all three required:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[V2]], i32 2,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[V2]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[V2]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[V2]], i32 0, i32 0,

define spir_kernel void @test_asymmetric_scev_width_hasstep(i32 %N, i64 %off, i32 %width1, i32 %width2, i32 %height, i32 %pitch, i32 %startX, i32 %startY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %startX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %startY, %entry ], [ %phi_y_next, %body ]
  %phi_w = phi i32 [ %width2, %entry ], [ %phi_w_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  ; LSC1: loop-invariant width (%width1)
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width1, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 0)
  ; LSC2: loop-variant width (%phi_w)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %phi_w, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 0)
  %phi_x_next = add i32 %phi_x, 32
  %phi_y_next = add i32 %phi_y, 16
  %phi_w_next = add i32 %phi_w, 1
  %Nm1_dec = sub i32 %Nm1, 1
  %cond = icmp eq i32 %Nm1_dec, 0
  br i1 %cond, label %body, label %exit

exit:
  ret void
}
