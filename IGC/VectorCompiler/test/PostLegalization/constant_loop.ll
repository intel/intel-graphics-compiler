;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -licm -march=genx64 -mcpu=XeHPC -mtriple=spir64 -S < %s | FileCheck %s

define void @test(i64 %arg, <128 x i32> %add) {
; CHECK-LABEL: entry
entry:
; CHECK: [[ZEROES_0:%[^ ]+]] = call <64 x i64> @llvm.genx.wrregioni.v64i64.v16i64.i16.i1(<64 x i64> undef, <16 x i64> zeroinitializer, i32 16, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[ZEROES_16:%[^ ]+]] = call <64 x i64> @llvm.genx.wrregioni.v64i64.v16i64.i16.i1(<64 x i64> [[ZEROES_0]], <16 x i64> zeroinitializer, i32 16, i32 16, i32 1, i16 128, i32 undef, i1 true)
; CHECK-NEXT: [[ZEROES_32:%[^ ]+]] = call <64 x i64> @llvm.genx.wrregioni.v64i64.v16i64.i16.i1(<64 x i64> [[ZEROES_16]], <16 x i64> zeroinitializer, i32 16, i32 16, i32 1, i16 256, i32 undef, i1 true)
; CHECK-NEXT: [[ZEROES_48:%[^ ]+]] = call <64 x i64> @llvm.genx.wrregioni.v64i64.v16i64.i16.i1(<64 x i64> [[ZEROES_32]], <16 x i64> zeroinitializer, i32 16, i32 16, i32 1, i16 384, i32 undef, i1 true)
  br label %loop.outer

; CHECK-LABEL: loop.outer
loop.outer:
  %offset = phi i32 [ 0, %entry ], [ %offset.next, %end.inner ]
  %offset.zext = zext i32 %offset to i64
  %addr = add i64 %arg, %offset.zext
; CHECK: tail call void @llvm.vc.internal.lsc.store.ugm.i1.v2i8.i64.v64i64(i1 true, i8 3, i8 4, i8 8, <2 x i8> zeroinitializer, i64 0, i64 %addr, i16 1, i32 0, <64 x i64> [[ZEROES_48]])
  tail call void @llvm.vc.internal.lsc.store.ugm.i1.v2i8.i64.v64i64(i1 true, i8 3, i8 4, i8 8, <2 x i8> zeroinitializer, i64 0, i64 %addr, i16 1, i32 0, <64 x i64> zeroinitializer)
; CHECK-NOT: call <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1
  %zeroes.0 = call <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1(<128 x i32> undef, <32 x i32> zeroinitializer, i32 32, i32 32, i32 1, i16 0, i32 undef, i1 true)
  %zeroes.32 = call <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1(<128 x i32> %zeroes.0, <32 x i32> zeroinitializer, i32 32, i32 32, i32 1, i16 128, i32 undef, i1 true)
  %zeroes.64 = call <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1(<128 x i32> %zeroes.32, <32 x i32> zeroinitializer, i32 32, i32 32, i32 1, i16 256, i32 undef, i1 true)
  %zeroes.128 = call <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1(<128 x i32> %zeroes.64, <32 x i32> zeroinitializer, i32 32, i32 32, i32 1, i16 384, i32 undef, i1 true)
  br label %loop.inner

; CHECK-LABEL: loop.inner
loop.inner:
  %i = phi i32 [ 0, %loop.outer ], [ %i.next, %loop.inner ]
; CHECK: %val = phi <128 x i32> [ zeroinitializer, %loop.outer ], [ %val.next, %loop.inner ]
  %val = phi <128 x i32> [ %zeroes.128, %loop.outer ], [ %val.next, %loop.inner ]
  %val.next = add <128 x i32> %val, %add
  %i.next = add i32 %i, 1
  %i.cmp = icmp ult i32 %i.next, 1024
  br i1 %i.cmp, label %loop.inner, label %end.inner

end.inner:
  %offset.next = add nuw nsw i32 %offset, 512
  %offset.cmp = icmp ult i32 %offset.next, 32256
  br i1 %offset.cmp, label %loop.outer, label %end.outer

end.outer:
  ret void
}

declare void @llvm.vc.internal.lsc.store.ugm.i1.v2i8.i64.v64i64(i1, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <64 x i64>)
declare <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1(<128 x i32>, <32 x i32>, i32, i32, i32, i16, i32, i1)
