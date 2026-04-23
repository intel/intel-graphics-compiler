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
; XFAIL: *

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
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P1]], i32 5, i32 %offsetX1, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P1]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[P1]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P2]], i32 5, i32 %offsetX2, i1 false)
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
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[P3]], i32 5, i32 %phi_x, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[P3]], i32 0, i32 0,
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
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Q1]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Q1]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Q2]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Q2]], i32 0, i32 0,
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
