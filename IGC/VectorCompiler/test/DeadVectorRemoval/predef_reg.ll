;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare  <32 x i32> @llvm.genx.read.predef.reg.v32i32.v32i32(i32 %0, <32 x i32> %1) #3
declare  <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 %1, i32 %2, i32 %3, i16 %4, i32 %5) #1
declare  <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %0, <16 x i32> %1, i32 %2, i32 %3, i32 %4, i16 %5, i32 %6, i1 %7) #1
declare  <32 x i32> @llvm.genx.write.predef.reg.v32i32.v32i32(i32 %0, <32 x i32> %1) #3

; Function Attrs: noinline nounwind
define spir_func void @test(<32 x i32> %arg) {
  %read = call <32 x i32> @llvm.genx.read.predef.reg.v32i32.v32i32(i32 8, <32 x i32> undef)
  %.split05 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %arg, i32 16, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NOT: wrregion{{.*}}(<32 x i32> undef
  %.join0 = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %read, <16 x i32> %.split05, i32 16, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %.split16 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %arg, i32 16, i32 16, i32 1, i16 64, i32 undef)
  %.join16 = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %.join0, <16 x i32> %.split16, i32 16, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %write = call <32 x i32> @llvm.genx.write.predef.reg.v32i32.v32i32(i32 8, <32 x i32> %.join16)
  ret void
}

attributes #0 = { noinline nounwind "CMStackCall" }
attributes #1 = { nounwind readnone }
attributes #3 = { nounwind }

