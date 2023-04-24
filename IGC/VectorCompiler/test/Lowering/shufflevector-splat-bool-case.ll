;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare i1 @llvm.genx.all.v4i1(<4 x i1>)
declare i1 @llvm.genx.all.v8i1(<8 x i1>)

; CHECK-LABEL: @test_pseudoscalar_splat
; CHECK-NEXT: [[LSPLAT:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v1i32.i16(<1 x i32> %l, i32 0, i32 8, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[RSPLAT:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v1i32.i16(<1 x i32> %r, i32 0, i32 8, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[CMP_SPLAT:%[^ ]+]] = icmp eq <8 x i32> [[LSPLAT]], [[RSPLAT]]
; CHECK-NEXT: [[OLD_CMP:%[^ ]+]] = icmp eq <1 x i32> %l, %r
; CHECK-NEXT: [[RES:%[^ ]+]] = tail call i1 @llvm.genx.all.v8i1(<8 x i1> [[CMP_SPLAT]])

define internal spir_func void @test_pseudoscalar_splat(<1 x i32> %l, <1 x i32> %r) {
  %cmp = icmp eq <1 x i32> %l, %r
  %shh = shufflevector <1 x i1> %cmp, <1 x i1> undef, <8 x i32> zeroinitializer
  %res = tail call i1 @llvm.genx.all.v8i1(<8 x i1> %shh)
  ret void
}

; CHECK-LABEL: @test_vector_splat
; CHECK-NEXT: [[CMP:%[^ ]+]] = icmp eq <8 x i32> %l, %r
; CHECK-NEXT: [[SEL:%[^ ]+]] = select <8 x i1> [[CMP]], <8 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <8 x i16> zeroinitializer
; CHECK-NEXT: [[SPLAT:%[^ ]+]] = call <8 x i16> @llvm.genx.rdregioni.v8i16.v8i16.i16(<8 x i16> [[SEL]], i32 0, i32 8, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[NZ:%[^ ]+]] = icmp ne <8 x i16> [[SPLAT]], zeroinitializer
; CHECK-NEXT: [[RES:%[^ ]+]] = tail call i1 @llvm.genx.all.v8i1(<8 x i1> [[NZ]]
define internal spir_func void @test_vector_splat(<8 x i32> %l, <8 x i32> %r) {
  %cmp = icmp eq <8 x i32> %l, %r
  %shh = shufflevector <8 x i1> %cmp, <8 x i1> undef, <8 x i32> zeroinitializer
  %res = tail call i1 @llvm.genx.all.v8i1(<8 x i1> %shh)
  ret void
}

; CHECK-LABEL: @test_vector_splat_width_not_equal
; CHECK-NEXT: [[L:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> %l, i32 0, i32 4, i32 0, i16 12, i32 undef)
; CHECK-NEXT: [[R:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> %r, i32 0, i32 4, i32 0, i16 12, i32 undef)
; CHECK-NEXT: [[CMP:%[^ ]+]] = icmp eq <4 x i32> [[L]], [[R]]
; CHECK-NEXT: [[OLD_CMP:%[^ ]+]] = icmp eq <8 x i32> %l, %r
; CHECK-NEXT: [[RES:%[^ ]+]] = tail call i1 @llvm.genx.all.v4i1(<4 x i1> [[CMP]])
define internal spir_func void @test_vector_splat_width_not_equal(<8 x i32> %l, <8 x i32> %r) {
  %cmp = icmp eq <8 x i32> %l, %r
  %shh = shufflevector <8 x i1> %cmp, <8 x i1> undef, <4 x i32> <i32 3, i32 3, i32 3, i32 3>
  %res = tail call i1 @llvm.genx.all.v4i1(<4 x i1> %shh)
  ret void
}

; CHECK-LABEL: @test_vector_splat_width_not_equal_no_icmp
; CHECK-NEXT: [[SELECT:%[^ ]+]] = select <8 x i1> %val, <8 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <8 x i16> zeroinitializer
; CHECK-NEXT: [[RDREGION:%[^ ]+]] = call i16 @llvm.genx.rdregioni.i16.v8i16.i16(<8 x i16> [[SELECT]], i32 0, i32 1, i32 1, i16 6, i32 0)
; CHECK-NEXT: [[ICMP:%[^ ]+]] = icmp ne i16 [[RDREGION]], 0
; CHECK-NEXT: [[SELECT1:%[^ ]+]] = select i1 [[ICMP]], i16 1, i16 0
; CHECK-NEXT: [[BITCAST:%[^ ]+]] = bitcast i16 [[SELECT1]] to <1 x i16>
; CHECK-NEXT: [[RDREGION1:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v1i16.i16(<1 x i16> [[BITCAST]], i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[ICMP1:%[^ ]+]] = icmp ne <4 x i16> [[RDREGION1]], zeroinitializer
; CHECK-NEXT: [[RES:%[^ ]+]] = tail call i1 @llvm.genx.all.v4i1(<4 x i1> [[ICMP1]])
define internal spir_func void @test_vector_splat_width_not_equal_no_icmp(<8 x i1> %val) {
  %shh = shufflevector <8 x i1> %val, <8 x i1> undef, <4 x i32> <i32 3, i32 3, i32 3, i32 3>
  %res = tail call i1 @llvm.genx.all.v4i1(<4 x i1> %shh)
  ret void
}

