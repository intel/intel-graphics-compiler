;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrologEpilogInsertion -vc-arg-reg-size=32 -vc-ret-reg-size=12 \
; RUN: -mattr=+ocl_runtime -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

%struct = type { i8, float, i8 }

; CHECK-LABEL: foo
; COM: This test checks memory allocations and some of FP, SP manipulations, skipping
; COM: some of details.
; COM: Also it checks the order of operations related to the calling convention

; COM: read FP and write if to tmpFP
; CHECK: %[[fp_read:[^ ]+]]  = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 11, i64 undef)
; CHECK: %[[fp_rdr:[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %[[fp_read]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[fp_copy:[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> %[[fp_rdr]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[tmp_fp:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[fp_copy]], i32 0, i32 1, i32 1, i16 0, i32 undef)

; COM: SP = FP (details are skipped)
; CHECK: call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 11, i64 %{{.*}})

; COM: read args: the first one from ARG reg, all the rest from the stack.
; CHECK: call <256 x i32> @llvm.genx.read.predef.reg.v256i32.v256i32(i32 8, <256 x i32> undef)
; CHECK: call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: sub i64 %{{.*}}, 1072
; CHECK: load i32, i32* %{{.*}}
; CHECK: add i64 %{{.*}}, 16
; CHECK: load <256 x i32>, <256 x i32>* %{{.*}}
; CHECK: add i64 %{{.*}}, 1024
; CHECK: load <31 x i8>, <31 x i8>* %{{.*}}

; COM: run-time alignment for memory allocations (align to the biggest value)
; CHECK: call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: add i64 %{{.*}}, 63
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %{{.*}})
; CHECK: call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: and i64 %{{.*}}, -64
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %{{.*}})

; COM: memory allocations. They are sorted in descending alignment order.
; CHECK: %[[sp_read:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: %[[sp_rdr:[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %[[sp_read]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; COM: make a copy of SP value because it is the first allocation address
; CHECK: %[[sp_copy:[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> %[[sp_rdr]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[alloca1_addr:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[sp_copy]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; COM: the first allocation requires 1 byte, but the next allocation must be 32 aligned.
; COM: Since SP is 64 bytes aligned (run-time alignment), the next allocation can be
; COM: aligned statically.
; CHECK: add i64 %[[alloca1_addr]], 32
; COM: allocation size for <4 x i64> is 32, preferred type alignment is 32 as well
; CHECK: add i64 %{{.*}}, 32
; COM: allocation size for %struct is 12, alignment requirement is 8 (default).
; CHECK: add i64 %{{.*}}, 32
; COM: allocation size is 24, alignment requirement is 16, because after all allocations stack must be oword aligned (general alignment requirement).
; CHECK add i64 %{{.*}}, 32

; COM: restore SP and FP.
; CHECK: %[[fp_read:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 11, i64 undef)
; CHECK: %[[fp_rdr:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[fp_read]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[sp_wrr:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[fp_rdr]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %[[sp_wrr]])
; CHECK: %[[fp_wrr:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[tmp_fp]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 11, i64 %[[fp_wrr]])

define internal spir_func void @foo(<256 x i32> %0, i32 %1, <256 x i32> %2, <31 x i8> %3) #0 {
entry:
  %a1 = alloca i32, i32 6
  %a2 = alloca %struct, align 32
  %a3 = alloca i8, i8 1, align 64
  %a4 = alloca <4 x i64>
  ret void
}

attributes #0 = { noinline nounwind readnone "CMStackCall" "VCFunction" "VCStackCall" }
