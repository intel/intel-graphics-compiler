;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -enable-debugify -GenXLegacyToLscTranslator -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -mattr=+translate_legacy_message -S < %s 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <16 x i8> @llvm.genx.oword.ld.v16i8(i32, i32, i32)
declare <16 x i8> @llvm.genx.oword.ld.unaligned.v16i8(i32, i32, i32)
declare void @llvm.genx.oword.st.v16i8(i32, i32, <16 x i8>)

declare <16 x i8> @llvm.genx.svm.block.ld.v16i8.i64(i64)
declare <16 x i8> @llvm.genx.svm.block.ld.unaligned.v16i8.i64(i64)
declare void @llvm.genx.svm.block.st.i64.v16i8(i64, <16 x i8>)

define void @test(i32 %addr, i64 %laddr) {
  ; CHECK: [[ADDR1:%[^ ]+]] = shl i32 %addr, 4
  ; CHECK: [[LD1:%[^ ]+]] = call <4 x i32> @llvm.vc.internal.lsc.load.bti.v4i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 1, i32 [[ADDR1]], i16 1, i32 0, <4 x i32> undef)
  ; CHECK: %ld.bti = bitcast <4 x i32> [[LD1]] to <16 x i8>
  %ld.bti = call <16 x i8> @llvm.genx.oword.ld.v16i8(i32 0, i32 1, i32 %addr)

  ; CHECK: [[LD2:%[^ ]+]] = call <4 x i32> @llvm.vc.internal.lsc.load.bti.v4i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 2, i32 %addr, i16 1, i32 0, <4 x i32> undef)
  ; CHECK: %lu.bti = bitcast <4 x i32> [[LD2]] to <16 x i8>
  %lu.bti = call <16 x i8> @llvm.genx.oword.ld.unaligned.v16i8(i32 0, i32 2, i32 %addr)

  ; CHECK: [[ADDR3:%[^ ]+]] = shl i32 %addr, 4
  ; CHECK: [[CAST3:%[^ ]+]] = bitcast <16 x i8> %ld.bti to <4 x i32>
  ; CHECK: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v4i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 3, i32 [[ADDR3]], i16 1, i32 0, <4 x i32> [[CAST3]])
  call void @llvm.genx.oword.st.v16i8(i32 3, i32 %addr, <16 x i8> %ld.bti)

  ; CHECK: [[ADDR4:%[^ ]+]] = shl i32 %addr, 4
  ; CHECK: [[LD4:%[^ ]+]] = call <4 x i32> @llvm.vc.internal.lsc.load.slm.v4i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 0, i32 [[ADDR4]], i16 1, i32 0, <4 x i32> undef)
  ; CHECK: %ld.slm = bitcast <4 x i32> [[LD4]] to <16 x i8>
  %ld.slm = call <16 x i8> @llvm.genx.oword.ld.v16i8(i32 0, i32 254, i32 %addr)

  ; CHECK: [[LD5:%[^ ]+]] = call <4 x i32> @llvm.vc.internal.lsc.load.slm.v4i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 0, i32 %addr, i16 1, i32 0, <4 x i32> undef)
  ; CHECK: %lu.slm = bitcast <4 x i32> [[LD5]] to <16 x i8>
  %lu.slm = call <16 x i8> @llvm.genx.oword.ld.unaligned.v16i8(i32 0, i32 254, i32 %addr)

  ; CHECK: [[ADDR6:%[^ ]+]] = shl i32 %addr, 4
  ; CHECK: [[CAST6:%[^ ]+]] = bitcast <16 x i8> %ld.bti to <4 x i32>
  ; CHECK: call void @llvm.vc.internal.lsc.store.slm.v1i1.v2i8.i32.v4i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 0, i32 [[ADDR6]], i16 1, i32 0, <4 x i32> [[CAST6]])
  call void @llvm.genx.oword.st.v16i8(i32 254, i32 %addr, <16 x i8> %ld.bti)

  ; CHECK: [[LD7:%[^ ]+]] = call <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %laddr, i16 1, i32 0, <4 x i32> undef)
  ; CHECK: %ld.ugm = bitcast <4 x i32> [[LD7]] to <16 x i8>
  %ld.ugm = call <16 x i8> @llvm.genx.svm.block.ld.v16i8.i64(i64 %laddr)

  ; CHECK: [[LD8:%[^ ]+]] = call <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %laddr, i16 1, i32 0, <4 x i32> undef)
  ; CHECK: %lu.ugm = bitcast <4 x i32> [[LD8]] to <16 x i8>
  %lu.ugm = call <16 x i8> @llvm.genx.svm.block.ld.unaligned.v16i8.i64(i64 %laddr)

  ; CHECK: [[CAST9:%[^ ]+]] = bitcast <16 x i8> %ld.ugm to <4 x i32>
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %laddr, i16 1, i32 0, <4 x i32> [[CAST9]])
  call void @llvm.genx.svm.block.st.i64.v16i8(i64 %laddr, <16 x i8> %ld.ugm)

  ret void
}
