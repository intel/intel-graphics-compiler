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
; CHECK: [[PAYLOAD1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[PAYLOAD2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[PAYLOAD3:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY2, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD1]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD2]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD3]], i32 16, i32 64, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)

define spir_kernel void @test_loopindependent(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  %offsetX1Shifted = add i32 %offsetX1, 16
  %offsetY2Shifted = add i32 %offsetY2, 64
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1Shifted, i32 %offsetY2Shifted, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_loopindependent_checklicm
; CHECK-LABEL: entry:
; CHECK: [[PAYLOAD_LICM1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[PAYLOAD_LICM2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[ADD_X1:%.+]] = add i32 %offsetX2, %offsetX1
; CHECK: [[ADD_X2:%.+]] = add i32 [[ADD_X1]], 11
; CHECK: [[ADD_Y1:%.+]] = add i32 %offsetY2, %offsetY1
; CHECK: [[ADD_Y2:%.+]] = add i32 [[ADD_Y1]], 22
; CHECK: [[PAYLOAD_LICM3:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 [[ADD_X2]], i32 [[ADD_Y2]], i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD_LICM1]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD_LICM2]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD_LICM3]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)

define spir_kernel void @test_loopindependent_checklicm(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetX1Shifted1 = add i32 %offsetX1, %offsetX2
  %offsetY2Shifted1 = add i32 %offsetY1, %offsetY2
  %offsetX1Shifted11 = add i32 %offsetX1Shifted1, 11
  %offsetY2Shifted22 = add i32 %offsetY2Shifted1, 22
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1Shifted11, i32 %offsetY2Shifted22, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; CHECK-LABEL: entry:
; CHECK: [[PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PAYLOAD]], i32 5, i32 %phi_node1, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PAYLOAD]], i32 6, i32 %phi_node2, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD]], i32 11, i32 22, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PAYLOAD]], i32 33, i32 44, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)

define spir_kernel void @test_loopdependent(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_node1 = phi i32 [ %offsetX, %entry ], [ %phi_offsetX, %body ]
  %phi_node2 = phi i32 [ %offsetY, %entry ], [ %phi_offsetY, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetXShifted1 = add i32 %phi_node1, 11
  %offsetYShifted1 = add i32 %phi_node2, 22
  %offsetXShifted2 = add i32 %phi_node1, 33
  %offsetYShifted2 = add i32 %phi_node2, 44
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetXShifted1, i32 %offsetYShifted1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetXShifted2, i32 %offsetYShifted2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_offsetX = add i32 %phi_node1, 32
  %phi_offsetY = add i32 %phi_node2, 64
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Block write with loop-independent offsets and immediate offsets
; Verifies that writes with constant offset differences share payload via immediate offsets
; CHECK-LABEL: define spir_kernel void @test_write_loopindependent
; CHECK-LABEL: entry:
; CHECK: [[WRITE_PAYLOAD1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[WRITE_PAYLOAD2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_PAYLOAD1]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val1)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_PAYLOAD2]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val2)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_PAYLOAD1]], i32 32, i32 64, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val3)

define spir_kernel void @test_write_loopindependent(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val1, <8 x i32> %val2, <8 x i32> %val3) {
entry:
  %offsetX1Shifted = add i32 %offsetX1, 32
  %offsetY1Shifted = add i32 %offsetY1, 64
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val1)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val2)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1Shifted, i32 %offsetY1Shifted, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val3)
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Block write with loop-dependent offsets and immediate offsets
; Verifies setfield is created for loop-varying base, with immediate offsets for constant differences
; CHECK-LABEL: define spir_kernel void @test_write_loopdependent
; CHECK-LABEL: entry:
; CHECK: [[WRITE_PAYLOAD_DEP:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[WRITE_PAYLOAD_DEP]], i32 5, i32 %phi_node1, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[WRITE_PAYLOAD_DEP]], i32 6, i32 %phi_node2, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_PAYLOAD_DEP]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val1)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[WRITE_PAYLOAD_DEP]], i32 16, i32 32, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val2)

define spir_kernel void @test_write_loopdependent(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val1, <8 x i32> %val2) {
entry:
  br label %body

body:
  %phi_node1 = phi i32 [ %offsetX, %entry ], [ %phi_offsetX, %body ]
  %phi_node2 = phi i32 [ %offsetY, %entry ], [ %phi_offsetY, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetXShifted = add i32 %phi_node1, 16
  %offsetYShifted = add i32 %phi_node2, 32
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val1)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetXShifted, i32 %offsetYShifted, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val2)
  %phi_offsetX = add i32 %phi_node1, 64
  %phi_offsetY = add i32 %phi_node2, 128
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Mixed read and write operations sharing payload via immediate offsets
; Verifies that reads and writes with same base can use immediate offsets
; CHECK-LABEL: define spir_kernel void @test_mixed_read_write_immoffsets
; CHECK-LABEL: entry:
; CHECK: [[MIXED_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[MIXED_PAYLOAD]], i32 5, i32 %phi_node1, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[MIXED_PAYLOAD]], i32 6, i32 %phi_node2, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[MIXED_PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[MIXED_PAYLOAD]], i32 64, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val)

define spir_kernel void @test_mixed_read_write_immoffsets(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val) {
entry:
  br label %body

body:
  %phi_node1 = phi i32 [ %offsetX, %entry ], [ %phi_offsetX, %body ]
  %phi_node2 = phi i32 [ %offsetY, %entry ], [ %phi_offsetY, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetXForWrite = add i32 %phi_node1, 64
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_node1, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetXForWrite, i32 %phi_node2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val)
  %phi_offsetX = add i32 %phi_node1, 128
  %phi_offsetY = add i32 %phi_node2, 64
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Only X offset varies in loop, Y is constant
; Verifies setfield for X only, Y goes into payload creation
; CHECK-LABEL: define spir_kernel void @test_only_x_offset_varying
; CHECK-LABEL: entry:
; CHECK: [[X_ONLY_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 %offsetY, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[X_ONLY_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[X_ONLY_PAYLOAD]], i32 6,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[X_ONLY_PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[X_ONLY_PAYLOAD]], i32 16, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)

define spir_kernel void @test_only_x_offset_varying(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetXShifted = add i32 %phi_x, 16
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetXShifted, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 32
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Only Y offset varies in loop, X is constant
; Verifies setfield for Y only, X goes into payload creation
; CHECK-LABEL: define spir_kernel void @test_only_y_offset_varying
; CHECK-LABEL: entry:
; CHECK: [[Y_ONLY_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Y_ONLY_PAYLOAD]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[Y_ONLY_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Y_ONLY_PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[Y_ONLY_PAYLOAD]], i32 0, i32 8, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)

define spir_kernel void @test_only_y_offset_varying(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetYShifted = add i32 %phi_y, 8
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetYShifted, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_y_next = add i32 %phi_y, 16
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Multiple reads with different constant offsets from same base
; Verifies payload sharing with multiple immediate offsets
; CHECK-LABEL: define spir_kernel void @test_multiple_reads_shared_payload
; CHECK-LABEL: entry:
; CHECK: [[MULTI_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[MULTI_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[MULTI_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[MULTI_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[MULTI_PAYLOAD]], i32 16, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[MULTI_PAYLOAD]], i32 32, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[MULTI_PAYLOAD]], i32 48, i32 0,

define spir_kernel void @test_multiple_reads_shared_payload(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetX1 = add i32 %phi_x, 16
  %offsetX2 = add i32 %phi_x, 32
  %offsetX3 = add i32 %phi_x, 48
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %2 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %3 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX3, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 64
  %phi_y_next = add i32 %phi_y, 16
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Multiple writes with different constant offsets from same base
; Verifies payload sharing for multiple writes with immediate offsets
; CHECK-LABEL: define spir_kernel void @test_multiple_writes_shared_payload
; CHECK-LABEL: entry:
; CHECK: [[MULTI_WRITE_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[MULTI_WRITE_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[MULTI_WRITE_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[MULTI_WRITE_PAYLOAD]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[MULTI_WRITE_PAYLOAD]], i32 0, i32 8,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[MULTI_WRITE_PAYLOAD]], i32 0, i32 16,

define spir_kernel void @test_multiple_writes_shared_payload(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val1, <8 x i32> %val2, <8 x i32> %val3) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetY1 = add i32 %phi_y, 8
  %offsetY2 = add i32 %phi_y, 16
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val1)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val2)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val3)
  %phi_x_next = add i32 %phi_x, 32
  %phi_y_next = add i32 %phi_y, 24
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}
