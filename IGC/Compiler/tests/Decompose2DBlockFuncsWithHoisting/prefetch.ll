;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting --regkey=AllowPrefetchDecomposeWithHoisting=1 -dce -platformbmg -S < %s 2>&1 | FileCheck %s

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

; Test: Basic prefetch decomposition with loop-dependent coordinates
; Verifies that prefetch intrinsics are decomposed when AllowPrefetchDecomposeWithHoisting=1
; CHECK-LABEL: define spir_kernel void @test_prefetch_basic
; CHECK-LABEL: entry:
; CHECK: [[PF_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)

define spir_kernel void @test_prefetch_basic(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 32
  %phi_y_next = add i32 %phi_y, 64
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Prefetch with immediate offsets between calls
; Verifies that prefetches with constant offset differences share payload via immediate offsets
; CHECK-LABEL: define spir_kernel void @test_prefetch_immoffsets
; CHECK-LABEL: entry:
; CHECK: [[PF_IMM_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_IMM_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_IMM_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_IMM_PAYLOAD]], i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_IMM_PAYLOAD]], i32 16, i32 8, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_IMM_PAYLOAD]], i32 32, i32 16, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)

define spir_kernel void @test_prefetch_immoffsets(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
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
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 64
  %phi_y_next = add i32 %phi_y, 32
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Prefetch only X varies in loop, Y is loop-invariant
; Verifies setfield only for X, Y goes into payload creation
; CHECK-LABEL: define spir_kernel void @test_prefetch_only_x_varying
; CHECK-LABEL: entry:
; CHECK: [[PF_X_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 %offsetY, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_X_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_X_PAYLOAD]], i32 6,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_X_PAYLOAD]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_X_PAYLOAD]], i32 0, i32 0,

define spir_kernel void @test_prefetch_only_x_varying(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 32
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Prefetch only Y varies in loop, X is loop-invariant
; Verifies setfield only for Y, X goes into payload creation
; CHECK-LABEL: define spir_kernel void @test_prefetch_only_y_varying
; CHECK-LABEL: entry:
; CHECK: [[PF_Y_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_Y_PAYLOAD]], i32 5,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_Y_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_Y_PAYLOAD]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_Y_PAYLOAD]], i32 0, i32 0,

define spir_kernel void @test_prefetch_only_y_varying(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_y_next = add i32 %phi_y, 16
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Mixed prefetch and read sharing payload
; Verifies that prefetches and reads can share payload with setfields
; CHECK-LABEL: define spir_kernel void @test_prefetch_mixed_with_read
; CHECK-LABEL: entry:
; CHECK: [[PF_MIXED_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_MIXED_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_MIXED_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_MIXED_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PF_MIXED_PAYLOAD]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_MIXED_PAYLOAD]], i32 32, i32 0,

define spir_kernel void @test_prefetch_mixed_with_read(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %offsetX_pf2 = add i32 %phi_x, 32
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX_pf2, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %phi_x_next = add i32 %phi_x, 64
  %phi_y_next = add i32 %phi_y, 16
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Mixed prefetch, read, and write sharing payload
; Verifies that all three operation types can share a decomposed payload
; CHECK-LABEL: define spir_kernel void @test_prefetch_read_write_mixed
; CHECK-LABEL: entry:
; CHECK: [[PF_ALL_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_ALL_PAYLOAD]], i32 5, i32 %phi_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_ALL_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_ALL_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PF_ALL_PAYLOAD]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[PF_ALL_PAYLOAD]], i32 0, i32 0,

define spir_kernel void @test_prefetch_read_write_mixed(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val)
  %phi_x_next = add i32 %phi_x, 64
  %phi_y_next = add i32 %phi_y, 32
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Prefetch with loop-independent offsets (hoisted to entry)
; Verifies payload creation in entry block for loop-invariant prefetches
; CHECK-LABEL: define spir_kernel void @test_prefetch_loopindependent
; CHECK-LABEL: entry:
; CHECK: [[PF_LI_PAYLOAD1:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK: [[PF_LI_PAYLOAD2:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_LI_PAYLOAD1]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_LI_PAYLOAD2]], i32 0, i32 0,

define spir_kernel void @test_prefetch_loopindependent(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX1, i32 %offsetY1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX2, i32 %offsetY2, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Multiple prefetches in a tile pattern with immediate offsets
; Simulates prefetching a 2x2 tile pattern ahead of computation
; CHECK-LABEL: define spir_kernel void @test_prefetch_tile_pattern
; CHECK-LABEL: entry:
; CHECK: [[PF_TILE_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_TILE_PAYLOAD]], i32 5, i32 %tile_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_TILE_PAYLOAD]], i32 6, i32 %tile_y, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_TILE_PAYLOAD]], i32 0, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_TILE_PAYLOAD]], i32 16, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_TILE_PAYLOAD]], i32 0, i32 16,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_TILE_PAYLOAD]], i32 16, i32 16,

define spir_kernel void @test_prefetch_tile_pattern(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %baseX, i32 %baseY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %tile_x = phi i32 [ %baseX, %entry ], [ %tile_x_next, %body ]
  %tile_y = phi i32 [ %baseY, %entry ], [ %tile_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %tile_x1 = add i32 %tile_x, 16
  %tile_y1 = add i32 %tile_y, 16
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %tile_x, i32 %tile_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %tile_x1, i32 %tile_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %tile_x, i32 %tile_y1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %tile_x1, i32 %tile_y1, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %tile_x_next = add i32 %tile_x, 32
  %tile_y_next = add i32 %tile_y, 32
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

; Test: Prefetch followed by read and write in pipeline pattern
; Simulates a software pipelining pattern: prefetch next, read current, write result
; CHECK-LABEL: define spir_kernel void @test_prefetch_pipeline_pattern
; CHECK-LABEL: entry:
; CHECK: [[PF_PIPE_PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks)
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_PIPE_PAYLOAD]], i32 5, i32 %prefetch_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[PF_PIPE_PAYLOAD]], i32 6, i32 %phi_y, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[PF_PIPE_PAYLOAD]], i32 0, i32 0,
; CHECK: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[PF_PIPE_PAYLOAD]], i32 -64, i32 0,
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32(ptr [[PF_PIPE_PAYLOAD]], i32 -64, i32 0,

define spir_kernel void @test_prefetch_pipeline_pattern(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val) {
entry:
  br label %body

body:
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %phi_y = phi i32 [ %offsetY, %entry ], [ %phi_y_next, %body ]
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  ; Prefetch next iteration's data
  %prefetch_x = add i32 %phi_x, 64
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %prefetch_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  ; Read current iteration's data
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  ; Write result
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %phi_y, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val)
  %phi_x_next = add i32 %phi_x, 64
  %phi_y_next = add i32 %phi_y, 16
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}
