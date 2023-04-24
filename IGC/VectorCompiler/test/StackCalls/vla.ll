;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrologEpilogInsertion -vc-arg-reg-size=32 -vc-ret-reg-size=12 \
; RUN: -mattr=+ocl_runtime -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

%struct = type { i8, float, i8 }

; CHECK-LABEL: test
define internal spir_func void @test(i32 %n1, i8 %n2)#0 {
entry:
; CHECK:       [[A3_0:[^ ]+]] = zext i8 %n2 to i64
; CHECK-NEXT:  [[A3_1:[^ ]+]] = mul i64 1, [[A3_0]]
; CHECK-NEXT:  [[A3_2:[^ ]+]] = add i64 [[A3_1]], 31
; CHECK-NEXT:  [[A3_3:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK-NEXT:  [[A3_4:[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> [[A3_3]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[A3_5:[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> [[A3_4]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[A3_6:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> [[A3_5]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[A3_7:[^ ]+]] = add i64 [[A3_6]], [[A3_2]]
; CHECK-NEXT:  [[A3_8:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 [[A3_7]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[A3_9:[^ ]+]] = call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 [[A3_8]])
; CHECK-NEXT: [[A3_10:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK-NEXT: [[A3_11:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> [[A3_10]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[A3_12:[^ ]+]] = and i64 [[A3_11]], 4294967264
; CHECK-NEXT: [[A3_13:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 [[A3_12]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[A3_14:[^ ]+]] = call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 [[A3_13]])
; CHECK-NEXT:  [[A2_0:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK-NEXT:  [[A2_1:[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> [[A2_0]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[A2_2:[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> [[A2_1]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[A2_3:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> [[A2_2]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[A2_4:[^ ]+]] = add i64 [[A2_3]], 19
; CHECK-NEXT:  [[A2_5:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 [[A2_4]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[A2_6:[^ ]+]] = call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 [[A2_5]])
; CHECK-NEXT:  [[A2_7:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK-NEXT:  [[A2_8:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> [[A2_7]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[A2_9:[^ ]+]] = and i64 [[A2_8]], 4294967288
; CHECK-NEXT: [[A2_10:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 [[A2_9]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[A2_11:[^ ]+]] = call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 [[A2_10]])
; CHECK-NEXT:  [[A1_0:[^ ]+]] = zext i32 %n1 to i64
; CHECK-NEXT:  [[A1_1:[^ ]+]] = mul i64 4, [[A1_0]]
; CHECK-NEXT:  [[A1_2:[^ ]+]] = add i64 [[A1_1]], 15
; CHECK-NEXT:  [[A1_3:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK-NEXT:  [[A1_4:[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> [[A1_3]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[A1_5:[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> [[A1_4]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[A1_6:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> [[A1_5]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[A1_7:[^ ]+]] = add i64 [[A1_6]], [[A1_2]]
; CHECK-NEXT:  [[A1_8:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 [[A1_7]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[A1_9:[^ ]+]] = call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 [[A1_8]])
; CHECK-NEXT: [[A1_10:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK-NEXT: [[A1_11:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> [[A1_10]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[A1_12:[^ ]+]] = and i64 [[A1_11]], 4294967280
; CHECK-NEXT: [[A1_13:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 [[A1_12]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[A1_14:[^ ]+]] = call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 [[A1_13]])

; CHECK: [[A1:[^ ]+]] = inttoptr i64 [[A1_6]] to i32*
  %a1 = alloca i32, i32 %n1
; CHECK: [[A2:[^ ]+]] = inttoptr i64 [[A2_3]] to %struct*
  %a2 = alloca %struct, align 32
; CHECK: [[A3:[^ ]+]] = inttoptr i64 [[A3_6]] to i8*
  %a3 = alloca i8, i8 %n2, align 64
  ret void
}

;attributes #0 = { noinline nounwind readnone "CMStackCall" "VCFunction" "VCStackCall" }

