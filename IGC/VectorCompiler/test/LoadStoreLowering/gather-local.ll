;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-LSC,CHECK-LSC-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-LSC,CHECK-LSC-OPAQUE-PTRS
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; CHECK-LSC-NOT: WARNING
; CHECK-LSC: CheckModuleDebugify: PASS

; COM: Basic test on load lowering pass
; COM: @llvm.masked.gather from addrspace(3)

target datalayout = "e-p:64:64-p3:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 3 (local) operations are lowered into bti(254)/slm intrinsics

declare <8 x i8> @llvm.masked.gather.v8i8.v8p3i8(<8 x i8 addrspace(3)*>, i32, <8 x i1>, <8 x i8>)
declare <8 x i16> @llvm.masked.gather.v8i16.v8p3i16(<8 x i16 addrspace(3)*>, i32, <8 x i1>, <8 x i16>)
declare <8 x i32> @llvm.masked.gather.v8i32.v8p3i32(<8 x i32 addrspace(3)*>, i32, <8 x i1>, <8 x i32>)
declare <8 x i64> @llvm.masked.gather.v8i64.v8p3i64(<8 x i64 addrspace(3)*>, i32, <8 x i1>, <8 x i64>)
declare <8 x half> @llvm.masked.gather.v8f16.v8p3f16(<8 x half addrspace(3)*>, i32, <8 x i1>, <8 x half>)
declare <8 x float> @llvm.masked.gather.v8f32.v8p3f32(<8 x float addrspace(3)*>, i32, <8 x i1>, <8 x float>)
declare <8 x double> @llvm.masked.gather.v8f64.v8p3f64(<8 x double addrspace(3)*>, i32, <8 x i1>, <8 x double>)
declare <8 x i8*> @llvm.masked.gather.v8p0i8.v8p3p0i8(<8 x i8* addrspace(3)*>, i32, <8 x i1>, <8 x i8*>)

define <8 x i8> @test_i8(<8 x i8 addrspace(3)*> %pi8, <8 x i1> %mask, <8 x i8> %passthru) {
; CHECK: [[ZEXT8:[^ ]+]] = zext <8 x i8> %passthru to <8 x i32>
; CHECK-TYPED-PTRS: [[ADDR8:[^ ]+]] = ptrtoint <8 x i8 addrspace(3)*> %pi8 to <8 x i32>
; CHECK-OPAQUE-PTRS: [[ADDR8:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi8 to <8 x i32>
; CHECK: [[LOAD8:[^ ]+]] = call <8 x i32> @llvm.genx.gather.scaled.v8i32.v8i1.v8i32(<8 x i1> %mask, i32 0, i16 0, i32 254, i32 0, <8 x i32> [[ADDR8]], <8 x i32> [[ZEXT8]])
; CHECK: %res = trunc <8 x i32> [[LOAD8]] to <8 x i8>
; CHECK-LSC-DAG: [[PASSTHRU8:[^ ]+]] = zext <8 x i8> %passthru to <8 x i32>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x i8 addrspace(3)*> %pi8 to <8 x i32>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi8 to <8 x i32>
; CHECK-LSC: [[DATA8:[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.slm.v8i32.v8i1.v2i8.v8i32(<8 x i1> %mask, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 0, <8 x i32> [[ADDR8]], i16 1, i32 0, <8 x i32> [[PASSTHRU8]])
; CHECK-LSC: %res = trunc <8 x i32> [[DATA8]] to <8 x i8>
  %res = call <8 x i8> @llvm.masked.gather.v8i8.v8p3i8(<8 x i8 addrspace(3)*> %pi8, i32 1, <8 x i1> %mask, <8 x i8> %passthru)
  ret <8 x i8> %res
}

define <8 x i16> @test_i16(<8 x i16 addrspace(3)*> %pi16, <8 x i1> %mask, <8 x i16> %passthru) {
; CHECK: [[ZEXT16:[^ ]+]] = zext <8 x i16> %passthru to <8 x i32>
; CHECK-TYPED-PTRS: [[ADDR16:[^ ]+]] = ptrtoint <8 x i16 addrspace(3)*> %pi16 to <8 x i32>
; CHECK-OPAQUE-PTRS: [[ADDR16:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi16 to <8 x i32>
; CHECK: [[LOAD16:[^ ]+]] = call <8 x i32> @llvm.genx.gather.scaled.v8i32.v8i1.v8i32(<8 x i1> %mask, i32 1, i16 0, i32 254, i32 0, <8 x i32> [[ADDR16]], <8 x i32> [[ZEXT16]])
; CHECK: %res = trunc <8 x i32> [[LOAD16]] to <8 x i16>
; CHECK-LSC-DAG: [[PASSTHRU16:[^ ]+]] = zext <8 x i16> %passthru to <8 x i32>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x i16 addrspace(3)*> %pi16 to <8 x i32>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi16 to <8 x i32>
; CHECK-LSC: [[DATA16:[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.slm.v8i32.v8i1.v2i8.v8i32(<8 x i1> %mask, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 0, <8 x i32> [[ADDR16]], i16 1, i32 0, <8 x i32> [[PASSTHRU16]])
; CHECK-LSC: %res = trunc <8 x i32> [[DATA16]] to <8 x i16>
  %res = call <8 x i16> @llvm.masked.gather.v8i16.v8p3i16(<8 x i16 addrspace(3)*> %pi16, i32 2, <8 x i1> %mask, <8 x i16> %passthru)
  ret <8 x i16> %res
}

define <8 x i32> @test_i32(<8 x i32 addrspace(3)*> %pi32, <8 x i1> %mask, <8 x i32> %passthru) {
; CHECK-TYPED-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x i32 addrspace(3)*> %pi32 to <8 x i32>
; CHECK-OPAQUE-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi32 to <8 x i32>
; CHECK: %res = call <8 x i32> @llvm.genx.gather.scaled.v8i32.v8i1.v8i32(<8 x i1> %mask, i32 2, i16 0, i32 254, i32 0, <8 x i32> [[ADDR32]], <8 x i32> %passthru)
; CHECK-LSC-TYPED-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x i32 addrspace(3)*> %pi32 to <8 x i32>
; CHECK-LSC-OPAQUE-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi32 to <8 x i32>
; CHECK-LSC: %res = call <8 x i32> @llvm.vc.internal.lsc.load.slm.v8i32.v8i1.v2i8.v8i32(<8 x i1> %mask, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 0, <8 x i32> [[ADDR32]], i16 1, i32 0, <8 x i32> %passthru)
  %res = call <8 x i32> @llvm.masked.gather.v8i32.v8p3i32(<8 x i32 addrspace(3)*> %pi32, i32 4, <8 x i1> %mask, <8 x i32> %passthru)
  ret <8 x i32> %res
}

define <8 x i64> @test_i64(<8 x i64 addrspace(3)*> %pi64, <8 x i1> %mask, <8 x i64> %passthru) {
; CHECK-TYPED-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x i64 addrspace(3)*> %pi64 to <8 x i32>
; CHECK-OPAQUE-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi64 to <8 x i32>
; CHECK: [[CAST64:[^ ]+]] = bitcast <8 x i64> %passthru to <16 x i32>
; CHECK: [[STOA64:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> [[CAST64]], i32 1, i32 8, i32 2, i16 0, i32 undef)
; CHECK: [[LOAD64:[^ ]+]] = call <16 x i32> @llvm.genx.gather4.scaled.v16i32.v8i1.v8i32(<8 x i1> %mask, i32 12, i16 0, i32 254, i32 0, <8 x i32> [[ADDR64]], <16 x i32> [[STOA64]])
; CHECK: [[ATOS64:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> [[LOAD64]], i32 1, i32 2, i32 8, i16 0, i32 undef)
; CHECK: %res = bitcast <16 x i32> [[ATOS64]] to <8 x i64>
; CHECK-LSC-TYPED-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x i64 addrspace(3)*> %pi64 to <8 x i32>
; CHECK-LSC-OPAQUE-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi64 to <8 x i32>
; CHECK-LSC: %res = call <8 x i64> @llvm.vc.internal.lsc.load.slm.v8i64.v8i1.v2i8.v8i32(<8 x i1> %mask, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <8 x i32> [[ADDR32]], i16 1, i32 0, <8 x i64> %passthru)
  %res = call <8 x i64> @llvm.masked.gather.v8i64.v8p3i64(<8 x i64 addrspace(3)*> %pi64, i32 8, <8 x i1> %mask, <8 x i64> %passthru)
  ret <8 x i64> %res
}

define <8 x half> @test_f16(<8 x half addrspace(3)*> %pi16, <8 x i1> %mask, <8 x half> %passthru) {
; CHECK: [[CASTH:[^ ]+]] = bitcast <8 x half> %passthru to <8 x i16>
; CHECK: [[ZEXTH:[^ ]+]] = zext <8 x i16> [[CASTH]] to <8 x i32>
; CHECK-TYPED-PTRS: [[ADDRH:[^ ]+]] = ptrtoint <8 x half addrspace(3)*> %pi16 to <8 x i32>
; CHECK-OPAQUE-PTRS: [[ADDRH:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi16 to <8 x i32>
; CHECK: [[LOADH:[^ ]+]] = call <8 x i32> @llvm.genx.gather.scaled.v8i32.v8i1.v8i32(<8 x i1> %mask, i32 1, i16 0, i32 254, i32 0, <8 x i32> [[ADDRH]], <8 x i32> [[ZEXTH]])
; CHECK: %5 = trunc <8 x i32> %4 to <8 x i16>
; CHECK: %res = bitcast <8 x i16> %5 to <8 x half>
; CHECK-LSC-DAG: [[CASTH:[^ ]+]] = bitcast <8 x half> %passthru to <8 x i16>
; CHECK-LSC-DAG: [[PASSTHRUH:[^ ]+]] = zext <8 x i16> [[CASTH]] to <8 x i32>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x half addrspace(3)*> %pi16 to <8 x i32>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi16 to <8 x i32>
; CHECK-LSC: [[DATAH:[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.slm.v8i32.v8i1.v2i8.v8i32(<8 x i1> %mask, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 0, <8 x i32> [[ADDRH]], i16 1, i32 0, <8 x i32> [[PASSTHRUH]])
; CHECK-LSC: [[TRUNC:[^ ]+]] = trunc <8 x i32> [[DATAH]] to <8 x i16>
; CHECK-LSC: %res = bitcast <8 x i16> [[TRUNC]] to <8 x half>
  %res = call <8 x half> @llvm.masked.gather.v8f16.v8p3f16(<8 x half addrspace(3)*> %pi16, i32 2, <8 x i1> %mask, <8 x half> %passthru)
  ret <8 x half> %res
}

define <8 x float> @test_f32(<8 x float addrspace(3)*> %pi32, <8 x i1> %mask, <8 x float> %passthru) {
; CHECK-TYPED-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x float addrspace(3)*> %pi32 to <8 x i32>
; CHECK-OPAQUE-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi32 to <8 x i32>
; CHECK: %res = call <8 x float> @llvm.genx.gather.scaled.v8f32.v8i1.v8i32(<8 x i1> %mask, i32 2, i16 0, i32 254, i32 0, <8 x i32> [[ADDRF]], <8 x float> %passthru)
; CHECK-LSC-TYPED-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x float addrspace(3)*> %pi32 to <8 x i32>
; CHECK-LSC-OPAQUE-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi32 to <8 x i32>
; CHECK-LSC: %res = call <8 x float> @llvm.vc.internal.lsc.load.slm.v8f32.v8i1.v2i8.v8i32(<8 x i1> %mask, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 0, <8 x i32> [[ADDRF]], i16 1, i32 0, <8 x float> %passthru)
  %res = call <8 x float> @llvm.masked.gather.v8f32.v8p3f32(<8 x float addrspace(3)*> %pi32, i32 4, <8 x i1> %mask, <8 x float> %passthru)
  ret <8 x float> %res
}

define <8 x double> @test_f64(<8 x double addrspace(3)*> %pi64, <8 x i1> %mask, <8 x double> %passthru) {
; CHECK-TYPED-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x double addrspace(3)*> %pi64 to <8 x i32>
; CHECK-OPAQUE-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi64 to <8 x i32>
; CHECK: [[CASTD:[^ ]+]] = bitcast <8 x double> %passthru to <16 x i32>
; CHECK: [[STOAD:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> [[CASTD]], i32 1, i32 8, i32 2, i16 0, i32 undef)
; CHECK: [[LOADD:[^ ]+]] = call <16 x i32> @llvm.genx.gather4.scaled.v16i32.v8i1.v8i32(<8 x i1> %mask, i32 12, i16 0, i32 254, i32 0, <8 x i32> [[ADDRD]], <16 x i32> [[STOAD]])
; CHECK: [[ATOSD:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> [[LOADD]], i32 1, i32 2, i32 8, i16 0, i32 undef)
; CHECK: %res = bitcast <16 x i32> [[ATOSD]] to <8 x double>
; CHECK-LSC-TYPED-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x double addrspace(3)*> %pi64 to <8 x i32>
; CHECK-LSC-OPAQUE-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi64 to <8 x i32>
; CHECK-LSC: %res = call <8 x double> @llvm.vc.internal.lsc.load.slm.v8f64.v8i1.v2i8.v8i32(<8 x i1> %mask, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <8 x i32> [[ADDRD]], i16 1, i32 0, <8 x double> %passthru)
  %res = call <8 x double> @llvm.masked.gather.v8f64.v8p3f64(<8 x double addrspace(3)*> %pi64, i32 8, <8 x i1> %mask, <8 x double> %passthru)
  ret <8 x double> %res
}

define <8 x i8*> @test_ptr(<8 x i8* addrspace(3)*> %pptr, <8 x i1> %mask, <8 x i8*> %passthru) {
; CHECK-TYPED-PTRS: [[PTI:[^ ]+]] = ptrtoint <8 x i8*> %passthru to <8 x i64>
; CHECK-TYPED-PTRS: [[ADDRP:[^ ]+]] = ptrtoint <8 x i8* addrspace(3)*> %pptr to <8 x i32>
; CHECK-OPAQUE-PTRS: [[PTI:[^ ]+]] = ptrtoint <8 x ptr> %passthru to <8 x i64>
; CHECK-OPAQUE-PTRS: [[ADDRP:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pptr to <8 x i32>
; CHECK: [[CASTP:[^ ]+]] = bitcast <8 x i64> [[PTI]] to <16 x i32>
; CHECK: [[STOAP:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> [[CASTP]], i32 1, i32 8, i32 2, i16 0, i32 undef)
; CHECK: [[LOADP:[^ ]+]] = call <16 x i32> @llvm.genx.gather4.scaled.v16i32.v8i1.v8i32(<8 x i1> %mask, i32 12, i16 0, i32 254, i32 0, <8 x i32> [[ADDRP]], <16 x i32> [[STOAP]])
; CHECK: [[ATOSP:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> [[LOADP]], i32 1, i32 2, i32 8, i16 0, i32 undef)
; CHECK: [[ITP:[^ ]+]] = bitcast <16 x i32> [[ATOSP]] to <8 x i64>
; CHECK-TYPED-PTRS: %res = inttoptr <8 x i64> [[ITP]] to <8 x i8*>
; CHECK-OPAQUE-PTRS: %res = inttoptr <8 x i64> [[ITP]] to <8 x ptr>
; CHECK-LSC-TYPED-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x i8*> %passthru to <8 x i64>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x i8* addrspace(3)*> %pptr to <8 x i32>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x ptr> %passthru to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pptr to <8 x i32>
; CHECK-LSC: [[DATAP:[^ ]+]] = call <8 x i64> @llvm.vc.internal.lsc.load.slm.v8i64.v8i1.v2i8.v8i32(<8 x i1> %mask, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <8 x i32> [[ADDRP]], i16 1, i32 0, <8 x i64> [[PTI]])
; CHECK-LSC-TYPED-PTRS: %res = inttoptr <8 x i64> [[DATAP]] to <8 x i8*>
; CHECK-LSC-OPAQUE-PTRS: %res = inttoptr <8 x i64> [[DATAP]] to <8 x ptr>
  %res = call <8 x i8*> @llvm.masked.gather.v8p0i8.v8p3p0i8(<8 x i8* addrspace(3)*> %pptr, i32 8, <8 x i1> %mask, <8 x i8*> %passthru)
  ret <8 x i8*> %res
}

define <8 x i64> @test_i64_unaligned(<8 x i64 addrspace(3)*> %pi64, <8 x i1> %mask, <8 x i64> %passthru) {
; CHECK-TYPED-PTRS: [[ADDRU64:[^ ]+]] = ptrtoint <8 x i64 addrspace(3)*> %pi64 to <8 x i32>
; CHECK-OPAQUE-PTRS: [[ADDRU64:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi64 to <8 x i32>
; CHECK: [[CASTU64:[^ ]+]] = bitcast <8 x i64> %passthru to <16 x i32>
; CHECK: [[LOWU64:[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[CASTU64]], i32 2, i32 1, i32 0, i16 0, i32 undef)
; CHECK: [[HIGHU64:[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[CASTU64]], i32 2, i32 1, i32 0, i16 4, i32 undef)
; CHECK: [[LOADL64:[^ ]+]] = call <8 x i32> @llvm.genx.gather.scaled.v8i32.v8i1.v8i32(<8 x i1> %mask, i32 2, i16 0, i32 254, i32 0, <8 x i32> [[ADDRU64]], <8 x i32> [[LOWU64]])
; CHECK: [[LOADH64:[^ ]+]] = call <8 x i32> @llvm.genx.gather.scaled.v8i32.v8i1.v8i32(<8 x i1> %mask, i32 2, i16 0, i32 254, i32 4, <8 x i32> [[ADDRU64]], <8 x i32> [[HIGHU64]])
; CHECK: [[INSL64:[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[LOADL64]], i32 2, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: [[INSH64:[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> %7, <8 x i32> [[LOADH64]], i32 2, i32 1, i32 0, i16 4, i32 undef, i1 true)
; CHECK: %res = bitcast <16 x i32> [[INSH64]] to <8 x i64>
; CHECK-LSC-TYPED-PTRS: [[ADDRU64:[^ ]+]] = ptrtoint <8 x i64 addrspace(3)*> %pi64 to <8 x i32>
; CHECK-LSC-OPAQUE-PTRS: [[ADDRU64:[^ ]+]] = ptrtoint <8 x ptr addrspace(3)> %pi64 to <8 x i32>
; CHECK-LSC: %res = call <8 x i64> @llvm.vc.internal.lsc.load.slm.v8i64.v8i1.v2i8.v8i32(<8 x i1> %mask, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <8 x i32> [[ADDRU64]], i16 1, i32 0, <8 x i64> %passthru)
  %res = call <8 x i64> @llvm.masked.gather.v8i64.v8p3i64(<8 x i64 addrspace(3)*> %pi64, i32 1, <8 x i1> %mask, <8 x i64> %passthru)
  ret <8 x i64> %res
}
