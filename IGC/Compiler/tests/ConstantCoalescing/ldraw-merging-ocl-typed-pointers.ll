;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -ocl -inputocl -platformdg2 -igc-constant-coalescing | FileCheck %s

; This test verifies bindless, scalar, uniform loads merging on OpenCL path.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_kernel(<8 x i32> addrspace(1)* %input, <3 x i64> addrspace(1)* %output, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i32 %bufferOffset1, i32 %bindlessOffset, i32 %bindlessOffset2) {
entry:
  %0 = inttoptr i32 %bindlessOffset to <4 x i64> addrspace(2490368)*
; CHECK: call <3 x i64> @llvm.genx.GenISA.ldrawvector.indexed.v3i64.p2490368v4i64(<4 x i64> addrspace(2490368)* %0, i32 0, i32 32, i1 false)
  %1 = call i64 @llvm.genx.GenISA.ldrawvector.indexed.i64.p2490368v4i64(<4 x i64> addrspace(2490368)* %0, i32 0, i32 32, i1 false)
  %2 = call i64 @llvm.genx.GenISA.ldrawvector.indexed.i64.p2490368v4i64(<4 x i64> addrspace(2490368)* %0, i32 8, i32 8, i1 false)
  %3 = call i64 @llvm.genx.GenISA.ldrawvector.indexed.i64.p2490368v4i64(<4 x i64> addrspace(2490368)* %0, i32 16, i32 16, i1 false)
  %4 = inttoptr i32 %bindlessOffset2 to <4 x i64> addrspace(2490368)*
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490368v4i64.i64(<4 x i64> addrspace(2490368)* %4, i32 0, i64 %1, i32 32, i1 false)
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490368v4i64.i64(<4 x i64> addrspace(2490368)* %4, i32 8, i64 %2, i32 8, i1 false)
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490368v4i64.i64(<4 x i64> addrspace(2490368)* %4, i32 16, i64 %3, i32 16, i1 false)
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490368v4i64.i64(<4 x i64> addrspace(2490368)* %4, i32 24, i64 undef, i32 8, i1 false)
  ret void
}

; Function Attrs: argmemonly nounwind readonly
declare i64 @llvm.genx.GenISA.ldrawvector.indexed.i64.p2490368v4i64(<4 x i64> addrspace(2490368)*, i32, i32, i1) #0

; Function Attrs: argmemonly nounwind writeonly
declare void @llvm.genx.GenISA.storerawvector.indexed.p2490368v4i64.i64(<4 x i64> addrspace(2490368)*, i32, i64, i32, i1) #1

attributes #0 = { argmemonly nounwind readonly }
attributes #1 = { argmemonly nounwind writeonly }

!igc.functions = !{!0}
!0 = !{void (<8 x i32> addrspace(1)*, <3 x i64> addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32, i32, i32)* @test_kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
