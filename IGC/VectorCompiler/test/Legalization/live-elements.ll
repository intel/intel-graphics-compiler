;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.oword.ld.v16i32(i32, i32, i32)

declare <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32>, i32, i32, i32, i16, i32)

declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>)

; CHECK-LABEL: @test1
; CHECK: [[LOAD:%[^ ]+]] = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
; CHECK-NEXT: [[LOAD_SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LOAD]], i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[ADD_SPLIT0:%[^ ]+]] = add <8 x i32> [[LOAD_SPLIT0]], <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
; CHECK-NEXT: [[ADD_SPLIT0_JOIN0:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[ADD_SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[LOAD_SPLIT8:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LOAD]], i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK-NEXT: [[ADD_SPLIT8:%[^ ]+]] = add <8 x i32> [[LOAD_SPLIT8]], <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
; CHECK-NEXT: [[ADD_SPLIT8_JOIN8:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[ADD_SPLIT0_JOIN0]], <8 x i32> [[ADD_SPLIT8]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: [[RDREGION:%[^ ]+]] = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[ADD_SPLIT8_JOIN8]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; CHECK-NEXT: tail call void @llvm.genx.oword.st.v8i32(i32 2, i32 0, <8 x i32> [[RDREGION]])
define void @test1() {
  %load = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
  %add = add <16 x i32> %load, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %rdregion = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %add, i32 0, i32 8, i32 1, i16 0, i32 undef)
  tail call void @llvm.genx.oword.st.v8i32(i32 2, i32 0, <8 x i32> %rdregion)
  ret void
}

; CHECK-LABEL: @test2
; CHECK: [[LOAD:%[^ ]+]] = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
; CHECK-NEXT: [[LOAD_SPLIT0:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> [[LOAD]], i32 4, i32 4, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[ADD_SPLIT0:%[^ ]+]] = add <4 x i32> [[LOAD_SPLIT0]], <i32 1, i32 1, i32 1, i32 1>
; CHECK-NEXT: [[ADD_SPLIT0_JOIN0:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v4i32.i16.i1(<16 x i32> undef, <4 x i32> [[ADD_SPLIT0]], i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[LOAD_SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LOAD]], i32 8, i32 8, i32 1, i16 16, i32 undef)
; CHECK-NEXT: [[ADD_SPLIT4:%[^ ]+]] = add <8 x i32> [[LOAD_SPLIT4]], <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
; CHECK-NEXT: [[ADD_SPLIT4_JOIN4:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[ADD_SPLIT0_JOIN0]], <8 x i32> [[ADD_SPLIT4]], i32 0, i32 8, i32 1, i16 16, i32 undef, i1 true)
; CHECK-NEXT: [[LOAD_SPLIT12:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> [[LOAD]], i32 4, i32 4, i32 1, i16 48, i32 undef)
; CHECK-NEXT: [[ADD_SPLIT12:%[^ ]+]] = add <4 x i32> [[LOAD_SPLIT12]], <i32 1, i32 1, i32 1, i32 1>
; CHECK-NEXT: [[ADD_SPLIT12_JOIN12:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v4i32.i16.i1(<16 x i32> [[ADD_SPLIT4_JOIN4]], <4 x i32> [[ADD_SPLIT12]], i32 0, i32 4, i32 1, i16 48, i32 undef, i1 true)
; CHECK-NEXT: [[RDREGION:%[^ ]+]] = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[ADD_SPLIT12_JOIN12]], i32 0, i32 8, i32 1, i16 16, i32 undef)
; CHECK-NEXT:  tail call void @llvm.genx.oword.st.v8i32(i32 2, i32 0, <8 x i32> [[RDREGION]])
define void @test2() {
  %load = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
  %add = add <16 x i32> %load, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %rdregion = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %add, i32 0, i32 8, i32 1, i16 16, i32 undef)
  tail call void @llvm.genx.oword.st.v8i32(i32 2, i32 0, <8 x i32> %rdregion)
  ret void
}

; CHECK-LABEL: @test3
; CHECK: [[LOAD:%[^ ]+]] = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
; CHECK-NEXT: [[LOAD_SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LOAD]], i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[ADD_SPLIT0:%[^ ]+]] = add <8 x i32> [[LOAD_SPLIT0]], <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
; CHECK-NEXT: [[ADD_SPLIT0_JOIN0:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[ADD_SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[LOAD_SPLIT8:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LOAD]], i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK-NEXT: [[ADD_SPLIT8:%[^ ]+]] = add <8 x i32> [[LOAD_SPLIT8]], <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
; CHECK-NEXT: [[ADD_SPLIT8_JOIN8:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[ADD_SPLIT0_JOIN0]], <8 x i32> [[ADD_SPLIT8]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: [[RDREGION:%[^ ]+]] = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[ADD_SPLIT8_JOIN8]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; CHECK-NEXT: tail call void @llvm.genx.oword.st.v8i32(i32 2, i32 0, <8 x i32> [[RDREGION]])
define void @test3() {
  %load = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
  %add = add <16 x i32> %load, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %rdregion = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %add, i32 0, i32 8, i32 1, i16 32, i32 undef)
  tail call void @llvm.genx.oword.st.v8i32(i32 2, i32 0, <8 x i32> %rdregion)
  ret void
}

; CHECK-LABEL: @test4
; CHECK: [[LOAD:%[^ ]+]] = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
; CHECK-NEXT: [[ADD:%[^ ]+]] = add <16 x i32> [[LOAD]], <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
; CHECK-NEXT: [[RDREGION:%[^ ]+]] = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[ADD]], i32 0, i32 8, i32 2, i16 0, i32 undef)
; CHECK-NEXT: tail call void @llvm.genx.oword.st.v8i32(i32 2, i32 0, <8 x i32> [[RDREGION]])
define void @test4() {
  %load = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
  %add = add <16 x i32> %load, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %rdregion = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %add, i32 0, i32 8, i32 2, i16 0, i32 undef)
  tail call void @llvm.genx.oword.st.v8i32(i32 2, i32 0, <8 x i32> %rdregion)
  ret void
}
