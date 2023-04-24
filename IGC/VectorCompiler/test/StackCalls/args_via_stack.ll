;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrologEpilogInsertion -vc-arg-reg-size=32 -vc-ret-reg-size=12 \
; RUN: -mattr=+ocl_runtime -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; COM: This test checks only the current state of prologepilog transformation.
; COM: So, quite strange things can be observed here such as the absence of SP
; COM: increment.

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: foo
; COM: ARG is fully occupied by the first arg.
; CHECK: %[[ARGREAD:[^ ]+]] = call <256 x i32> @llvm.genx.read.predef.reg.v256i32.v256i32(i32 8, <256 x i32> undef)
; CHECK: %[[ARG0:[^ ]+]] = call <256 x i32> @llvm.genx.rdregioni.v256i32.v256i32.i16(<256 x i32> %[[ARGREAD]], i32 0, i32 256, i32 1, i16 0, i32 undef)

; COM: read SP
; CHECK: %[[SPREAD:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: %[[SP:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[SPREAD]], i32 0, i32 1, i32 1, i16 0, i32 undef)

; COM: Calculate arguments offset in stack
; CHECK: %[[ARGBEGIN:[^ ]+]] = sub i64 %[[SP]], 1072

; COM: Other args are passed in stack. Check that simple llvm loads are generated.
; CHECK: %[[SP1PTR:[^ ]+]] = inttoptr i64 %[[ARGBEGIN]] to i32*
; CHECK: %[[ARG1:[^ ]+]] = load i32, i32* %[[SP1PTR]]

; CHECK: %[[SP2:[^ ]+]] = add i64 %[[ARGBEGIN]], 16
; CHECK: %[[SP2PTR:[^ ]+]] = inttoptr i64 %[[SP2]] to <256 x i32>*
; CHECK: %[[ARG2:[^ ]+]] =  load <256 x i32>, <256 x i32>* %[[SP2PTR]]

; CHECK: %[[SP3:[^ ]+]] = add i64 %[[SP2]], 1024
; CHECK: %[[SP3PTR:[^ ]+]] = inttoptr i64 %[[SP3]] to <31 x i8>*
; CHECK: %[[ARG3:[^ ]+]] =  load <31 x i8>, <31 x i8>* %[[SP3PTR]]

define internal spir_func i32 @foo(<256 x i32> %0, i32 %1, <256 x i32> %2, <31 x i8> %3) #0 {
entry:
  ret i32 %1
}

declare spir_func <256 x i32> @get_arg()
declare spir_func i32 @get_arg1()
declare spir_func <31 x i8> @get_arg2()

; CHECK-LABEL: bar
; COM: IGC calling conv: tmpFP = FP; FP = SP.
; CHECK: %[[FPREAD:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 11, i64 undef)
; CHECK: %[[FPRDR:[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %[[FPREAD]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[FPCOPY:[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> %[[FPRDR]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[TMPFP:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[FPCOPY]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[SPREAD:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: %[[SPRDR:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[SPREAD]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[FPWRR:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[SPRDR]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 11, i64 %[[FPWRR]])

; COM: The first arg fully occupies ARG
; CHECK: %[[ARGREAD:[^ ]+]] = call <256 x i32> @llvm.genx.read.predef.reg.v256i32.v256i32(i32 8, <256 x i32> undef)
; CHECK: %[[ARG0:[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v256i32.i16.i1(<256 x i32> %[[ARGREAD]], <256 x i32> %arg1and3, i32 0, i32 256, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[ARGWRITE:[^ ]+]] = call <256 x i32> @llvm.genx.write.predef.reg.v256i32.v256i32(i32 8, <256 x i32> %[[ARG0]])

; COM: read SP
; CHECK: %[[SPREAD:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: %[[SPRDR:[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %[[SPREAD]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[SPCOPY:[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> %[[SPRDR]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[SP:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[SPCOPY]], i32 0, i32 1, i32 1, i16 0, i32 undef)

; COM: allocate stack for args
; CHECK: %[[NEWSP:[^ ]+]] = add i64 %[[SP]], 1072
; CHECK: %[[NEWSPWRR:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[NEWSP]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %[[NEWSPWRR]])

; COM: Other args are passed in stack. Check that simple llvm stores are generated.
; CHECK: %[[SP1PTR:[^ ]+]] = inttoptr i64 %[[SP]] to i32*
; CHECK: store i32 %arg2, i32* %[[SP1PTR]]

; CHECK: %[[SP2:[^ ]+]] = add i64 %[[SP]], 16
; CHECK: %[[SP2PTR:[^ ]+]] = inttoptr i64 %[[SP2]] to <256 x i32>*
; CHECK:  store <256 x i32> %arg1and3, <256 x i32>* %[[SP2PTR]]

; CHECK: %[[SP3:[^ ]+]] = add i64 %[[SP2]], 1024
; CHECK: %[[SP3PTR:[^ ]+]] = inttoptr i64 %[[SP3]] to <31 x i8>*
; CHECK: store <31 x i8> %arg4, <31 x i8>* %[[SP3PTR]]

; COM: Restore FP and SP: SP = FP; FP = tmpFP.
; CHECK: %[[FPREAD:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 11, i64 undef)
; CHECK: %[[FPRDR:[^ ]+]]  = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[FPREAD]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[SPWRR:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[FPRDR]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %[[SPWRR]])
; CHECK: %[[FPWRR:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[TMPFP]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 11, i64 %[[FPWRR]])

define internal spir_func i32 @bar() #0 {
entry:
  %arg1and3 = call spir_func <256 x i32> @get_arg()
  %arg2 = call spir_func i32 @get_arg1()
  %arg4 = call spir_func <31 x i8> @get_arg2()
  %res = tail call spir_func i32 @foo(<256 x i32> %arg1and3, i32 %arg2, <256 x i32> %arg1and3, <31 x i8> %arg4)
  ret i32 %res
}

; CHECK-NO: readnone "CMStackCall"
attributes #0 = { noinline nounwind readnone "CMStackCall" "VCFunction" "VCStackCall" }

