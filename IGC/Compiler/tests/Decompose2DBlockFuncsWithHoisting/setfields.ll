;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting -dce -platformbmg -S < %s 2>&1 | FileCheck %s

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

; CHECK-LABEL: entry:
; CHECK: [[PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PAYLOAD]], i32 5, i32 %phi_node1, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PAYLOAD]], i32 6, i32 %phi_node2, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)

define spir_kernel void @test_loopdependent1(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_node1 = phi i32 [ %offsetX, %entry ], [ %phi_offsetX, %body ]
  %phi_node2 = phi i32 [ %offsetY, %entry ], [ %phi_offsetY, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_offsetX = add i32 %phi_node1, 32
  %phi_offsetY = add i32 %phi_node2, 64
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_loopdependent2
; CHECK-LABEL: entry:
; CHECK: [[PAYLOAD2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PAYLOAD2]], i32 5, i32 %phi_node1, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PAYLOAD2]], i32 6, i32 %phi_node2, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD2]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD2]], i32 11, i32 22, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD2]], i32 33, i32 44, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)

define spir_kernel void @test_loopdependent2(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_node1 = phi i32 [ %offsetX, %entry ], [ %phi_offsetX, %body ]
  %phi_node2 = phi i32 [ %offsetY, %entry ], [ %phi_offsetY, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %phi_node1_shifted1 = add i32 %phi_node1, 11
  %phi_node2_shifted1 = add i32 %phi_node2, 22
  %phi_node1_shifted2 = add i32 %phi_node1, 33
  %phi_node2_shifted2 = add i32 %phi_node2, 44
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1_shifted1, i32 %phi_node2_shifted1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1_shifted2, i32 %phi_node2_shifted2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_offsetX = add i32 %phi_node1, 32
  %phi_offsetY = add i32 %phi_node2, 64
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Block write with loop-dependent setfields
; Verifies setfield creation for loop-varying coordinates in writes
; CHECK-LABEL: define spir_kernel void @test_write_setfields
; CHECK-LABEL: entry:
; CHECK: [[WRITE_SF_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[WRITE_SF_PAYLOAD]], i32 5, i32 %phi_node1, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[WRITE_SF_PAYLOAD]], i32 6, i32 %phi_node2, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_SF_PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val1)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_SF_PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val2)

define spir_kernel void @test_write_setfields(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val1, <8 x i32> %val2) {
entry:
  br label %body

body:
  %phi_node1 = phi i32 [ %offsetX, %entry ], [ %phi_offsetX, %body ]
  %phi_node2 = phi i32 [ %offsetY, %entry ], [ %phi_offsetY, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val1)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val2)
  %phi_offsetX = add i32 %phi_node1, 32
  %phi_offsetY = add i32 %phi_node2, 64
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Mixed read and write with loop-dependent setfields
; Verifies that reads and writes can share payload with setfields
; CHECK-LABEL: define spir_kernel void @test_mixed_read_write_setfields
; CHECK-LABEL: entry:
; CHECK: [[MIXED_SF_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[MIXED_SF_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[MIXED_SF_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[MIXED_SF_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[MIXED_SF_PAYLOAD]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[MIXED_SF_PAYLOAD]], i32 0, i32 0,

define spir_kernel void @test_mixed_read_write_setfields(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val)
  %phi_x_next = add i32 %phi_x, 64
  %phi_y_next = add i32 %phi_y, 32
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Only X varies in loop with setfield, Y is loop-invariant
; Verifies setfield only for X, Y goes into payload creation
; CHECK-LABEL: define spir_kernel void @test_setfields_only_x_varying
; CHECK-LABEL: entry:
; CHECK: [[X_SF_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 %offsetY, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[X_SF_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[X_SF_PAYLOAD]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[X_SF_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[X_SF_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[X_SF_PAYLOAD]], i32 0, i32 0,

define spir_kernel void @test_setfields_only_x_varying(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 32
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Only Y varies in loop with setfield, X is loop-invariant
; Verifies setfield only for Y, X goes into payload creation
; CHECK-LABEL: define spir_kernel void @test_setfields_only_y_varying
; CHECK-LABEL: entry:
; CHECK: [[Y_SF_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Y_SF_PAYLOAD]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Y_SF_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Y_SF_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Y_SF_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Y_SF_PAYLOAD]], i32 0, i32 0,

define spir_kernel void @test_setfields_only_y_varying(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_y_next = add i32 %phi_y, 16
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Block writes with setfields and immediate offsets combined
; Verifies setfield for base with immediate offsets for differences
; CHECK-LABEL: define spir_kernel void @test_write_setfields_with_immoffsets
; CHECK-LABEL: entry:
; CHECK: [[WRITE_COMBO_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[WRITE_COMBO_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[WRITE_COMBO_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_COMBO_PAYLOAD]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_COMBO_PAYLOAD]], i32 16, i32 8,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_COMBO_PAYLOAD]], i32 32, i32 16,

define spir_kernel void @test_write_setfields_with_immoffsets(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val1, <8 x i32> %val2, <8 x i32> %val3) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetX1 = add i32 %phi_x, 16
  %offsetY1 = add i32 %phi_y, 8
  %offsetX2 = add i32 %phi_x, 32
  %offsetY2 = add i32 %phi_y, 16
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val1)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val2)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val3)
  %phi_x_next = add i32 %phi_x, 64
  %phi_y_next = add i32 %phi_y, 32
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Multiple reads and writes interleaved with loop-dependent coordinates
; Verifies proper setfield and payload sharing for interleaved operations
; CHECK-LABEL: define spir_kernel void @test_interleaved_read_write
; CHECK-LABEL: entry:
; CHECK: [[INTERLEAVED_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[INTERLEAVED_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[INTERLEAVED_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[INTERLEAVED_PAYLOAD]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[INTERLEAVED_PAYLOAD]], i32 32, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[INTERLEAVED_PAYLOAD]], i32 64, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[INTERLEAVED_PAYLOAD]], i32 96, i32 0,

define spir_kernel void @test_interleaved_read_write(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val1, <8 x i32> %val2) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetX_w1 = add i32 %phi_x, 32
  %offsetX_r2 = add i32 %phi_x, 64
  %offsetX_w2 = add i32 %phi_x, 96
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX_w1, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val1)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX_r2, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX_w2, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val2)
  %phi_x_next = add i32 %phi_x, 128
  %phi_y_next = add i32 %phi_y, 16
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Tile-based matrix access pattern with setfields
; Simulates a common matrix tiling pattern with loop-dependent base and tile offsets
; CHECK-LABEL: define spir_kernel void @test_matrix_tile_access
; CHECK-LABEL: entry:
; CHECK: [[TILE_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[TILE_PAYLOAD]], i32 5, i32 %tile_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[TILE_PAYLOAD]], i32 6, i32 %tile_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[TILE_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[TILE_PAYLOAD]], i32 16, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[TILE_PAYLOAD]], i32 0, i32 16,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[TILE_PAYLOAD]], i32 16, i32 16,

define spir_kernel void @test_matrix_tile_access(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %baseX, i32 %baseY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %tile_x = phi i32 [ %baseX, %entry ], [ %tile_x_next, %body ]
  %tile_y = phi i32 [ %baseY, %entry ], [ %tile_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  ; Load 4 tiles in a 2x2 pattern
  %tile_x1 = add i32 %tile_x, 16
  %tile_y1 = add i32 %tile_y, 16
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %tile_x, i32 %tile_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %tile_x1, i32 %tile_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %tile_x, i32 %tile_y1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %3 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %tile_x1, i32 %tile_y1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %tile_x_next = add i32 %tile_x, 32
  %tile_y_next = add i32 %tile_y, 32
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Loop-variant WIDTH (field 2) - should generate SetField
; Verifies SetField creation for non-constant width parameter
; CHECK-LABEL: define spir_kernel void @test_setfield_width_variant
; CHECK-LABEL: entry:
; CHECK: [[WIDTH_SF_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 0, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[WIDTH_SF_PAYLOAD]], i32 2, i32 %phi_width, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[WIDTH_SF_PAYLOAD]],

define spir_kernel void @test_setfield_width_variant(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_width = phi i32 [ %width, %entry ], [ %phi_width_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %phi_width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_width_next = add i32 %phi_width, 8
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Loop-variant HEIGHT (field 3) - should generate SetField
; Verifies SetField creation for non-constant height parameter
; CHECK-LABEL: define spir_kernel void @test_setfield_height_variant
; CHECK-LABEL: entry:
; CHECK: [[HEIGHT_SF_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 0, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[HEIGHT_SF_PAYLOAD]], i32 3, i32 %phi_height, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[HEIGHT_SF_PAYLOAD]],

define spir_kernel void @test_setfield_height_variant(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_height = phi i32 [ %height, %entry ], [ %phi_height_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %phi_height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_height_next = add i32 %phi_height, 4
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Loop-variant PITCH (field 4) - should generate SetField
; Verifies SetField creation for non-constant pitch parameter
; CHECK-LABEL: define spir_kernel void @test_setfield_pitch_variant
; CHECK-LABEL: entry:
; CHECK: [[PITCH_SF_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 0, i32 %offsetX, i32 %offsetY, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PITCH_SF_PAYLOAD]], i32 4, i32 %phi_pitch, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PITCH_SF_PAYLOAD]],

define spir_kernel void @test_setfield_pitch_variant(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_pitch = phi i32 [ %pitch, %entry ], [ %phi_pitch_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %phi_pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_pitch_next = add i32 %phi_pitch, 16
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Loop-variant BASE (field 1/arg 0) - should NOT generate SetField, should abort decomposition
; Verifies that loop-variant base address prevents decomposition
; CHECK-LABEL: define spir_kernel void @test_no_setfield_base_variant
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-NOT: LSC2DBlockSetAddrPayloadField
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %phi_base,

define spir_kernel void @test_no_setfield_base_variant(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_base = phi i64 [ %off, %entry ], [ %phi_base_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %phi_base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_base_next = add i64 %phi_base, 1024
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Loop-variant TileWidth (arg 6) - should NOT generate SetField, should abort decomposition
; Verifies that loop-variant tile configuration prevents decomposition
; CHECK-LABEL: define spir_kernel void @test_no_setfield_tilewidth_variant
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-NOT: LSC2DBlockSetAddrPayloadField
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %phi_tileWidth,

define spir_kernel void @test_no_setfield_tilewidth_variant(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_tileWidth = phi i32 [ %tileWidth, %entry ], [ %phi_tileWidth_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %phi_tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_tileWidth_next = add i32 %phi_tileWidth, 1
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Loop-variant TileHeight (arg 7) - should NOT generate SetField, should abort decomposition
; Verifies that loop-variant tile configuration prevents decomposition
; CHECK-LABEL: define spir_kernel void @test_no_setfield_tileheight_variant
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-NOT: LSC2DBlockSetAddrPayloadField
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %phi_tileHeight,

define spir_kernel void @test_no_setfield_tileheight_variant(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_tileHeight = phi i32 [ %tileHeight, %entry ], [ %phi_tileHeight_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %phi_tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_tileHeight_next = add i32 %phi_tileHeight, 1
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Loop-variant VNumBlocks (arg 8) - should NOT generate SetField, should abort decomposition
; Verifies that loop-variant vnumBlocks parameter prevents decomposition
; CHECK-LABEL: define spir_kernel void @test_no_setfield_vnumblocks_variant
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-NOT: LSC2DBlockSetAddrPayloadField
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %phi_vNumBlocks,

define spir_kernel void @test_no_setfield_vnumblocks_variant(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_vNumBlocks = phi i32 [ %vNumBlocks, %entry ], [ %phi_vNumBlocks_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %phi_vNumBlocks, i1 false, i1 true, i32 11)
  %phi_vNumBlocks_next = add i32 %phi_vNumBlocks, 1
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}