;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -decompose-2d-block-funcs -platformdg2 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LSCFuncsResolution
; ------------------------------------------------

; Test checks that 2D block intrinsics are split properly

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

define spir_kernel void @test_prefetch(i32 %N) {
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11)
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11)
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

define spir_kernel void @test_read(i32 %N) {
; CHECK-LABEL: body:
; CHECK: [[PAYLOAD:%.+]] = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 1, i32 2, i32 3, i32 4, i32 0, i32 0, i32 8, i32 9, i32 10)
; CHECK-NEXT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* [[PAYLOAD]], i32 5, i32 5, i1 false)
; CHECK-NEXT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* [[PAYLOAD]], i32 6, i32 6, i1 false)
; CHECK-NEXT: = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32(i32* [[PAYLOAD]], i32 0, i32 0, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11)

entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11)
  %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}

define spir_kernel void @test_write(<8 x i32> %val, i32 %N) {
; CHECK-LABEL: body:
; CHECK: [[PAYLOAD:%.+]] = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 1, i32 2, i32 3, i32 4, i32 0, i32 0, i32 8, i32 9, i32 10)
; CHECK-NEXT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* [[PAYLOAD]], i32 5, i32 5, i1 false)
; CHECK-NEXT: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* [[PAYLOAD]], i32 6, i32 6, i1 false)
; CHECK-NEXT: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0i32.v8i32(i32* [[PAYLOAD]], i32 0, i32 0, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11, <8 x i32> %val)
entry:
  br label %body

body:
  %Nm1 = phi i32 [ %N, %entry ], [ %Nm1, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11, <8 x i32> %val) %success = icmp eq i32 %Nm1, 0
  br i1 %success, label %body, label %exit

exit:
  ret void
}
