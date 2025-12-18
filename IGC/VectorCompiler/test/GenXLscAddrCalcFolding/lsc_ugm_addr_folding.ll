;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled  -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled  -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32)
declare <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1>, i8, i8, i8, <2 x i8>, i64, <32 x i64>, i16, i32, <32 x i64>)

; CHECK-LABEL: test2
define <32 x i64> @test2(i8 addrspace(1)* %base, <32 x i64> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %addr = add <32 x i64> %broadcast, %index
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> {{<(i1 true(, )?){32}>}}, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i64> %index, i16 1, i32 0, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; CHECK-LABEL: test3
define <32 x i64> @test3(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %ext = zext <32 x i32> %index to <32 x i64>
  %addr = add <32 x i64> %broadcast, %ext
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{<(i1 true(, )?){32}>}}, i8 4, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 1, i32 0, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; CHECK-LABEL: test4
define <32 x i64> @test4(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %ext = sext <32 x i32> %index to <32 x i64>
  %addr = add <32 x i64> %broadcast, %ext
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{<(i1 true(, )?){32}>}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 1, i32 0, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; CHECK-LABEL: test5
define <32 x i64> @test5(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %scale = mul <32 x i32> %index, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %ext = sext <32 x i32> %scale to <32 x i64>
  %addr = add <32 x i64> %broadcast, %ext
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{<(i1 true(, )?){32}>}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 8, i32 0, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; CHECK-LABEL: test6
define <32 x i64> @test6(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %scale = shl <32 x i32> %index, <i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3>
  %offset = add <32 x i32> <i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256, i32 256>, %scale
  %ext = sext <32 x i32> %offset to <32 x i64>
  %addr = add <32 x i64> %broadcast, %ext
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{<(i1 true(, )?){32}>}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 8, i32 256, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; CHECK-LABEL: test7
define <32 x i64> @test7(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %ext = sext <32 x i32> %index to <32 x i64>
  %scale = shl <32 x i64> %ext, <i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3>
  %offset = add <32 x i64> %scale, <i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256>
  %addr = add <32 x i64> %broadcast, %offset
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{<(i1 true(, )?){32}>}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 8, i32 256, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; CHECK-LABEL: test8
define <32 x i64> @test8(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %ext = sext <32 x i32> %index to <32 x i64>
  %scale = shl <32 x i64> %ext, <i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3>
  %offset = sub <32 x i64> %scale, <i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256>
  %addr = add <32 x i64> %broadcast, %offset
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{<(i1 true(, )?){32}>}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 8, i32 -256, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

declare i32 @llvm.genx.group.id.x()
declare i32 @llvm.genx.rdregioni.i32.v3i32.i16(<3 x i32>, i32, i32, i32, i16, i32)
declare i16 @llvm.genx.rdregioni.i16.v3i16.i16(<3 x i16>, i32, i32, i32, i16, i32)
declare <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <32 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v32i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <32 x i32>)

; CHECK-LABEL: vadd
define void @vadd(i8 addrspace(1)* %0, i8 addrspace(1)* %1, i8 addrspace(1)* %2, <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i32> %impl.arg.llvm.genx.local.size) {
  %ibase0 = ptrtoint i8 addrspace(1)* %0 to i64
  %ibase1 = ptrtoint i8 addrspace(1)* %1 to i64
  %ibase2 = ptrtoint i8 addrspace(1)* %2 to i64
  %gid = tail call i32 @llvm.genx.group.id.x()
  %lsize = call i32 @llvm.genx.rdregioni.i32.v3i32.i16(<3 x i32> %impl.arg.llvm.genx.local.size, i32 0, i32 1, i32 1, i16 0, i32 0)
  %glid = mul i32 %gid, %lsize
  %lid = call i16 @llvm.genx.rdregioni.i16.v3i16.i16(<3 x i16> %impl.arg.llvm.genx.local.id16, i32 0, i32 1, i32 1, i16 0, i32 0)
  %lidext = zext i16 %lid to i32
  %index = add i32 %glid, %lidext
  %scale = shl i32 %index, 7
  %ext = zext i32 %scale to i64
  %addr0 = add i64 %ext, %ibase0
  ; CHECK: %a = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 4, i8 3, i8 7, <2 x i8> zeroinitializer, i64 %ibase0, i32 %scale, i16 1, i32 0, <32 x i32> undef)
  %a = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i64 0, i64 %addr0, i16 1, i32 0, <32 x i32> undef)
  %addr1 = add i64 %ext, %ibase1
  ; CHECK: %b = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 4, i8 3, i8 7, <2 x i8> zeroinitializer, i64 %ibase1, i32 %scale, i16 1, i32 0, <32 x i32> undef)
  %b = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i64 0, i64 %addr1, i16 1, i32 0, <32 x i32> undef)
  %c = add <32 x i32> %a, %b
  %addr2 = add i64 %ext, %ibase2
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i32.v32i32(<1 x i1> <i1 true>, i8 4, i8 3, i8 7, <2 x i8> zeroinitializer, i64 %ibase2, i32 %scale, i16 1, i32 0, <32 x i32> %c)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v32i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i64 0, i64 %addr2, i16 1, i32 0, <32 x i32> %c)
  ret void
}

declare <16 x i64> @llvm.genx.rdregioni.v16i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32)
declare <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x i64>)

; CHECK-LABEL: fold_base_offset
define <16 x i64> @fold_base_offset(i8 addrspace(1)* %ptr, <16 x i64> %index) {
  %base = ptrtoint i8 addrspace(1)* %ptr to i64
  %baseoff = add i64 %base, 128
  %vbaseoff = bitcast i64 %baseoff to <1 x i64>
  %broadcast = call <16 x i64> @llvm.genx.rdregioni.v16i64.v1i64.i16(<1 x i64> %vbaseoff, i32 0, i32 16, i32 0, i16 0, i32 undef)
  %addr = add <16 x i64> %broadcast, %index
; CHECK: %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %index, i16 1, i32 128, <16 x i64> undef)
  %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> undef)
  ret <16 x i64> %data
}

; CHECK-LABEL: fold_base_offset_shuffle
define <16 x i64> @fold_base_offset_shuffle(i8 addrspace(1)* %ptr, <16 x i64> %index) {
  %base = ptrtoint i8 addrspace(1)* %ptr to i64
  %baseoff = add i64 %base, 128
  %vbaseoff = insertelement <16 x i64> undef, i64 %baseoff, i32 0
  %broadcast = shufflevector <16 x i64> %vbaseoff, <16 x i64> undef, <16 x i32> zeroinitializer
  %addr = add <16 x i64> %broadcast, %index
; CHECK: %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %index, i16 1, i32 128, <16 x i64> undef)
  %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> undef)
  ret <16 x i64> %data
}

; CHECK-LABEL: fold_const_index
define <16 x i64> @fold_const_index(i8 addrspace(1)* %ptr) {
  %base = ptrtoint i8 addrspace(1)* %ptr to i64
  %vbase = bitcast i64 %base to <1 x i64>
  %broadcast = call <16 x i64> @llvm.genx.rdregioni.v16i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 16, i32 0, i16 0, i32 undef)
  %addr = add <16 x i64> %broadcast, <i64 8, i64 16, i64 24, i64 32, i64 40, i64 48, i64 56, i64 64, i64 72, i64 80, i64 88, i64 96, i64 104, i64 112, i64 120, i64 128>
; CHECK: %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, i16 8, i32 8, <16 x i64> undef)
  %data = tail call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> undef)
  ret <16 x i64> %data
}
