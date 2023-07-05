;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;
; RUN: opt %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unkonwn-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: opt %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unkonwn-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; CHECK-LSC-NOT: WARNING
; CHECK-LSC: CheckModuleDebugify: PASS

; COM: Basic test on store lowering pass
; COM: @llvm.masked.scatter from addrspace(3)

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 3 (local) operations are lowered into bti(254)/slm intrinsics

declare void @llvm.masked.scatter.v8p1i8.v8i8(<8 x i8>, <8 x i8 addrspace(3)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1i16.v8i16(<8 x i16>, <8 x i16 addrspace(3)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1i32.v8i32(<8 x i32>, <8 x i32 addrspace(3)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1i64.v8i64(<8 x i64>, <8 x i64 addrspace(3)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1f16.v8f16(<8 x half>, <8 x half addrspace(3)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1f32.v8f32(<8 x float>, <8 x float addrspace(3)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1f64.v8f64(<8 x double>, <8 x double addrspace(3)*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p1p0i8.v8p0i8(<8 x i8*>, <8 x i8* addrspace(3)*>, i32, <8 x i1>)

define void @test_i8(<8 x i8 addrspace(3)*> %pi8, <8 x i1> %mask, <8 x i8> %data) {
; CHECK-DAG: [[DATA8:[^ ]+]] = zext <8 x i8> %data to <8 x i32>
; CHECK-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x i8 addrspace(3)*> %pi8 to <8 x i32>
; CHECK: call void @llvm.genx.scatter.scaled.v8i1.v8i32.v8i32(<8 x i1> %mask, i32 0, i16 0, i32 254, i32 0, <8 x i32> [[ADDR8]], <8 x i32> [[DATA8]])
; CHECK-SAME: !VCAlignment [[MDALIGN1:![0-9]+]]
; CHECK-LSC-DAG: [[DATA8:[^ ]+]] = zext <8 x i8> %data to <8 x i32>
; CHECK-LSC-DAG: [[ADDR8:[^ ]+]] = ptrtoint <8 x i8 addrspace(3)*> %pi8 to <8 x i32>
; CHECK-LSC: call void @llvm.genx.lsc.store.slm.v8i1.v8i32.v8i32(<8 x i1> %mask, i8 4, i8 0, i8 0, i16 1, i32 0, i8 5, i8 1, i8 1, i8 0, <8 x i32> [[ADDR8]], <8 x i32> [[DATA8]], i32 0)
; CHECK-LSC-SAME: !VCAlignment [[MDALIGN1:![0-9]+]]
  call void @llvm.masked.scatter.v8p1i8.v8i8(<8 x i8> %data, <8 x i8 addrspace(3)*> %pi8, i32 1, <8 x i1> %mask)
  ret void
}

define void @test_i16(<8 x i16 addrspace(3)*> %pi16, <8 x i1> %mask, <8 x i16> %data) {
; CHECK-DAG: [[DATA16:[^ ]+]] = zext <8 x i16> %data to <8 x i32>
; CHECK-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x i16 addrspace(3)*> %pi16 to <8 x i32>
; CHECK: call void @llvm.genx.scatter.scaled.v8i1.v8i32.v8i32(<8 x i1> %mask, i32 1, i16 0, i32 254, i32 0, <8 x i32> [[ADDR16]], <8 x i32> [[DATA16]])
; CHECK-SAME: !VCAlignment [[MDALIGN2:![0-9]+]]
; CHECK-LSC-DAG: [[DATA16:[^ ]+]] = zext <8 x i16> %data to <8 x i32>
; CHECK-LSC-DAG: [[ADDR16:[^ ]+]] = ptrtoint <8 x i16 addrspace(3)*> %pi16 to <8 x i32>
; CHECK-LSC: call void @llvm.genx.lsc.store.slm.v8i1.v8i32.v8i32(<8 x i1> %mask, i8 4, i8 0, i8 0, i16 1, i32 0, i8 6, i8 1, i8 1, i8 0, <8 x i32> [[ADDR16]], <8 x i32> [[DATA16]], i32 0)
; CHECK-LSC-SAME: !VCAlignment [[MDALIGN2:![0-9]+]]
  call void @llvm.masked.scatter.v8p1i16.v8i16(<8 x i16> %data, <8 x i16 addrspace(3)*> %pi16, i32 2, <8 x i1> %mask)
  ret void
}

define void @test_i32(<8 x i32 addrspace(3)*> %pi32, <8 x i1> %mask, <8 x i32> %data) {
; CHECK: [[ADDR32:[^ ]+]] = ptrtoint <8 x i32 addrspace(3)*> %pi32 to <8 x i32>
; CHECK: call void @llvm.genx.scatter.scaled.v8i1.v8i32.v8i32(<8 x i1> %mask, i32 2, i16 0, i32 254, i32 0, <8 x i32> [[ADDR32]], <8 x i32> %data)
; CHECK-SAME: !VCAlignment [[MDALIGN4:![0-9]+]]
; CHECK-LSC: [[ADDR32:[^ ]+]] = ptrtoint <8 x i32 addrspace(3)*> %pi32 to <8 x i32>
; CHECK-LSC: call void @llvm.genx.lsc.store.slm.v8i1.v8i32.v8i32(<8 x i1> %mask, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <8 x i32> [[ADDR32]], <8 x i32> %data, i32 0)
; CHECK-LSC-SAME: !VCAlignment [[MDALIGN4:![0-9]+]]
  call void @llvm.masked.scatter.v8p1i32.v8i32(<8 x i32> %data, <8 x i32 addrspace(3)*> %pi32, i32 4, <8 x i1> %mask)
  ret void
}

define void @test_i64(<8 x i64 addrspace(3)*> %pi64, <8 x i1> %mask, <8 x i64> %data) {
; CHECK: [[ADDR64:[^ ]+]] = ptrtoint <8 x i64 addrspace(3)*> %pi64 to <8 x i32>
; CHECK: [[CAST64:[^ ]+]] = bitcast <8 x i64> %data to <16 x i32>
; CHECK: [[S2A64:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> [[CAST64]], i32 1, i32 8, i32 2, i16 0, i32 undef)
; CHECK: call void @llvm.genx.scatter4.scaled.v8i1.v8i32.v16i32(<8 x i1> %mask, i32 12, i16 0, i32 254, i32 0, <8 x i32> [[ADDR64]], <16 x i32> [[S2A64]])
; CHECK-SAME: !VCAlignment [[MDALIGN8:![0-9]+]]
; CHECK-LSC: [[ADDR64:[^ ]+]] = ptrtoint <8 x i64 addrspace(3)*> %pi64 to <8 x i32>
; CHECK-LSC: call void @llvm.genx.lsc.store.slm.v8i1.v8i32.v8i64(<8 x i1> %mask, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> [[ADDR64]], <8 x i64> %data, i32 0)
; CHECK-LSC-SAME: !VCAlignment [[MDALIGN8:![0-9]+]]
  call void @llvm.masked.scatter.v8p1i64.v8i64(<8 x i64> %data, <8 x i64 addrspace(3)*> %pi64, i32 8, <8 x i1> %mask)
  ret void
}

define void @test_f16(<8 x half addrspace(3)*> %pi16, <8 x i1> %mask, <8 x half> %data) {
; CHECK-DAG: [[CASTH:[^ ]+]] = bitcast <8 x half> %data to <8 x i16>
; CHECK-DAG: [[DATAH:[^ ]+]] = zext <8 x i16> [[CASTH]] to <8 x i32>
; CHECK-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x half addrspace(3)*> %pi16 to <8 x i32>
; CHECK: call void @llvm.genx.scatter.scaled.v8i1.v8i32.v8i32(<8 x i1> %mask, i32 1, i16 0, i32 254, i32 0, <8 x i32> [[ADDRH]], <8 x i32> [[DATAH]])
; CHECK-SAME: !VCAlignment [[MDALIGN2]]
; CHECK-LSC-DAG: [[CASTH:[^ ]+]] = bitcast <8 x half> %data to <8 x i16>
; CHECK-LSC-DAG: [[DATAH:[^ ]+]] = zext <8 x i16> [[CASTH]] to <8 x i32>
; CHECK-LSC-DAG: [[ADDRH:[^ ]+]] = ptrtoint <8 x half addrspace(3)*> %pi16 to <8 x i32>
; CHECK-LSC: call void @llvm.genx.lsc.store.slm.v8i1.v8i32.v8i32(<8 x i1> %mask, i8 4, i8 0, i8 0, i16 1, i32 0, i8 6, i8 1, i8 1, i8 0, <8 x i32> [[ADDRH]], <8 x i32> [[DATAH]], i32 0)
; CHECK-LSC-SAME: !VCAlignment [[MDALIGN2]]
  call void @llvm.masked.scatter.v8p1f16.v8f16(<8 x half> %data, <8 x half addrspace(3)*> %pi16, i32 2, <8 x i1> %mask)
  ret void
}

define void @test_f32(<8 x float addrspace(3)*> %pi32, <8 x i1> %mask, <8 x float> %data) {
; CHECK: [[ADDRF:[^ ]+]] = ptrtoint <8 x float addrspace(3)*> %pi32 to <8 x i32>
; CHECK: call void @llvm.genx.scatter.scaled.v8i1.v8i32.v8f32(<8 x i1> %mask, i32 2, i16 0, i32 254, i32 0, <8 x i32> [[ADDRF]], <8 x float> %data)
; CHECK-SAME: !VCAlignment [[MDALIGN4]]
; CHECK-LSC: [[ADDRF:[^ ]+]] = ptrtoint <8 x float addrspace(3)*> %pi32 to <8 x i32>
; CHECK-LSC: call void @llvm.genx.lsc.store.slm.v8i1.v8i32.v8f32(<8 x i1> %mask, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <8 x i32> [[ADDRF]], <8 x float> %data, i32 0)
; CHECK-LSC-SAME: !VCAlignment [[MDALIGN4]]
  call void @llvm.masked.scatter.v8p1f32.v8f32(<8 x float> %data, <8 x float addrspace(3)*> %pi32, i32 4, <8 x i1> %mask)
  ret void
}

define void @test_f64(<8 x double addrspace(3)*> %pi64, <8 x i1> %mask, <8 x double> %data) {
; CHECK-DAG: [[ADDRD:[^ ]+]] = ptrtoint <8 x double addrspace(3)*> %pi64 to <8 x i32>
; CHECK-DAG: [[CASTD:[^ ]+]] = bitcast <8 x double> %data to <16 x i32>
; CHECK-DAG: [[S2AD:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> [[CASTD]], i32 1, i32 8, i32 2, i16 0, i32 undef)
; CHECK: call void @llvm.genx.scatter4.scaled.v8i1.v8i32.v16i32(<8 x i1> %mask, i32 12, i16 0, i32 254, i32 0, <8 x i32> [[ADDRD]], <16 x i32> [[S2AD]])
; CHECK-SAME: !VCAlignment [[MDALIGN8]]
; CHECK-LSC: [[ADDRD:[^ ]+]] = ptrtoint <8 x double addrspace(3)*> %pi64 to <8 x i32>
; CHECK-LSC: call void @llvm.genx.lsc.store.slm.v8i1.v8i32.v8f64(<8 x i1> %mask, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> [[ADDRD]], <8 x double> %data, i32 0)
; CHECK-LSC-SAME: !VCAlignment [[MDALIGN8]]
  call void @llvm.masked.scatter.v8p1f64.v8f64(<8 x double> %data, <8 x double addrspace(3)*> %pi64, i32 8, <8 x i1> %mask)
  ret void
}

define void @test_ptr(<8 x i8* addrspace(3)*> %pptr, <8 x i1> %mask, <8 x i8*> %data) {
; CHECK-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x i8*> %data to <8 x i64>
; CHECK-DAG: [[CASTP:[^ ]+]] = bitcast <8 x i64> [[PTI]] to <16 x i32>
; CHECK-DAG: [[S2AP:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.i16(<16 x i32> [[CASTP]], i32 1, i32 8, i32 2, i16 0, i32 undef)
; CHECK-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x i8* addrspace(3)*> %pptr to <8 x i32>
; CHECK: call void @llvm.genx.scatter4.scaled.v8i1.v8i32.v16i32(<8 x i1> %mask, i32 12, i16 0, i32 254, i32 0, <8 x i32> [[ADDRP]], <16 x i32> [[S2AP]])
; CHECK-SAME: !VCAlignment [[MDALIGN8]]
; CHECK-LSC-DAG: [[PTI:[^ ]+]] = ptrtoint <8 x i8*> %data to <8 x i64>
; CHECK-LSC-DAG: [[ADDRP:[^ ]+]] = ptrtoint <8 x i8* addrspace(3)*> %pptr to <8 x i32>
; CHECK-LSC: call void @llvm.genx.lsc.store.slm.v8i1.v8i32.v8i64(<8 x i1> %mask, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> [[ADDRP]], <8 x i64> [[PTI]], i32 0)
; CHECK-LSC-SAME: !VCAlignment [[MDALIGN8]]
  call void @llvm.masked.scatter.v8p1p0i8.v8p0i8(<8 x i8*> %data, <8 x i8* addrspace(3)*> %pptr, i32 8, <8 x i1> %mask)
  ret void
}

define void @test_i64_unaligned(<8 x i64 addrspace(3)*> %pi64, <8 x i1> %mask, <8 x i64> %data) {
; CHECK: [[ADDRU64:[^ ]+]] = ptrtoint <8 x i64 addrspace(3)*> %pi64 to <8 x i32>
; CHECK: [[CASTU64:[^ ]+]] = bitcast <8 x i64> %data to <16 x i32>
; CHECK: [[LOWU64:[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[CASTU64]], i32 2, i32 1, i32 0, i16 0, i32 undef)
; CHECK: [[HIGHU64:[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[CASTU64]], i32 2, i32 1, i32 0, i16 4, i32 undef)
; CHECK: call void @llvm.genx.scatter.scaled.v8i1.v8i32.v8i32(<8 x i1> %mask, i32 2, i16 0, i32 254, i32 0, <8 x i32> [[ADDRU64]], <8 x i32> [[LOWU64]])
; CHECK-SAME: !VCAlignment [[MDALIGN1]]
; CHECK: call void @llvm.genx.scatter.scaled.v8i1.v8i32.v8i32(<8 x i1> %mask, i32 2, i16 0, i32 254, i32 4, <8 x i32> [[ADDRU64]], <8 x i32> [[HIGHU64]])
; CHECK-SAME: !VCAlignment [[MDALIGN1]]
; CHECK-LSC: [[ADDRU64:[^ ]+]] = ptrtoint <8 x i64 addrspace(3)*> %pi64 to <8 x i32>
; CHECK-LSC: call void @llvm.genx.lsc.store.slm.v8i1.v8i32.v8i64(<8 x i1> %mask, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> [[ADDRU64]], <8 x i64> %data, i32 0)
; CHECK-LSC-SAME: !VCAlignment [[MDALIGN1]]
  call void @llvm.masked.scatter.v8p1i64.v8i64(<8 x i64> %data, <8 x i64 addrspace(3)*> %pi64, i32 1, <8 x i1> %mask)
  ret void
}


; CHECK-DAG: [[MDALIGN1]] = !{i32 1}
; CHECK-DAG: [[MDALIGN2]] = !{i32 2}
; CHECK-DAG: [[MDALIGN4]] = !{i32 4}
; CHECK-DAG: [[MDALIGN8]] = !{i32 8}
; CHECK-LSC-DAG: [[MDALIGN1]] = !{i32 1}
; CHECK-LSC-DAG: [[MDALIGN2]] = !{i32 2}
; CHECK-LSC-DAG: [[MDALIGN4]] = !{i32 4}
; CHECK-LSC-DAG: [[MDALIGN8]] = !{i32 8}
