;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper \
; RUN:  -GenXAddressCommoningWrapper -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN:  -mcpu=Gen9 -S < %s | FileCheck %s
; REQUIRES: llvm_16_or_greater

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define spir_kernel void @"foo<char, 2, 8>"() #0 {
entry:
  call spir_func void @_Z4funcIcLi2ELi8EEvyiiu2CMmrT0_xT1__T_()
  ret void
}

; CHECK: define internal spir_func void @_Z4funcIcLi2ELi8EEvyiiu2CMmrT0_xT1__T_
; CHECK: entry:
; CHECK: %iselect.split14.multiindirect_idx_subregion.categoryconv3 =
; CHECK-SAME: call <1 x i16> @llvm.genx.rdregioni.v1i16.v2i16.i16(<2 x i16> {{.*}}, i32 1, i32 1, i32 1, i16 2, i32 undef)

define internal spir_func void @_Z4funcIcLi2ELi8EEvyiiu2CMmrT0_xT1__T_() {
entry:
  %rdregioni = call <16 x i16> @llvm.genx.rdregioni.v16i16.v1i16.i16(<1 x i16> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0)
  %constant.split.int81 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v8i8.i16.i1(<16 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %iselect.split7.multiindirect_idx_subregion = call <1 x i16> @llvm.genx.rdregioni.v1i16.v16i16.i16(<16 x i16> %rdregioni, i32 1, i32 0, i32 0, i16 0, i32 0)
  %iselect.split7.multiindirect_idx_subregion.categoryconv = call <1 x i16> @llvm.genx.convert.addr.v1i16(<1 x i16> %iselect.split7.multiindirect_idx_subregion, i16 0)
  %iselect.split7 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v16i8.v1i16(<16 x i8> %constant.split.int81, i32 0, i32 1, i32 0, <1 x i16> %iselect.split7.multiindirect_idx_subregion.categoryconv, i32 0)
  %iselect.split7.join7 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v1i8.i16.i1(<16 x i8> zeroinitializer, <1 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %iselect.split8.multiindirect_idx_subregion.categoryconv = call <1 x i16> @llvm.genx.convert.addr.v1i16(<1 x i16> zeroinitializer, i16 0)
  %iselect.split8 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v16i8.v1i16(<16 x i8> zeroinitializer, i32 0, i32 0, i32 0, <1 x i16> zeroinitializer, i32 0)
  %iselect.split8.join8 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v1i8.i16.i1(<16 x i8> zeroinitializer, <1 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %iselect.split9.multiindirect_idx_subregion = call <1 x i16> @llvm.genx.rdregioni.v1i16.v16i16.i16(<16 x i16> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0)
  %iselect.split9.multiindirect_idx_subregion.categoryconv = call <1 x i16> @llvm.genx.convert.addr.v1i16(<1 x i16> zeroinitializer, i16 0)
  %iselect.split9 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v16i8.v1i16(<16 x i8> zeroinitializer, i32 0, i32 0, i32 0, <1 x i16> zeroinitializer, i32 0)
  %iselect.split9.join9 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v1i8.i16.i1(<16 x i8> zeroinitializer, <1 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %iselect.split10.multiindirect_idx_subregion = call <1 x i16> @llvm.genx.rdregioni.v1i16.v16i16.i16(<16 x i16> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0)
  %iselect.split10.multiindirect_idx_subregion.categoryconv = call <1 x i16> @llvm.genx.convert.addr.v1i16(<1 x i16> zeroinitializer, i16 0)
  %iselect.split10 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v16i8.v1i16(<16 x i8> zeroinitializer, i32 0, i32 0, i32 0, <1 x i16> zeroinitializer, i32 0)
  %iselect.split10.join10 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v1i8.i16.i1(<16 x i8> zeroinitializer, <1 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %iselect.split11.join11 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v1i8.i16.i1(<16 x i8> zeroinitializer, <1 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %iselect.split12.multiindirect_idx_subregion = call <1 x i16> @llvm.genx.rdregioni.v1i16.v16i16.i16(<16 x i16> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0)
  %iselect.split12.multiindirect_idx_subregion.categoryconv = call <1 x i16> @llvm.genx.convert.addr.v1i16(<1 x i16> zeroinitializer, i16 0)
  %iselect.split12 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v16i8.v1i16(<16 x i8> zeroinitializer, i32 0, i32 0, i32 0, <1 x i16> zeroinitializer, i32 0)
  %iselect.split12.join12 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v1i8.i16.i1(<16 x i8> zeroinitializer, <1 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %iselect.split13.join13 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v1i8.i16.i1(<16 x i8> zeroinitializer, <1 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %iselect.split14.multiindirect_idx_subregion = call <1 x i16> @llvm.genx.rdregioni.v1i16.v16i16.i16(<16 x i16> %rdregioni, i32 0, i32 0, i32 0, i16 0, i32 0)
  %iselect.split14.multiindirect_idx_subregion.categoryconv = call <1 x i16> @llvm.genx.convert.addr.v1i16(<1 x i16> %iselect.split14.multiindirect_idx_subregion, i16 0)
  %iselect.split14 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v16i8.v1i16(<16 x i8> %constant.split.int81, i32 0, i32 1, i32 0, <1 x i16> %iselect.split14.multiindirect_idx_subregion.categoryconv, i32 0)
  %iselect.split14.join14 = call <16 x i8> @llvm.genx.wrregioni.v16i8.v1i8.i16.i1(<16 x i8> zeroinitializer, <1 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %iselect.split15.multiindirect_idx_subregion = call <1 x i16> @llvm.genx.rdregioni.v1i16.v16i16.i16(<16 x i16> %rdregioni, i32 0, i32 0, i32 0, i16 0, i32 1)
  %iselect.split15.multiindirect_idx_subregion.categoryconv = call <1 x i16> @llvm.genx.convert.addr.v1i16(<1 x i16> %iselect.split15.multiindirect_idx_subregion, i16 0)
  %iselect.split15 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v16i8.v1i16(<16 x i8> %constant.split.int81, i32 0, i32 1, i32 0, <1 x i16> %iselect.split15.multiindirect_idx_subregion.categoryconv, i32 0)
  ret void
}

declare <16 x i16> @llvm.genx.rdregioni.v16i16.v1i16.i16(<1 x i16>, i32, i32, i32, i16, i32)

declare <1 x i16> @llvm.genx.rdregioni.v1i16.v16i16.i16(<16 x i16>, i32, i32, i32, i16, i32)

declare <1 x i8> @llvm.genx.rdregioni.v1i8.v16i8.v1i16(<16 x i8>, i32, i32, i32, <1 x i16>, i32)

declare <16 x i8> @llvm.genx.wrregioni.v16i8.v1i8.i16.i1(<16 x i8>, <1 x i8>, i32, i32, i32, i16, i32, i1)

declare <16 x i8> @llvm.genx.wrregioni.v16i8.v8i8.i16.i1(<16 x i8>, <8 x i8>, i32, i32, i32, i16, i32, i1)

declare <1 x i16> @llvm.genx.convert.addr.v1i16(<1 x i16>, i16)

; uselistorder directives
uselistorder ptr @llvm.genx.rdregioni.v1i16.v16i16.i16, { 5, 4, 3, 2, 1, 0 }
uselistorder ptr @llvm.genx.rdregioni.v1i8.v16i8.v1i16, { 6, 5, 4, 3, 2, 1, 0 }
uselistorder ptr @llvm.genx.wrregioni.v16i8.v1i8.i16.i1, { 7, 6, 5, 4, 3, 2, 1, 0 }
uselistorder ptr @llvm.genx.convert.addr.v1i16, { 6, 5, 4, 3, 2, 1, 0 }

attributes #0 = { "CMGenxMain" }
