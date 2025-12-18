;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled  -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled  -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1>, i8, i8, i8, <2 x i8>, i32, <4 x i32>, i16, i32, <4 x i64>)

; CHECK-LABEL: test1
define <4 x i64> @test1(<4 x i32> %arg) {
; CHECK: %data = tail call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <4 x i32> %arg, i16 8, i32 16, <4 x i64> undef)
  %1 = shl <4 x i32> %arg, <i32 3, i32 3, i32 3, i32 3>
  %2 = add <4 x i32> %1, <i32 16, i32 16, i32 16, i32 16>
  %data = tail call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <4 x i32> %2, i16 1, i32 0, <4 x i64> undef)
  ret <4 x i64> %data
}

; CHECK-LABEL: test_unsupported_scale
define <4 x i64> @test_unsupported_scale(<4 x i32> %arg) {
; CHECK: [[SCALE:%[^ ]+]] = shl <4 x i32> %arg, <i32 2, i32 2, i32 2, i32 2>
; CHECK: %data = tail call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <4 x i32> [[SCALE]], i16 1, i32 16, <4 x i64> undef)
  %1 = shl <4 x i32> %arg, <i32 2, i32 2, i32 2, i32 2>
  %2 = add <4 x i32> %1, <i32 16, i32 16, i32 16, i32 16>
  %data = tail call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <4 x i32> %2, i16 1, i32 0, <4 x i64> undef)
  ret <4 x i64> %data
}

; CHECK-LABEL: test_unsupported_offset
define <4 x i64> @test_unsupported_offset(<4 x i32> %arg) {
; CHECK: [[SCALE:%[^ ]+]] = shl <4 x i32> %arg, <i32 3, i32 3, i32 3, i32 3>
; CHECK: [[OFFSET:%[^ ]+]] = add <4 x i32> [[SCALE]], <i32 12, i32 12, i32 12, i32 12>
; CHECK: %data = tail call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <4 x i32> [[OFFSET]], i16 1, i32 0, <4 x i64> undef)
  %1 = shl <4 x i32> %arg, <i32 3, i32 3, i32 3, i32 3>
  %2 = add <4 x i32> %1, <i32 12, i32 12, i32 12, i32 12>
  %data = tail call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <4 x i32> %2, i16 1, i32 0, <4 x i64> undef)
  ret <4 x i64> %data
}

declare <32 x i32> @llvm.genx.rdregioni.v32i32.v1i32.i16(<1 x i32>, i32, i32, i32, i16, i32)
declare <32 x i64> @llvm.vc.internal.lsc.load.slm.v32i64.v32i1.v2i8.v32i32(<32 x i1>, i8, i8, i8, <2 x i8>, i32, <32 x i32>, i16, i32, <32 x i64>)

; CHECK-LABEL: test2
define <32 x i64> @test2(i8 addrspace(3)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(3)* %base to i32
  %vbase = bitcast i32 %ibase to <1 x i32>
  %broadcast = call <32 x i32> @llvm.genx.rdregioni.v32i32.v1i32.i16(<1 x i32> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %addr = add <32 x i32> %broadcast, %index
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.slm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{<(i1 true(, )?){32}>}}, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 %ibase, <32 x i32> %index, i16 1, i32 0, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.slm.v32i64.v32i1.v2i8.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <32 x i32> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; CHECK-LABEL: test6
define <32 x i64> @test6(i8 addrspace(3)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(3)* %base to i32
  %vbase = bitcast i32 %ibase to <1 x i32>
  %broadcast = call <32 x i32> @llvm.genx.rdregioni.v32i32.v1i32.i16(<1 x i32> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %scale = shl <32 x i32> %index, <i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3>
  %offset = add <32 x i32> <i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256>, %scale
  %addr = add <32 x i32> %broadcast, %offset
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.slm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{<(i1 true(, )?){32}>}}, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 %ibase, <32 x i32> %index, i16 8, i32 256, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.slm.v32i64.v32i1.v2i8.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <32 x i32> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

declare i32 @llvm.genx.group.id.x()
declare i32 @llvm.genx.rdregioni.i32.v3i32.i16(<3 x i32>, i32, i32, i32, i16, i32)
declare i16 @llvm.genx.rdregioni.i16.v3i16.i16(<3 x i16>, i32, i32, i32, i16, i32)
declare <32 x i32> @llvm.vc.internal.lsc.load.slm.v32i32.v1i1.v2i8.i32(<1 x i1>, i8, i8, i8, <2 x i8>, i32, i32, i16, i32, <32 x i32>)
declare void @llvm.vc.internal.lsc.store.slm.v1i1.v2i8.i32.v32i32(<1 x i1>, i8, i8, i8, <2 x i8>, i32, i32, i16, i32, <32 x i32>)

; CHECK-LABEL: vadd
define void @vadd(i8 addrspace(3)* %0, i8 addrspace(3)* %1, i8 addrspace(3)* %2, <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i32> %impl.arg.llvm.genx.local.size) {
  %ibase0 = ptrtoint i8 addrspace(3)* %0 to i32
  %ibase1 = ptrtoint i8 addrspace(3)* %1 to i32
  %ibase2 = ptrtoint i8 addrspace(3)* %2 to i32
  %gid = tail call i32 @llvm.genx.group.id.x()
  %lsize = call i32 @llvm.genx.rdregioni.i32.v3i32.i16(<3 x i32> %impl.arg.llvm.genx.local.size, i32 0, i32 1, i32 1, i16 0, i32 0)
  %glid = mul i32 %gid, %lsize
  %lid = call i16 @llvm.genx.rdregioni.i16.v3i16.i16(<3 x i16> %impl.arg.llvm.genx.local.id16, i32 0, i32 1, i32 1, i16 0, i32 0)
  %lidext = zext i16 %lid to i32
  %index = add i32 %glid, %lidext
  %scale = shl i32 %index, 7
  %addr0 = add i32 %scale, %ibase0
  ; CHECK: %a = call <32 x i32> @llvm.vc.internal.lsc.load.slm.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i32 %ibase0, i32 %scale, i16 1, i32 0, <32 x i32> undef)
  %a = call <32 x i32> @llvm.vc.internal.lsc.load.slm.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i32 0, i32 %addr0, i16 1, i32 0, <32 x i32> undef)
  %addr1 = add i32 %scale, %ibase1
  ; CHECK: %b = call <32 x i32> @llvm.vc.internal.lsc.load.slm.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i32 %ibase1, i32 %scale, i16 1, i32 0, <32 x i32> undef)
  %b = call <32 x i32> @llvm.vc.internal.lsc.load.slm.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i32 0, i32 %addr1, i16 1, i32 0, <32 x i32> undef)
  %c = add <32 x i32> %a, %b
  %addr2 = add i32 %scale, %ibase2
  ; CHECK: call void @llvm.vc.internal.lsc.store.slm.v1i1.v2i8.i32.v32i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i32 %ibase2, i32 %scale, i16 1, i32 0, <32 x i32> %c)
  call void @llvm.vc.internal.lsc.store.slm.v1i1.v2i8.i32.v32i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i32 0, i32 %addr2, i16 1, i32 0, <32 x i32> %c)
  ret void
}

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32>, i32, i32, i32, i16, i32)
declare <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <16 x i64>)

; CHECK-LABEL: fold_base_offset
define <16 x i64> @fold_base_offset(i8 addrspace(3)* %ptr, <16 x i32> %index) {
  %base = ptrtoint i8 addrspace(3)* %ptr to i32
  %baseoff = add i32 %base, 128
  %vbaseoff = bitcast i32 %baseoff to <1 x i32>
  %broadcast = call <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32> %vbaseoff, i32 0, i32 16, i32 0, i16 0, i32 undef)
  %addr = add <16 x i32> %broadcast, %index
; CHECK: %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 %base, <16 x i32> %index, i16 1, i32 128, <16 x i64> undef)
  %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v16i1.v2i8.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i64> undef)
  ret <16 x i64> %data
}

; CHECK-LABEL: fold_base_offset_shuffle
define <16 x i64> @fold_base_offset_shuffle(i8 addrspace(3)* %ptr, <16 x i32> %index) {
  %base = ptrtoint i8 addrspace(3)* %ptr to i32
  %baseoff = add i32 %base, 128
  %vbaseoff = insertelement <16 x i32> undef, i32 %baseoff, i32 0
  %broadcast = shufflevector <16 x i32> %vbaseoff, <16 x i32> undef, <16 x i32> zeroinitializer
  %addr = add <16 x i32> %broadcast, %index
; CHECK: %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 %base, <16 x i32> %index, i16 1, i32 128, <16 x i64> undef)
  %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v16i1.v2i8.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i64> undef)
  ret <16 x i64> %data
}

; CHECK-LABEL: fold_const_index_1
define <16 x i64> @fold_const_index_1(i8 addrspace(3)* %ptr) {
  %base = ptrtoint i8 addrspace(3)* %ptr to i32
  %vbase = bitcast i32 %base to <1 x i32>
  %broadcast = call <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32> %vbase, i32 0, i32 16, i32 0, i16 0, i32 undef)
  %addr = add <16 x i32> %broadcast, <i32 8, i32 16, i32 24, i32 32, i32 40, i32 48, i32 56, i32 64, i32 72, i32 80, i32 88, i32 96, i32 104, i32 112, i32 120, i32 128>
; CHECK: %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 %base, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, i16 8, i32 8, <16 x i64> undef)
  %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v16i1.v2i8.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i64> undef)
  ret <16 x i64> %data
}

declare void @llvm.vc.internal.lsc.store.slm.v4i1.v2i8.v4i32.v32f32(<4 x i1>, i8, i8, i8, <2 x i8>, i32, <4 x i32>, i16, i32, <32 x float>)

; CHECK-LABEL: fold_const_index_2
define void @fold_const_index_2(<32 x float> %arg) {
; CHECK: tail call void @llvm.vc.internal.lsc.store.slm.v4i1.v2i8.v4i32.v32f32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 3, i8 5, <2 x i8> zeroinitializer, i32 0, <4 x i32> <i32 0, i32 128, i32 256, i32 384>, i16 1, i32 0, <32 x float> %arg)
  tail call void @llvm.vc.internal.lsc.store.slm.v4i1.v2i8.v4i32.v32f32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 3, i8 5, <2 x i8> zeroinitializer, i32 0, <4 x i32> <i32 268435456, i32 268435584, i32 268435712, i32 268435840>, i16 1, i32 0, <32 x float> %arg)
  ret void
}
