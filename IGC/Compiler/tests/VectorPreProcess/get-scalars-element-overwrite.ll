;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -ocl -inputocl -platformdg2 -igc-vectorpreprocess | FileCheck %s

; Test finding vector elements in insertelement instructions used to construct the vector.
; Test that the last inserted value is used.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define void @test_kernel(i8 addrspace(2490368)* %bso, i32 %bufferOffset, half %a, half %b, half %c, half %d, half %e)
{
entry:
  %vec0 = insertelement <5 x half> undef, half %a, i64 0
  %vec1 = insertelement <5 x half> %vec0, half %b, i64 1
  %vec2 = insertelement <5 x half> %vec1, half %c, i64 2
  %vec3 = insertelement <5 x half> %vec2, half %d, i64 3
  %vec4.overwritten = insertelement <5 x half> %vec3, half %d, i64 4
  %vec4 = insertelement <5 x half> %vec4.overwritten, half %e, i64 4
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490368i8.v5f16(i8 addrspace(2490368)* %bso, i32 %bufferOffset, <5 x half> %vec4, i32 16, i1 false)

  ret void
}

; CHECK-LABEL: void @test_kernel(
; CHECK-SAME: i8 addrspace(2490368)* [[BSO:%.*]], i32 [[OFFSET:%.*]], half [[A:%.*]], half [[B:%.*]], half [[C:%.*]], half [[D:%.*]], half [[E:%.*]])

; CHECK: [[VEC0:%.*]] = insertelement <4 x half> undef, half [[A]], i32 0
; CHECK: [[VEC1:%.*]] = insertelement <4 x half> [[VEC0]], half [[B]], i32 1
; CHECK: [[VEC2:%.*]] = insertelement <4 x half> [[VEC1]], half [[C]], i32 2
; CHECK: [[VEC3:%.*]] = insertelement <4 x half> [[VEC2]], half [[D]], i32 3
; CHECK: [[OFF0:%.*]] = add i32 0, [[OFFSET]]
; CHECK: call void @llvm.genx.GenISA.storerawvector.indexed.p2490368i8.v4f16(i8 addrspace(2490368)* [[BSO]], i32 [[OFF0]], <4 x half> [[VEC3]], i32 16, i1 false)
; CHECK: [[OFF1:%.*]] = add i32 8, [[OFFSET]]
; CHECK: call void @llvm.genx.GenISA.storerawvector.indexed.p2490368i8.f16(i8 addrspace(2490368)* [[BSO]], i32 [[OFF1]], half [[E]], i32 8, i1 false)


; Function Desc: Write a vector to a buffer pointer at byte offset
; Output:
; Arg 0: buffer pointer, result of GetBufferPtr
; Arg 1: offset from the base pointer, in bytes
; Arg 2: value to store
; Arg 3: aligment in bytes
; Arg 4: volatile, must be an immediate
; Function Attrs: argmemonly nounwind writeonly
declare void @llvm.genx.GenISA.storerawvector.indexed.p2490368i8.v5f16(i8 addrspace(2490368)*, i32, <5 x half>, i32, i1)

