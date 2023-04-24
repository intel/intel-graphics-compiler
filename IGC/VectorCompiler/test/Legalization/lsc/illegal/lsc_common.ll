;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test_baled_in_rdregion_split(<32 x i32> %0, <64 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_baled_in_rdregion_split

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %1, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %1, i32 {{[0-9]+}}, i32 16, i32 1, i16 64, i32 undef)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT0]], <16 x i32> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT1]], <16 x i32> [[ARG1SPLIT1]], i32 %2)
; CHECK:     ret void

  %data = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %1, i32 0, i32 32, i32 1, i16 0, i32 undef)
  call void @llvm.genx.lsc.store.bti.v32i1.v32i32.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <32 x i32> %0, <32 x i32> %data, i32 %2)
  ret void
}

define void @test_baled_into_wrregion_split(<32 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_baled_into_wrregion_split

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC0:%[^ ]+]] = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: [[LSC1:%[^ ]+]] = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT1]], i32 %1)
; CHECK-DAG: [[RES0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> undef, <16 x i32> [[LSC0]], i32 32, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> [[RES0]], <16 x i32> [[LSC1]], i32 32, i32 16, i32 1, i16 64, i32 undef, i1 true)
; CHECK:     ret void

  %data = call <32 x i32> @llvm.genx.lsc.load.bti.v32i32.v32i1.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <32 x i32> %0, i32 %1)
  %res = call <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.i1(<64 x i32> undef, <32 x i32> %data, i32 0, i32 32, i32 1, i16 0, i32 undef, i1 true)
  ret void
}

define void @test_constant(i32 %0) {
entry:
; CHECK-LABEL: test_constant

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v6i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <2 x i32> <i32 0, i32 12>, <6 x i32> <i32 0, i32 3, i32 1, i32 4, i32 2, i32 5>, i32 %0)
; CHECK-NEXT: call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v3i32(<1 x i1> {{<(i1 true(, )?){1}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <1 x i32> <i32 24>, <3 x i32> <i32 6, i32 7, i32 8>, i32 %0)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v3i1.v3i32.v9i32(<3 x i1> <i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <3 x i32> <i32 0, i32 12, i32 24>, <9 x i32> <i32 0, i32 3, i32 6, i32 1, i32 4, i32 7,i32 2, i32 5, i32 8>, i32 %0)
  ret void
}

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32>, i32, i32, i32, i16, i32)
declare <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32)
declare <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)
declare <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.i1(<64 x i32>, <32 x i32>, i32, i32, i32, i16, i32, i1)
declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <16 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.v32i1.v32i32.v32i32(<32 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <32 x i32>, <32 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.v3i1.v3i32.v9i32(<3 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <3 x i32>, <9 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <16 x i32>, i32)
declare <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)
declare <32 x i32> @llvm.genx.lsc.load.bti.v32i32.v32i1.v32i32(<32 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <32 x i32>, i32)
declare <16 x i32> @llvm.genx.lsc.load.bti.v16i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)
