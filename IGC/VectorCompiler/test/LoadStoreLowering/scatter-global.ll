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

; COM: Basic test on store lowering pass
; COM: @llvm.masked.scatter from addrspace(1)

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 1 (global) operations are lowered into svm/stateless intrinsics

declare void @llvm.masked.scatter.v8p1i8.v8i8(<8 x i8>, <8 x i8 addrspace(1)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1i16.v8i16(<8 x i16>, <8 x i16 addrspace(1)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1i32.v8i32(<8 x i32>, <8 x i32 addrspace(1)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1i64.v8i64(<8 x i64>, <8 x i64 addrspace(1)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1f16.v8f16(<8 x half>, <8 x half addrspace(1)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1f32.v8f32(<8 x float>, <8 x float addrspace(1)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1f64.v8f64(<8 x double>, <8 x double addrspace(1)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1p0i8.v8p0i8(<8 x i8*>, <8 x i8* addrspace(1)*>, i32, <8 x i1>)

define void @test_i8(<8 x i8 addrspace(1)*> %pi8, <8 x i1> %mask, <8 x i8> %data) {
; CHECK-DAG: [[DATA8:[^ ]+]] = zext <8 x i8> %data to <8 x i32>
; CHECK-DAG: [[CAST8:[^ ]+]] = bitcast <8 x i32> [[DATA8]] to <32 x i8>
; CHECK-TYPED-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x i8 addrspace(1)*> %pi8 to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi8 to <8 x i64>
; CHECK: call void @llvm.genx.svm.scatter.v8i1.v8i64.v32i8(<8 x i1> %mask, i32 0, <8 x i64> [[ADDR8]], <32 x i8> [[CAST8]])
; CHECK-LSC-DAG: [[DATA8:[^ ]+]] = zext <8 x i8> %data to <8 x i32>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x i8 addrspace(1)*> %pi8 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi8 to <8 x i64>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR8]], i16 1, i32 0, <8 x i32> [[DATA8]])
  call void @llvm.masked.scatter.v8p1i8.v8i8(<8 x i8> %data, <8 x i8 addrspace(1)*> %pi8, i32 1, <8 x i1> %mask)
  ret void
}

define void @test_i16(<8 x i16 addrspace(1)*> %pi16, <8 x i1> %mask, <8 x i16> %data) {
; CHECK-DAG: [[DATA16:[^ ]+]] = zext <8 x i16> %data to <8 x i32>
; CHECK-DAG: [[CAST16:[^ ]+]] = bitcast <8 x i32> [[DATA16]] to <32 x i8>
; CHECK-TYPED-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x i16 addrspace(1)*> %pi16 to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi16 to <8 x i64>
; CHECK: call void @llvm.genx.svm.scatter.v8i1.v8i64.v32i8(<8 x i1> %mask, i32 1, <8 x i64> [[ADDR16]], <32 x i8> [[CAST16]])
; CHECK-LSC-DAG: [[DATA16:[^ ]+]] = zext <8 x i16> %data to <8 x i32>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x i16 addrspace(1)*> %pi16 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi16 to <8 x i64>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR16]], i16 1, i32 0, <8 x i32> [[DATA16]])
  call void @llvm.masked.scatter.v8p1i16.v8i16(<8 x i16> %data, <8 x i16 addrspace(1)*> %pi16, i32 2, <8 x i1> %mask)
  ret void
}

define void @test_i32(<8 x i32 addrspace(1)*> %pi32, <8 x i1> %mask, <8 x i32> %data) {
; CHECK-TYPED-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x i32 addrspace(1)*> %pi32 to <8 x i64>
; CHECK-OPAQUE-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi32 to <8 x i64>
; CHECK: call void @llvm.genx.svm.scatter.v8i1.v8i64.v8i32(<8 x i1> %mask, i32 0, <8 x i64> [[ADDR32]], <8 x i32> %data)
; CHECK-LSC-TYPED-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x i32 addrspace(1)*> %pi32 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS: [[ADDR32:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi32 to <8 x i64>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR32]], i16 1, i32 0, <8 x i32> %data)
  call void @llvm.masked.scatter.v8p1i32.v8i32(<8 x i32> %data, <8 x i32 addrspace(1)*> %pi32, i32 4, <8 x i1> %mask)
  ret void
}

define void @test_i64(<8 x i64 addrspace(1)*> %pi64, <8 x i1> %mask, <8 x i64> %data) {
; CHECK-TYPED-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x i64 addrspace(1)*> %pi64 to <8 x i64>
; CHECK-OPAQUE-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi64 to <8 x i64>
; CHECK: call void @llvm.genx.svm.scatter.v8i1.v8i64.v8i64(<8 x i1> %mask, i32 0, <8 x i64> [[ADDR64]], <8 x i64> %data)
; CHECK-LSC-TYPED-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x i64 addrspace(1)*> %pi64 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS: [[ADDR64:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi64 to <8 x i64>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i64(<8 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR32]], i16 1, i32 0, <8 x i64> %data)
  call void @llvm.masked.scatter.v8p1i64.v8i64(<8 x i64> %data, <8 x i64 addrspace(1)*> %pi64, i32 8, <8 x i1> %mask)
  ret void
}

define void @test_f16(<8 x half addrspace(1)*> %pi16, <8 x i1> %mask, <8 x half> %data) {
; CHECK-DAG: [[CASTH:[^ ]+]] = bitcast <8 x half> %data to <8 x i16>
; CHECK-DAG: [[DATAH:[^ ]+]] = zext <8 x i16> [[CASTH]] to <8 x i32>
; CHECK-DAG: [[CAST2H:[^ ]+]] = bitcast <8 x i32> [[DATAH]] to <32 x i8>
; CHECK-TYPED-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x half addrspace(1)*> %pi16 to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi16 to <8 x i64>
; CHECK: call void @llvm.genx.svm.scatter.v8i1.v8i64.v32i8(<8 x i1> %mask, i32 1, <8 x i64> [[ADDRH]], <32 x i8> [[CAST2H]])
; CHECK-LSC-DAG: [[CASTH:[^ ]+]] = bitcast <8 x half> %data to <8 x i16>
; CHECK-LSC-DAG: [[DATAH:[^ ]+]] = zext <8 x i16> [[CASTH]] to <8 x i32>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x half addrspace(1)*> %pi16 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi16 to <8 x i64>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDRH]], i16 1, i32 0, <8 x i32> [[DATAH]])
  call void @llvm.masked.scatter.v8p1f16.v8f16(<8 x half> %data, <8 x half addrspace(1)*> %pi16, i32 2, <8 x i1> %mask)
  ret void
}

define void @test_f32(<8 x float addrspace(1)*> %pi32, <8 x i1> %mask, <8 x float> %data) {
; CHECK-TYPED-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x float addrspace(1)*> %pi32 to <8 x i64>
; CHECK-OPAQUE-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi32 to <8 x i64>
; CHECK: call void @llvm.genx.svm.scatter.v8i1.v8i64.v8f32(<8 x i1> %mask, i32 0, <8 x i64> [[ADDRF]], <8 x float> %data)
; CHECK-LSC-TYPED-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x float addrspace(1)*> %pi32 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS: [[ADDRF:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi32 to <8 x i64>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8f32(<8 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDRF]], i16 1, i32 0, <8 x float> %data)
  call void @llvm.masked.scatter.v8p1f32.v8f32(<8 x float> %data, <8 x float addrspace(1)*> %pi32, i32 4, <8 x i1> %mask)
  ret void
}

define void @test_f64(<8 x double addrspace(1)*> %pi64, <8 x i1> %mask, <8 x double> %data) {
; CHECK-TYPED-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x double addrspace(1)*> %pi64 to <8 x i64>
; CHECK-OPAQUE-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi64 to <8 x i64>
; CHECK: call void @llvm.genx.svm.scatter.v8i1.v8i64.v8f64(<8 x i1> %mask, i32 0, <8 x i64> [[ADDRD]], <8 x double> %data)
; CHECK-LSC-TYPED-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x double addrspace(1)*> %pi64 to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS: [[ADDRD:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pi64 to <8 x i64>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8f64(<8 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDRD]], i16 1, i32 0, <8 x double> %data)
  call void @llvm.masked.scatter.v8p1f64.v8f64(<8 x double> %data, <8 x double addrspace(1)*> %pi64, i32 8, <8 x i1> %mask)
  ret void
}

define void @test_ptr(<8 x i8* addrspace(1)*> %pptr, <8 x i1> %mask, <8 x i8*> %data) {
; CHECK-TYPED-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x i8*> %data to <8 x i64>
; CHECK-TYPED-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x i8* addrspace(1)*> %pptr to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x ptr> %data to <8 x i64>
; CHECK-OPAQUE-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pptr to <8 x i64>
; CHECK: call void @llvm.genx.svm.scatter.v8i1.v8i64.v8i64(<8 x i1> %mask, i32 0, <8 x i64> [[ADDRP]], <8 x i64> [[PTI]])
; CHECK-LSC-TYPED-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x i8*> %data to <8 x i64>
; CHECK-LSC-TYPED-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x i8* addrspace(1)*> %pptr to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x ptr> %data to <8 x i64>
; CHECK-LSC-OPAQUE-PTRS-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x ptr addrspace(1)> %pptr to <8 x i64>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i64(<8 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDRP]], i16 1, i32 0, <8 x i64> [[PTI]])
  call void @llvm.masked.scatter.v8p1p0i8.v8p0i8(<8 x i8*> %data, <8 x i8* addrspace(1)*> %pptr, i32 8, <8 x i1> %mask)
  ret void
}
