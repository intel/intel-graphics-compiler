;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test_rdregion_baled(<32 x i32> %0, <64 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test_rdregion_baled

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v64i64.i16(<64 x i64> %1, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v64i64.i16(<64 x i64> %1, i32 {{[0-9]+}}, i32 16, i32 1, i16 128, i32 undef)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT0]], <16 x i64> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT1]], <16 x i64> [[ARG1SPLIT1]], i32 %2)
; CHECK:     ret void

  %data = call <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64> %1, i32 0, i32 32, i32 1, i16 0, i32 undef)
  call void @llvm.genx.lsc.store.bti.v32i1.v32i32.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <32 x i32> %0, <32 x i64> %data, i32 %2)
  ret void
}

define void @test_rdregion_not_baled(<32 x i32> %0, <64 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test_rdregion_not_baled

; CHECK-DAG: [[CAST:%[^ ]+]]       = bitcast <64 x i64> %1 to <128 x i32>

; CHECK-DAG: [[CASTSPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %.cast, i32 {{[0-9]+}}, i32 8, i32 1, i16 8, i32 undef)
; CHECK-DAG: [[CASTSPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %.cast, i32 {{[0-9]+}}, i32 8, i32 1, i16 40, i32 undef)
; CHECK-DAG: [[CASTSPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %.cast, i32 {{[0-9]+}}, i32 8, i32 1, i16 72, i32 undef)
; CHECK-DAG: [[CASTSPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %.cast, i32 {{[0-9]+}}, i32 8, i32 1, i16 104, i32 undef)
; CHECK-DAG: [[CASTSPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %.cast, i32 {{[0-9]+}}, i32 8, i32 1, i16 136, i32 undef)
; CHECK-DAG: [[CASTSPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %.cast, i32 {{[0-9]+}}, i32 8, i32 1, i16 168, i32 undef)
; CHECK-DAG: [[CASTSPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %.cast, i32 {{[0-9]+}}, i32 8, i32 1, i16 200, i32 undef)
; CHECK-DAG: [[CASTSPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %.cast, i32 {{[0-9]+}}, i32 8, i32 1, i16 232, i32 undef)

; CHECK-DAG: [[CASTJOIN0:%[^ ]+]]  = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> undef,         <8 x i32> [[CASTSPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[CASTJOIN1:%[^ ]+]]  = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[CASTJOIN0]], <8 x i32> [[CASTSPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[CASTJOIN2:%[^ ]+]]  = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[CASTJOIN1]], <8 x i32> [[CASTSPLIT2]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[CASTJOIN3:%[^ ]+]]  = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[CASTJOIN2]], <8 x i32> [[CASTSPLIT3]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[CASTJOIN4:%[^ ]+]]  = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[CASTJOIN3]], <8 x i32> [[CASTSPLIT4]], i32 0, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[CASTJOIN5:%[^ ]+]]  = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[CASTJOIN4]], <8 x i32> [[CASTSPLIT5]], i32 0, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[CASTJOIN6:%[^ ]+]]  = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[CASTJOIN5]], <8 x i32> [[CASTSPLIT6]], i32 0, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[CASTJOIN7:%[^ ]+]]  = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[CASTJOIN6]], <8 x i32> [[CASTSPLIT7]], i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)

; CHECK-DAG: [[ARG1:%[^ ]+]] = bitcast <64 x i32> [[CASTJOIN7]] to <32 x i64>

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 64, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> [[ARG1]], i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> [[ARG1]], i32 {{[0-9]+}}, i32 16, i32 1, i16 128, i32 undef)

; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT0]], <16 x i64> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT1]], <16 x i64> [[ARG1SPLIT1]], i32 %2)

; CHECK:     ret void

  %data = call <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64> %1, i32 0, i32 32, i32 1, i16 8, i32 undef)
  call void @llvm.genx.lsc.store.bti.v32i1.v32i32.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <32 x i32> %0, <32 x i64> %data, i32 %2)
  ret void
}

declare <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64>, i32, i32, i32, i16, i32)
declare void @llvm.genx.lsc.store.bti.v32i1.v32i32.v32i64(<32 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <32 x i32>, <32 x i64>, i32)
