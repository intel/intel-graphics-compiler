;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Tests that redundant SetAddrPayloadField intrinsics are NOT emitted for
; loop-invariant BLOCKX/BLOCKY fields whose values are already baked into
; the CreateAddrPayload call.
;
; When a BLOCKX or BLOCKY field is loop-invariant, the pass expands its
; start value directly into the CreateAddrPayload call at the preheader.
; No SetAddrPayloadField should be emitted for such fields inside the
; loop, matching the existing behavior for WIDTH/HEIGHT/PITCH.
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting -dce -platformbmg -S < %s 2>&1 | FileCheck %s

declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

; Test 1: Loop-invariant BLOCKX, loop-variant BLOCKY.
; BLOCKX (%offsetX) is baked into payload creation. No redundant SetField for
; BLOCKX should be emitted. BLOCKY (%phi_y) correctly gets a SetField.
;
; CHECK-LABEL: define spir_kernel void @test_redundant_setfield_blockx
; CHECK-LABEL: entry:
; The payload must have %offsetX baked into position 4 (BLOCKX) and 0 for BLOCKY:
; CHECK: [[P1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; No redundant SetField for BLOCKX — %offsetX is already in the payload:
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P1]], i32 5,
; Required SetField for BLOCKY — loop-variant:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P1]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[P1]], i32 0, i32 0,

define spir_kernel void @test_redundant_setfield_blockx(i32 %N, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_y_next = add i32 %phi_y, 32
  %Nm1_dec = sub i32 %Nm1, 1
  %cond = icmp eq i32 %Nm1_dec, 0
  br i1 %cond, label %body, label %exit

exit:
  ret void
}

; Test 2: Loop-variant BLOCKX, loop-invariant BLOCKY.
; BLOCKY (%offsetY) is baked into payload creation. No redundant SetField for
; BLOCKY should be emitted. BLOCKX (%phi_x) correctly gets a SetField.
;
; CHECK-LABEL: define spir_kernel void @test_redundant_setfield_blocky
; CHECK-LABEL: entry:
; The payload must have 0 for BLOCKX and %offsetY baked into position 5 (BLOCKY):
; CHECK: [[P2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 0, i32 %offsetY, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; Required SetField for BLOCKX — loop-variant:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P2]], i32 5, i32 %phi_x, i1 false)
; No redundant SetField for BLOCKY — %offsetY is already in the payload:
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P2]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[P2]], i32 0, i32 0,

define spir_kernel void @test_redundant_setfield_blocky(i32 %N, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 32
  %Nm1_dec = sub i32 %Nm1, 1
  %cond = icmp eq i32 %Nm1_dec, 0
  br i1 %cond, label %body, label %exit

exit:
  ret void
}

; Test 3: Both BLOCKX and BLOCKY loop-invariant (only induction counter varies).
; Both values are baked into payload creation. Neither should get a redundant SetField.
;
; CHECK-LABEL: define spir_kernel void @test_redundant_setfield_both
; CHECK-LABEL: entry:
; WIDTH is loop-variant so stays 0; BLOCKX and BLOCKY are both baked in:
; CHECK: [[P3:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 0, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; Needed SetField for WIDTH — loop-variant:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P3]], i32 2, i32 %phi_w, i1 false)
; No redundant SetField for BLOCKX:
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P3]], i32 5,
; No redundant SetField for BLOCKY:
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P3]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[P3]], i32 0, i32 0,

define spir_kernel void @test_redundant_setfield_both(i32 %N, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  ; Loop-variant width to force the LSC into a loop context
  %phi_w = phi i32 [ %width, %entry ], [ %phi_w_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1_dec, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %base, i32 %phi_w, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_w_next = add i32 %phi_w, 1
  %Nm1_dec = sub i32 %Nm1, 1
  %cond = icmp eq i32 %Nm1_dec, 0
  br i1 %cond, label %body, label %exit

exit:
  ret void
}
