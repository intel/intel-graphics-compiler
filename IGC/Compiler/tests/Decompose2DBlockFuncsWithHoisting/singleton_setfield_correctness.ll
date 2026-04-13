;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Tests for singleton LSC groups correctness in Decompose2DBlockFuncsWithHoisting.
; These tests verify that when there is only one LSC intrinsic in a payload group,
; the SetFields are NOT cleared - otherwise payload fields initialized with zero
; placeholders would never be properly set before the IO intrinsic.
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting -dce -platformbmg -S < %s 2>&1 | FileCheck %s

declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

; Test 1: Single LSC read with non-constant offsetX and offsetY
; Verify that SetAddrPayloadField intrinsics are generated for BLOCKX and BLOCKY
; even though there's only one LSC call (singleton group).
;
; CHECK-LABEL: define spir_kernel void @test_singleton_read_with_variable_offsets
; CHECK: entry:
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; For singleton groups, SetAddrPayloadField must still be called to initialize
; the BLOCKX and BLOCKY fields that were set to zero in payload creation
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_singleton_read_with_variable_offsets(i32 %N, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Single LSC read with variable offsets - must have SetFields generated
  %newOffsetX = add i32 %offsetX, %Nm1
  %newOffsetY = add i32 %offsetY, %Nm1
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %newOffsetX, i32 %newOffsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 2: Single LSC write with non-constant offsetX
; Even a single write must have SetField for variable offset.
;
; CHECK-LABEL: define spir_kernel void @test_singleton_write_with_variable_offsetX
; CHECK: entry:
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v8i32

define spir_kernel void @test_singleton_write_with_variable_offsetX(i32 %N, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, <8 x i32> %val) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Single LSC write with variable offsetX
  %newOffsetX = add i32 %offsetX, %Nm1
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %newOffsetX, i32 %offsetY, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11, <8 x i32> %val)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 3: Single LSC read with all constant parameters
; When all parameters are constants, they should be embedded directly in payload creation
; and no SetFields should be needed.
;
; CHECK-LABEL: define spir_kernel void @test_singleton_read_all_constants
; CHECK: entry:
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; With all constants, no SetField calls needed
; CHECK-NOT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_singleton_read_all_constants(i32 %N, i64 %base) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Single LSC read with all constant payload parameters
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %base, i32 1024, i32 512, i32 2048, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}

; Test 4: Single LSC read with loop-variant width (requires SetField)
; Non-constant width that varies with loop must have SetField.
;
; CHECK-LABEL: define spir_kernel void @test_singleton_with_variable_width
; CHECK: entry:
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0
; CHECK: body:
; Variable width requires SetField
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0

define spir_kernel void @test_singleton_with_variable_width(i32 %N, i64 %base, i32 %baseWidth, i32 %height, i32 %pitch, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks) {
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Ndec, %body ]
  ; Width varies with loop iteration
  %width = add i32 %baseWidth, %Nm1
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 0, i32 0, i32 %elemSize, i32 %tileWidth, i32 %tileHeight, i32 %vNumBlocks, i1 false, i1 true, i32 11)
  %Ndec = sub i32 %Nm1, 1
  %success = icmp eq i32 %Ndec, 0
  br i1 %success, label %exit, label %body

exit:
  ret void
}
