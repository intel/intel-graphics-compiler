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
; COM: @llvm.masked.gather from addrspace(1)

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 1 (global) operations are lowered into svm/ugm intrinsics

declare <8 x i8> @llvm.masked.gather.v8i8.v8p1i8(<8 x i8 addrspace(1)*>, i32, <8 x i1>, <8 x i8>)
declare <8 x i16> @llvm.masked.gather.v8i16.v8p1i16(<8 x i16 addrspace(1)*>, i32, <8 x i1>, <8 x i16>)
declare <8 x i32> @llvm.masked.gather.v8i32.v8p1i32(<8 x i32 addrspace(1)*>, i32, <8 x i1>, <8 x i32>)
declare <8 x i64> @llvm.masked.gather.v8i64.v8p1i64(<8 x i64 addrspace(1)*>, i32, <8 x i1>, <8 x i64>)
declare <8 x half> @llvm.masked.gather.v8f16.v8p1f16(<8 x half addrspace(1)*>, i32, <8 x i1>, <8 x half>)
declare <8 x float> @llvm.masked.gather.v8f32.v8p1f32(<8 x float addrspace(1)*>, i32, <8 x i1>, <8 x float>)
declare <8 x double> @llvm.masked.gather.v8f64.v8p1f64(<8 x double addrspace(1)*>, i32, <8 x i1>, <8 x double>)
declare <8 x i8*> @llvm.masked.gather.v8p0i8.v8p1p0i8(<8 x i8* addrspace(1)*>, i32, <8 x i1>, <8 x i8*>)

define <8 x i8> @test_i8(<8 x i8 addrspace(1)*> %pi8, <8 x i1> %mask, <8 x i8> %passthru) {
; CHECK-DAG: [[PASSTHRU8:[^ ]+]] = zext <8 x i8> %passthru to <8 x i32>
; CHECK-DAG: [[CAST8:[^ ]+]] = bitcast <8 x i32> [[PASSTHRU8]] to <32 x i8>
; CHECK-TYPED-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x i8 addrspace(1)*> %pi8 to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi8 to <8 x i64>
; CHECK: [[DATA8:[^ ]+]] = call <32 x i8> @llvm.genx.svm.gather.v32i8.v8i1.v8i64(<8 x i1> %mask, i32 0, <8 x i64> [[ADDR8]], <32 x i8> [[CAST8]])
; CHECK: [[LCAST8:[^ ]+]] = bitcast <32 x i8> [[DATA8]] to <8 x i32>
; CHECK: %res = trunc <8 x i32> [[LCAST8]] to <8 x i8>
; CHECK-LSC-DAG: [[PASSTHRU8:[^ ]+]] = zext <8 x i8> %passthru to <8 x i32>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x i8 addrspace(1)*> %pi8 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi8 to <8 x i64>
; CHECK-LSC: [[DATA8:[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR8]], i16 1, i32 0, <8 x i32> [[PASSTHRU8]])
; CHECK-LSC: %res = trunc <8 x i32> [[DATA8]] to <8 x i8>
  %res = call <8 x i8> @llvm.masked.gather.v8i8.v8p1i8(<8 x i8 addrspace(1)*> %pi8, i32 1, <8 x i1> %mask, <8 x i8> %passthru)
  ret <8 x i8> %res
}

define <8 x i16> @test_i16(<8 x i16 addrspace(1)*> %pi16, <8 x i1> %mask, <8 x i16> %passthru) {
; CHECK-DAG: [[PASSTHRU16:[^ ]+]] = zext <8 x i16> %passthru to <8 x i32>
; CHECK-DAG: [[CAST16:[^ ]+]] = bitcast <8 x i32> [[PASSTHRU16]] to <32 x i8>
; CHECK-TYPED-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x i16 addrspace(1)*> %pi16 to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi16 to <8 x i64>
; CHECK: [[DATA16:[^ ]+]] = call <32 x i8> @llvm.genx.svm.gather.v32i8.v8i1.v8i64(<8 x i1> %mask, i32 1, <8 x i64> [[ADDR16]], <32 x i8> [[CAST16]])
; CHECK: [[LCAST16:[^ ]+]] = bitcast <32 x i8> [[DATA16]] to <8 x i32>
; CHECK: %res = trunc <8 x i32> [[LCAST16]] to <8 x i16>
; CHECK-LSC-DAG: [[PASSTHRU16:[^ ]+]] = zext <8 x i16> %passthru to <8 x i32>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x i16 addrspace(1)*> %pi16 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi16 to <8 x i64>
; CHECK-LSC: [[DATA16:[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR16]], i16 1, i32 0, <8 x i32> [[PASSTHRU16]])
; CHECK-LSC: %res = trunc <8 x i32> [[DATA16]] to <8 x i16>
  %res = call <8 x i16> @llvm.masked.gather.v8i16.v8p1i16(<8 x i16 addrspace(1)*> %pi16, i32 2, <8 x i1> %mask, <8 x i16> %passthru)
  ret <8 x i16> %res
}

define <8 x i32> @test_i32(<8 x i32 addrspace(1)*> %pi32, <8 x i1> %mask, <8 x i32> %passthru) {
; CHECK-TYPED-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x i32 addrspace(1)*> %pi32 to <8 x i64>
; CHECK-OPAQUE-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi32 to <8 x i64>
; CHECK: %res = call <8 x i32> @llvm.genx.svm.gather.v8i32.v8i1.v8i64(<8 x i1> %mask, i32 0, <8 x i64> [[ADDR32]], <8 x i32> %passthru)
; CHECK-LSC-TYPED-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x i32 addrspace(1)*> %pi32 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi32 to <8 x i64>
; CHECK-LSC: %res = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR32]], i16 1, i32 0, <8 x i32> %passthru)
  %res = call <8 x i32> @llvm.masked.gather.v8i32.v8p1i32(<8 x i32 addrspace(1)*> %pi32, i32 4, <8 x i1> %mask, <8 x i32> %passthru)
  ret <8 x i32> %res
}

define <8 x i64> @test_i64(<8 x i64 addrspace(1)*> %pi64, <8 x i1> %mask, <8 x i64> %passthru) {
; CHECK-TYPED-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x i64 addrspace(1)*> %pi64 to <8 x i64>
; CHECK-OPAQUE-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi64 to <8 x i64>
; CHECK: %res = call <8 x i64> @llvm.genx.svm.gather.v8i64.v8i1.v8i64(<8 x i1> %mask, i32 0, <8 x i64> [[ADDR64]], <8 x i64> %passthru)
; CHECK-LSC-TYPED-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x i64 addrspace(1)*> %pi64 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi64 to <8 x i64>
; CHECK-LSC: %res = call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v8i1.v2i8.v8i64(<8 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR32]], i16 1, i32 0, <8 x i64> %passthru)
  %res = call <8 x i64> @llvm.masked.gather.v8i64.v8p1i64(<8 x i64 addrspace(1)*> %pi64, i32 8, <8 x i1> %mask, <8 x i64> %passthru)
  ret <8 x i64> %res
}

define <8 x half> @test_f16(<8 x half addrspace(1)*> %pi16, <8 x i1> %mask, <8 x half> %passthru) {
; CHECK-DAG: [[CASTH:[^ ]+]] = bitcast <8 x half> %passthru to <8 x i16>
; CHECK-DAG: [[PASSTHRUH:[^ ]+]] = zext <8 x i16> [[CASTH]] to <8 x i32>
; CHECK-DAG: [[CASTI:[^ ]+]] = bitcast <8 x i32> [[PASSTHRUH]] to <32 x i8>
; CHECK-TYPED-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x half addrspace(1)*> %pi16 to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi16 to <8 x i64>
; CHECK: [[DATAH:[^ ]+]] = call <32 x i8> @llvm.genx.svm.gather.v32i8.v8i1.v8i64(<8 x i1> %mask, i32 1, <8 x i64> [[ADDRH]], <32 x i8> [[CASTI]])
; CHECK: [[LCASTH:[^ ]+]] = bitcast <32 x i8> [[DATAH]] to <8 x i32>
; CHECK: [[TRUNC:[^ ]+]] = trunc <8 x i32> [[LCASTH]] to <8 x i16>
; CHECK: %res = bitcast <8 x i16> [[TRUNC]] to <8 x half>
; CHECK-LSC-DAG: [[CASTH:[^ ]+]] = bitcast <8 x half> %passthru to <8 x i16>
; CHECK-LSC-DAG: [[PASSTHRUH:[^ ]+]] = zext <8 x i16> [[CASTH]] to <8 x i32>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x half addrspace(1)*> %pi16 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi16 to <8 x i64>
; CHECK-LSC: [[DATAH:[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDRH]], i16 1, i32 0, <8 x i32> [[PASSTHRUH]])
; CHECK-LSC: [[TRUNC:[^ ]+]] = trunc <8 x i32> [[DATAH]] to <8 x i16>
; CHECK-LSC: %res = bitcast <8 x i16> [[TRUNC]] to <8 x half>
  %res = call <8 x half> @llvm.masked.gather.v8f16.v8p1f16(<8 x half addrspace(1)*> %pi16, i32 2, <8 x i1> %mask, <8 x half> %passthru)
  ret <8 x half> %res
}

define <8 x float> @test_f32(<8 x float addrspace(1)*> %pi32, <8 x i1> %mask, <8 x float> %passthru) {
; CHECK-TYPED-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x float addrspace(1)*> %pi32 to <8 x i64>
; CHECK-OPAQUE-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi32 to <8 x i64>
; CHECK: %res = call <8 x float> @llvm.genx.svm.gather.v8f32.v8i1.v8i64(<8 x i1> %mask, i32 0, <8 x i64> [[ADDRF]], <8 x float> %passthru)
; CHECK-LSC-TYPED-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x float addrspace(1)*> %pi32 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi32 to <8 x i64>
; CHECK-LSC: %res = call <8 x float> @llvm.vc.internal.lsc.load.ugm.v8f32.v8i1.v2i8.v8i64(<8 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDRF]], i16 1, i32 0, <8 x float> %passthru)
  %res = call <8 x float> @llvm.masked.gather.v8f32.v8p1f32(<8 x float addrspace(1)*> %pi32, i32 4, <8 x i1> %mask, <8 x float> %passthru)
  ret <8 x float> %res
}

define <8 x double> @test_f64(<8 x double addrspace(1)*> %pi64, <8 x i1> %mask, <8 x double> %passthru) {
; CHECK-TYPED-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x double addrspace(1)*> %pi64 to <8 x i64>
; CHECK-OPAQUE-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi64 to <8 x i64>
; CHECK: %res = call <8 x double> @llvm.genx.svm.gather.v8f64.v8i1.v8i64(<8 x i1> %mask, i32 0, <8 x i64> [[ADDRD]], <8 x double> %passthru)
; CHECK-LSC-TYPED-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x double addrspace(1)*> %pi64 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi64 to <8 x i64>
; CHECK-LSC: %res = call <8 x double> @llvm.vc.internal.lsc.load.ugm.v8f64.v8i1.v2i8.v8i64(<8 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDRD]], i16 1, i32 0, <8 x double> %passthru)
  %res = call <8 x double> @llvm.masked.gather.v8f64.v8p1f64(<8 x double addrspace(1)*> %pi64, i32 8, <8 x i1> %mask, <8 x double> %passthru)
  ret <8 x double> %res
}

define <8 x i8*> @test_ptr(<8 x i8* addrspace(1)*> %pptr, <8 x i1> %mask, <8 x i8*> %passthru) {
; CHECK-TYPED-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x i8*> %passthru to <8 x i64>
; CHECK-TYPED-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x i8* addrspace(1)*> %pptr to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x ptr> %passthru to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pptr to <8 x i64>
; CHECK: [[DATAP:[^ ]+]] = call <8 x i64> @llvm.genx.svm.gather.v8i64.v8i1.v8i64(<8 x i1> %mask, i32 0, <8 x i64> [[ADDRP]], <8 x i64> [[PTI]])
; CHECK-TYPED-PTRS: %res = inttoptr <8 x i64> [[DATAP]] to <8 x i8*>
; CHECK-OPAQUE-PTRS: %res = inttoptr <8 x i64> [[DATAP]] to <8 x ptr>
; CHECK-LSC-TYPED-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x i8*> %passthru to <8 x i64>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x i8* addrspace(1)*> %pptr to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x ptr> %passthru to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pptr to <8 x i64>
; CHECK-LSC: [[DATAP:[^ ]+]] = call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v8i1.v2i8.v8i64(<8 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDRP]], i16 1, i32 0, <8 x i64> [[PTI]])
; CHECK-LSC-TYPED-PTRS: %res = inttoptr <8 x i64> [[DATAP]] to <8 x i8*>
; CHECK-LSC-OPAQUE-PTRS: %res = inttoptr <8 x i64> [[DATAP]] to <8 x ptr>
  %res = call <8 x i8*> @llvm.masked.gather.v8p0i8.v8p1p0i8(<8 x i8* addrspace(1)*> %pptr, i32 8, <8 x i1> %mask, <8 x i8*> %passthru)
  ret <8 x i8*> %res
}
