;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

; Width is not a power of 2, must be split into 2 parts
define void @test_illegalwidthsplit2(<3 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_illegalwidthsplit2

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v3i32.i16(<3 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v3i32.i16(<3 x i32> %0, i32 {{[0-9]+}}, i32 1, i32 1, i16 8, i32 undef)

; CHECK-DAG: [[LSC0:%[^ ]+]]       = call <2 x i32> @llvm.genx.lsc.load.bti.v2i32.v2i1.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: [[LSC1:%[^ ]+]]       = call <1 x i32> @llvm.genx.lsc.load.bti.v1i32.v1i1.v1i32(<1 x i1> {{<(i1 true(, )?){1}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[ARG0SPLIT1]], i32 %1)

; CHECK-DAG: [[RES0:%[^ ]+]]       = call <3 x i32> @llvm.genx.wrregioni.v3i32.v2i32.i16.i1(<3 x i32> undef,    <2 x i32> [[LSC0]], i32 3, i32 2, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]       = call <3 x i32> @llvm.genx.wrregioni.v3i32.v1i32.i16.i1(<3 x i32> [[RES0]], <1 x i32> [[LSC1]], i32 3, i32 1, i32 1, i16 8, i32 undef, i1 true)

; CHECK: ret void

  %data = call <3 x i32> @llvm.genx.lsc.load.bti.v3i32.v3i1.v3i32(<3 x i1> <i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <3 x i32> %0, i32 %1)
  ret void
}

; Width is not a power of 2, must be split into 3 parts
define void @test_illegalwidthsplit3(<7 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_illegalwidthsplit3

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v7i32.i16(<7 x i32> %0, i32 {{[0-9]+}}, i32 4, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v7i32.i16(<7 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 16, i32 undef)
; CHECK-DAG: [[ARG0SPLIT2:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v7i32.i16(<7 x i32> %0, i32 {{[0-9]+}}, i32 1, i32 1, i16 24, i32 undef)

; CHECK-DAG: [[LSC0:%[^ ]+]]       = call <4 x i32> @llvm.genx.lsc.load.bti.v4i32.v4i1.v4i32(<4 x i1> {{<(i1 true(, )?){4}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <4 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: [[LSC1:%[^ ]+]]       = call <2 x i32> @llvm.genx.lsc.load.bti.v2i32.v2i1.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> [[ARG0SPLIT1]], i32 %1)
; CHECK-DAG: [[LSC2:%[^ ]+]]       = call <1 x i32> @llvm.genx.lsc.load.bti.v1i32.v1i1.v1i32(<1 x i1> {{<(i1 true(, )?){1}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[ARG0SPLIT2]], i32 %1)

; CHECK-DAG: [[RES0:%[^ ]+]]       = call <7 x i32> @llvm.genx.wrregioni.v7i32.v4i32.i16.i1(<7 x i32> undef,    <4 x i32> [[LSC0]], i32 7, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]       = call <7 x i32> @llvm.genx.wrregioni.v7i32.v2i32.i16.i1(<7 x i32> [[RES0]], <2 x i32> [[LSC1]], i32 7, i32 2, i32 1, i16 16, i32 undef, i1 true)
; CHECK-DAG: [[RES2:%[^ ]+]]       = call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> [[RES1]], <1 x i32> [[LSC2]], i32 7, i32 1, i32 1, i16 24, i32 undef, i1 true)

; CHECK: ret void

  %data = call <7 x i32> @llvm.genx.lsc.load.bti.v7i32.v7i1.v7i32(<7 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <7 x i32> %0, i32 %1)
  ret void
}

; Too wide, must be split into 2 parts
define void @test_toowide(<32 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_toowide

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 64, i32 undef)

; CHECK-DAG: [[LSC0:%[^ ]+]]       = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: [[LSC1:%[^ ]+]]       = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT1]], i32 %1)

; CHECK-DAG: [[RES0:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef,    <16 x i32> [[LSC0]], i32 32, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> [[RES0]], <16 x i32> [[LSC1]], i32 32, i32 16, i32 1, i16 64, i32 undef, i1 true)

; CHECK: ret void

  %data = call <32 x i32> @llvm.genx.lsc.load.bti.v32i32.v32i1.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <32 x i32> %0, i32 %1)
  ret void
}

; Takes up 16 registers, must be split into 2 parts of 8 registers
define void @test_toomanyregisters(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_toomanyregisters

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)

; CHECK-DAG: [[LSC0:%[^ ]+]] = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: [[LSC1:%[^ ]+]] = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT1]], i32 %1)

; CHECK-DAG: [[LSC0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[LSC0SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[LSC0SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[LSC0SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[LSC0SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)

; CHECK-DAG: [[LSC1SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC1SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC1SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC1SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[LSC1SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[LSC1SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[LSC1SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[LSC1SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)

; CHECK-DAG: [[RES0:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> undef, <8 x i32> [[LSC0SPLIT0]], i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES0]], <8 x i32> [[LSC0SPLIT1]], i32 16, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[RES2:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES1]], <8 x i32> [[LSC0SPLIT2]], i32 16, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[RES3:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES2]], <8 x i32> [[LSC0SPLIT3]], i32 16, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[RES4:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES3]], <8 x i32> [[LSC0SPLIT4]], i32 16, i32 8, i32 1, i16 256, i32 undef, i1 true)
; CHECK-DAG: [[RES5:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES4]], <8 x i32> [[LSC0SPLIT5]], i32 16, i32 8, i32 1, i16 320, i32 undef, i1 true)
; CHECK-DAG: [[RES6:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES5]], <8 x i32> [[LSC0SPLIT6]], i32 16, i32 8, i32 1, i16 384, i32 undef, i1 true)
; CHECK-DAG: [[RES7:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES6]], <8 x i32> [[LSC0SPLIT7]], i32 16, i32 8, i32 1, i16 448, i32 undef, i1 true)

; CHECK-DAG: [[RES8:%[^ ]+]]  = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES7]], <8 x i32> [[LSC1SPLIT0]], i32 16, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[RES9:%[^ ]+]]  = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES8]], <8 x i32> [[LSC1SPLIT1]], i32 16, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[RES10:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES9]], <8 x i32> [[LSC1SPLIT2]], i32 16, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[RES11:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES10]], <8 x i32> [[LSC1SPLIT3]], i32 16, i32 8, i32 1, i16 224, i32 undef, i1 true)
; CHECK-DAG: [[RES12:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES11]], <8 x i32> [[LSC1SPLIT4]], i32 16, i32 8, i32 1, i16 288, i32 undef, i1 true)
; CHECK-DAG: [[RES13:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES12]], <8 x i32> [[LSC1SPLIT5]], i32 16, i32 8, i32 1, i16 352, i32 undef, i1 true)
; CHECK-DAG: [[RES14:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES13]], <8 x i32> [[LSC1SPLIT6]], i32 16, i32 8, i32 1, i16 416, i32 undef, i1 true)
; CHECK-DAG: [[RES15:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v8i32.i16.i1(<128 x i32> [[RES14]], <8 x i32> [[LSC1SPLIT7]], i32 16, i32 8, i32 1, i16 480, i32 undef, i1 true)

; CHECK:     ret void

  %data = call <128 x i32> @llvm.genx.lsc.load.bti.v128i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <16 x i32> %0, i32 %1)
  ret void
}

; Too wide and takes up 32 registers, must be split into 4 parts of 8 registers
define void @test_toowidetoomanyregisters(<32 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_toowidetoomanyregisters

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[ARG0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[ARG0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)

; CHECK-DAG: [[LSC0:%[^ ]+]] = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: [[LSC1:%[^ ]+]] = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT1]], i32 %1)
; CHECK-DAG: [[LSC2:%[^ ]+]] = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT2]], i32 %1)
; CHECK-DAG: [[LSC3:%[^ ]+]] = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT3]], i32 %1)

; CHECK-DAG: [[LSC0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[LSC0SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[LSC0SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[LSC0SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[LSC0SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)

; CHECK-DAG: [[LSC1SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC1SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC1SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC1SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[LSC1SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[LSC1SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[LSC1SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[LSC1SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)

; CHECK-DAG: [[LSC2SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC2SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC2SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC2SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[LSC2SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[LSC2SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[LSC2SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[LSC2SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)

; CHECK-DAG: [[LSC3SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC3]], i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC3SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC3]], i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC3SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC3]], i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC3SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC3]], i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[LSC3SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC3]], i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[LSC3SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC3]], i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[LSC3SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC3]], i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[LSC3SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC3]], i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)

; CHECK-DAG: [[RES0:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> undef, <8 x i32> [[LSC0SPLIT0]], i32 32, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES0]], <8 x i32> [[LSC0SPLIT1]], i32 32, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[RES2:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES1]], <8 x i32> [[LSC0SPLIT2]], i32 32, i32 8, i32 1, i16 256, i32 undef, i1 true)
; CHECK-DAG: [[RES3:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES2]], <8 x i32> [[LSC0SPLIT3]], i32 32, i32 8, i32 1, i16 384, i32 undef, i1 true)
; CHECK-DAG: [[RES4:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES3]], <8 x i32> [[LSC0SPLIT4]], i32 32, i32 8, i32 1, i16 512, i32 undef, i1 true)
; CHECK-DAG: [[RES5:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES4]], <8 x i32> [[LSC0SPLIT5]], i32 32, i32 8, i32 1, i16 640, i32 undef, i1 true)
; CHECK-DAG: [[RES6:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES5]], <8 x i32> [[LSC0SPLIT6]], i32 32, i32 8, i32 1, i16 768, i32 undef, i1 true)
; CHECK-DAG: [[RES7:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES6]], <8 x i32> [[LSC0SPLIT7]], i32 32, i32 8, i32 1, i16 896, i32 undef, i1 true)

; CHECK-DAG: [[RES8:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES7]], <8 x i32> [[LSC1SPLIT0]], i32 32, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[RES9:%[^ ]+]]  = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES8]], <8 x i32> [[LSC1SPLIT1]], i32 32, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[RES10:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES9]], <8 x i32> [[LSC1SPLIT2]], i32 32, i32 8, i32 1, i16 288, i32 undef, i1 true)
; CHECK-DAG: [[RES11:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES10]], <8 x i32> [[LSC1SPLIT3]], i32 32, i32 8, i32 1, i16 416, i32 undef, i1 true)
; CHECK-DAG: [[RES12:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES11]], <8 x i32> [[LSC1SPLIT4]], i32 32, i32 8, i32 1, i16 544, i32 undef, i1 true)
; CHECK-DAG: [[RES13:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES12]], <8 x i32> [[LSC1SPLIT5]], i32 32, i32 8, i32 1, i16 672, i32 undef, i1 true)
; CHECK-DAG: [[RES14:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES13]], <8 x i32> [[LSC1SPLIT6]], i32 32, i32 8, i32 1, i16 800, i32 undef, i1 true)
; CHECK-DAG: [[RES15:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES14]], <8 x i32> [[LSC1SPLIT7]], i32 32, i32 8, i32 1, i16 928, i32 undef, i1 true)

; CHECK-DAG: [[RES16:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES15]], <8 x i32> [[LSC2SPLIT0]], i32 32, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[RES17:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES16]], <8 x i32> [[LSC2SPLIT1]], i32 32, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[RES18:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES17]], <8 x i32> [[LSC2SPLIT2]], i32 32, i32 8, i32 1, i16 320, i32 undef, i1 true)
; CHECK-DAG: [[RES19:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES18]], <8 x i32> [[LSC2SPLIT3]], i32 32, i32 8, i32 1, i16 448, i32 undef, i1 true)
; CHECK-DAG: [[RES20:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES19]], <8 x i32> [[LSC2SPLIT4]], i32 32, i32 8, i32 1, i16 576, i32 undef, i1 true)
; CHECK-DAG: [[RES21:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES20]], <8 x i32> [[LSC2SPLIT5]], i32 32, i32 8, i32 1, i16 704, i32 undef, i1 true)
; CHECK-DAG: [[RES22:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES21]], <8 x i32> [[LSC2SPLIT6]], i32 32, i32 8, i32 1, i16 832, i32 undef, i1 true)
; CHECK-DAG: [[RES23:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES22]], <8 x i32> [[LSC2SPLIT7]], i32 32, i32 8, i32 1, i16 960, i32 undef, i1 true)

; CHECK-DAG: [[RES24:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES23]], <8 x i32> [[LSC3SPLIT0]], i32 32, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[RES25:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES24]], <8 x i32> [[LSC3SPLIT1]], i32 32, i32 8, i32 1, i16 224, i32 undef, i1 true)
; CHECK-DAG: [[RES26:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES25]], <8 x i32> [[LSC3SPLIT2]], i32 32, i32 8, i32 1, i16 352, i32 undef, i1 true)
; CHECK-DAG: [[RES27:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES26]], <8 x i32> [[LSC3SPLIT3]], i32 32, i32 8, i32 1, i16 480, i32 undef, i1 true)
; CHECK-DAG: [[RES28:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES27]], <8 x i32> [[LSC3SPLIT4]], i32 32, i32 8, i32 1, i16 608, i32 undef, i1 true)
; CHECK-DAG: [[RES29:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES28]], <8 x i32> [[LSC3SPLIT5]], i32 32, i32 8, i32 1, i16 736, i32 undef, i1 true)
; CHECK-DAG: [[RES30:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES29]], <8 x i32> [[LSC3SPLIT6]], i32 32, i32 8, i32 1, i16 864, i32 undef, i1 true)
; CHECK-DAG: [[RES31:%[^ ]+]] = call <256 x i32> @llvm.genx.wrregioni.v256i32.v8i32.i16.i1(<256 x i32> [[RES30]], <8 x i32> [[LSC3SPLIT7]], i32 32, i32 8, i32 1, i16 992, i32 undef, i1 true)

; CHECK    : ret void

  %data = call <256 x i32> @llvm.genx.lsc.load.bti.v256i32.v32i1.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <32 x i32> %0, i32 %1)
  ret void
}

; Width is not a power of 2 and takes up 16 registers, must be split into 2 parts
define void @test_toomanyregisterssplit2(<10 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_toomanyregisterssplit2

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v10i32.i16(<10 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v10i32.i16(<10 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 32, i32 undef)

; CHECK-DAG: [[LSC0:%[^ ]+]] = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: [[LSC1:%[^ ]+]] = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v2i1.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> [[ARG0SPLIT1]], i32 %1)

; CHECK-DAG: [[LSC0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[LSC0SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[LSC0SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[LSC0SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[LSC0SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)

; CHECK-DAG: [[LSC1SPLIT0:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 2, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC1SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 2, i32 1, i16 8, i32 undef)
; CHECK-DAG: [[LSC1SPLIT2:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 2, i32 1, i16 16, i32 undef)
; CHECK-DAG: [[LSC1SPLIT3:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 2, i32 1, i16 24, i32 undef)
; CHECK-DAG: [[LSC1SPLIT4:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 2, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC1SPLIT5:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 2, i32 1, i16 40, i32 undef)
; CHECK-DAG: [[LSC1SPLIT6:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 2, i32 1, i16 48, i32 undef)
; CHECK-DAG: [[LSC1SPLIT7:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 2, i32 1, i16 56, i32 undef)

; CHECK-DAG: [[RES0:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v8i32.i16.i1(<80 x i32> undef, <8 x i32> [[LSC0SPLIT0]], i32 10, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v8i32.i16.i1(<80 x i32> [[RES0]], <8 x i32> [[LSC0SPLIT1]], i32 10, i32 8, i32 1, i16 40, i32 undef, i1 true)
; CHECK-DAG: [[RES2:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v8i32.i16.i1(<80 x i32> [[RES1]], <8 x i32> [[LSC0SPLIT2]], i32 10, i32 8, i32 1, i16 80, i32 undef, i1 true)
; CHECK-DAG: [[RES3:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v8i32.i16.i1(<80 x i32> [[RES2]], <8 x i32> [[LSC0SPLIT3]], i32 10, i32 8, i32 1, i16 120, i32 undef, i1 true)
; CHECK-DAG: [[RES4:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v8i32.i16.i1(<80 x i32> [[RES3]], <8 x i32> [[LSC0SPLIT4]], i32 10, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[RES5:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v8i32.i16.i1(<80 x i32> [[RES4]], <8 x i32> [[LSC0SPLIT5]], i32 10, i32 8, i32 1, i16 200, i32 undef, i1 true)
; CHECK-DAG: [[RES6:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v8i32.i16.i1(<80 x i32> [[RES5]], <8 x i32> [[LSC0SPLIT6]], i32 10, i32 8, i32 1, i16 240, i32 undef, i1 true)
; CHECK-DAG: [[RES7:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v8i32.i16.i1(<80 x i32> [[RES6]], <8 x i32> [[LSC0SPLIT7]], i32 10, i32 8, i32 1, i16 280, i32 undef, i1 true)

; CHECK-DAG: [[RES8:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v2i32.i16.i1(<80 x i32> [[RES7]], <2 x i32> [[LSC1SPLIT0]], i32 10, i32 2, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[RES9:%[^ ]+]]  = call <80 x i32> @llvm.genx.wrregioni.v80i32.v2i32.i16.i1(<80 x i32> [[RES8]], <2 x i32> [[LSC1SPLIT1]], i32 10, i32 2, i32 1, i16 72, i32 undef, i1 true)
; CHECK-DAG: [[RES10:%[^ ]+]] = call <80 x i32> @llvm.genx.wrregioni.v80i32.v2i32.i16.i1(<80 x i32> [[RES9]], <2 x i32> [[LSC1SPLIT2]], i32 10, i32 2, i32 1, i16 112, i32 undef, i1 true)
; CHECK-DAG: [[RES11:%[^ ]+]] = call <80 x i32> @llvm.genx.wrregioni.v80i32.v2i32.i16.i1(<80 x i32> [[RES10]], <2 x i32> [[LSC1SPLIT3]], i32 10, i32 2, i32 1, i16 152, i32 undef, i1 true)
; CHECK-DAG: [[RES12:%[^ ]+]] = call <80 x i32> @llvm.genx.wrregioni.v80i32.v2i32.i16.i1(<80 x i32> [[RES11]], <2 x i32> [[LSC1SPLIT4]], i32 10, i32 2, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[RES13:%[^ ]+]] = call <80 x i32> @llvm.genx.wrregioni.v80i32.v2i32.i16.i1(<80 x i32> [[RES12]], <2 x i32> [[LSC1SPLIT5]], i32 10, i32 2, i32 1, i16 232, i32 undef, i1 true)
; CHECK-DAG: [[RES14:%[^ ]+]] = call <80 x i32> @llvm.genx.wrregioni.v80i32.v2i32.i16.i1(<80 x i32> [[RES13]], <2 x i32> [[LSC1SPLIT6]], i32 10, i32 2, i32 1, i16 272, i32 undef, i1 true)
; CHECK-DAG: [[RES15:%[^ ]+]] = call <80 x i32> @llvm.genx.wrregioni.v80i32.v2i32.i16.i1(<80 x i32> [[RES14]], <2 x i32> [[LSC1SPLIT7]], i32 10, i32 2, i32 1, i16 312, i32 undef, i1 true)

; CHECK:     ret void

  %data = call <80 x i32> @llvm.genx.lsc.load.bti.v80i32.v10i1.v10i32(<10 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <10 x i32> %0, i32 %1)
  ret void
}

; Width is not a power of 2 and takes up 16 registers, must be split into 3 parts
define void @test_toomanyregisterssplit3(<14 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_toomanyregisterssplit3

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v14i32.i16(<14 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v14i32.i16(<14 x i32> %0, i32 {{[0-9]+}}, i32 4, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[ARG0SPLIT2:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v14i32.i16(<14 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 48, i32 undef)

; CHECK-DAG: [[LSC0:%[^ ]+]] = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: [[LSC1:%[^ ]+]] = call <32 x i32> @llvm.genx.lsc.load.bti.v32i32.v4i1.v4i32(<4 x i1> {{<(i1 true(, )?){4}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <4 x i32> [[ARG0SPLIT1]], i32 %1)
; CHECK-DAG: [[LSC2:%[^ ]+]] = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v2i1.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> [[ARG0SPLIT2]], i32 %1)

; CHECK-DAG: [[LSC0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[LSC0SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[LSC0SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[LSC0SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[LSC0SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[LSC0]], i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)

; CHECK-DAG: [[LSC1SPLIT0:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v32i32.i16(<32 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 4, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC1SPLIT1:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v32i32.i16(<32 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 4, i32 1, i16 16, i32 undef)
; CHECK-DAG: [[LSC1SPLIT2:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v32i32.i16(<32 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 4, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC1SPLIT3:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v32i32.i16(<32 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 4, i32 1, i16 48, i32 undef)
; CHECK-DAG: [[LSC1SPLIT4:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v32i32.i16(<32 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 4, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[LSC1SPLIT5:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v32i32.i16(<32 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 4, i32 1, i16 80, i32 undef)
; CHECK-DAG: [[LSC1SPLIT6:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v32i32.i16(<32 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 4, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[LSC1SPLIT7:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v32i32.i16(<32 x i32> [[LSC1]], i32 {{[0-9]+}}, i32 4, i32 1, i16 112, i32 undef)

; CHECK-DAG: [[LSC2SPLIT0:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 2, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC2SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 2, i32 1, i16 8, i32 undef)
; CHECK-DAG: [[LSC2SPLIT2:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 2, i32 1, i16 16, i32 undef)
; CHECK-DAG: [[LSC2SPLIT3:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 2, i32 1, i16 24, i32 undef)
; CHECK-DAG: [[LSC2SPLIT4:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 2, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[LSC2SPLIT5:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 2, i32 1, i16 40, i32 undef)
; CHECK-DAG: [[LSC2SPLIT6:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 2, i32 1, i16 48, i32 undef)
; CHECK-DAG: [[LSC2SPLIT7:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[LSC2]], i32 {{[0-9]+}}, i32 2, i32 1, i16 56, i32 undef)

; CHECK-DAG: [[RES0:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v8i32.i16.i1(<112 x i32> undef,    <8 x i32> [[LSC0SPLIT0]], i32 14, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v8i32.i16.i1(<112 x i32> [[RES0]], <8 x i32> [[LSC0SPLIT1]], i32 14, i32 8, i32 1, i16 56, i32 undef, i1 true)
; CHECK-DAG: [[RES2:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v8i32.i16.i1(<112 x i32> [[RES1]], <8 x i32> [[LSC0SPLIT2]], i32 14, i32 8, i32 1, i16 112, i32 undef, i1 true)
; CHECK-DAG: [[RES3:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v8i32.i16.i1(<112 x i32> [[RES2]], <8 x i32> [[LSC0SPLIT3]], i32 14, i32 8, i32 1, i16 168, i32 undef, i1 true)
; CHECK-DAG: [[RES4:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v8i32.i16.i1(<112 x i32> [[RES3]], <8 x i32> [[LSC0SPLIT4]], i32 14, i32 8, i32 1, i16 224, i32 undef, i1 true)
; CHECK-DAG: [[RES5:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v8i32.i16.i1(<112 x i32> [[RES4]], <8 x i32> [[LSC0SPLIT5]], i32 14, i32 8, i32 1, i16 280, i32 undef, i1 true)
; CHECK-DAG: [[RES6:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v8i32.i16.i1(<112 x i32> [[RES5]], <8 x i32> [[LSC0SPLIT6]], i32 14, i32 8, i32 1, i16 336, i32 undef, i1 true)
; CHECK-DAG: [[RES7:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v8i32.i16.i1(<112 x i32> [[RES6]], <8 x i32> [[LSC0SPLIT7]], i32 14, i32 8, i32 1, i16 392, i32 undef, i1 true)

; CHECK-DAG: [[RES8:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v4i32.i16.i1(<112 x i32> [[RES7]],  <4 x i32> [[LSC1SPLIT0]], i32 14, i32 4, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[RES9:%[^ ]+]]  = call <112 x i32> @llvm.genx.wrregioni.v112i32.v4i32.i16.i1(<112 x i32> [[RES8]],  <4 x i32> [[LSC1SPLIT1]], i32 14, i32 4, i32 1, i16 88, i32 undef, i1 true)
; CHECK-DAG: [[RES10:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v4i32.i16.i1(<112 x i32> [[RES9]],  <4 x i32> [[LSC1SPLIT2]], i32 14, i32 4, i32 1, i16 144, i32 undef, i1 true)
; CHECK-DAG: [[RES11:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v4i32.i16.i1(<112 x i32> [[RES10]], <4 x i32> [[LSC1SPLIT3]], i32 14, i32 4, i32 1, i16 200, i32 undef, i1 true)
; CHECK-DAG: [[RES12:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v4i32.i16.i1(<112 x i32> [[RES11]], <4 x i32> [[LSC1SPLIT4]], i32 14, i32 4, i32 1, i16 256, i32 undef, i1 true)
; CHECK-DAG: [[RES13:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v4i32.i16.i1(<112 x i32> [[RES12]], <4 x i32> [[LSC1SPLIT5]], i32 14, i32 4, i32 1, i16 312, i32 undef, i1 true)
; CHECK-DAG: [[RES14:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v4i32.i16.i1(<112 x i32> [[RES13]], <4 x i32> [[LSC1SPLIT6]], i32 14, i32 4, i32 1, i16 368, i32 undef, i1 true)
; CHECK-DAG: [[RES15:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v4i32.i16.i1(<112 x i32> [[RES14]], <4 x i32> [[LSC1SPLIT7]], i32 14, i32 4, i32 1, i16 424, i32 undef, i1 true)

; CHECK-DAG: [[RES16:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v2i32.i16.i1(<112 x i32> [[RES15]], <2 x i32> [[LSC2SPLIT0]], i32 14, i32 2, i32 1, i16 48, i32 undef, i1 true)
; CHECK-DAG: [[RES17:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v2i32.i16.i1(<112 x i32> [[RES16]], <2 x i32> [[LSC2SPLIT1]], i32 14, i32 2, i32 1, i16 104, i32 undef, i1 true)
; CHECK-DAG: [[RES18:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v2i32.i16.i1(<112 x i32> [[RES17]], <2 x i32> [[LSC2SPLIT2]], i32 14, i32 2, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[RES19:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v2i32.i16.i1(<112 x i32> [[RES18]], <2 x i32> [[LSC2SPLIT3]], i32 14, i32 2, i32 1, i16 216, i32 undef, i1 true)
; CHECK-DAG: [[RES20:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v2i32.i16.i1(<112 x i32> [[RES19]], <2 x i32> [[LSC2SPLIT4]], i32 14, i32 2, i32 1, i16 272, i32 undef, i1 true)
; CHECK-DAG: [[RES21:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v2i32.i16.i1(<112 x i32> [[RES20]], <2 x i32> [[LSC2SPLIT5]], i32 14, i32 2, i32 1, i16 328, i32 undef, i1 true)
; CHECK-DAG: [[RES22:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v2i32.i16.i1(<112 x i32> [[RES21]], <2 x i32> [[LSC2SPLIT6]], i32 14, i32 2, i32 1, i16 384, i32 undef, i1 true)
; CHECK-DAG: [[RES23:%[^ ]+]] = call <112 x i32> @llvm.genx.wrregioni.v112i32.v2i32.i16.i1(<112 x i32> [[RES22]], <2 x i32> [[LSC2SPLIT7]], i32 14, i32 2, i32 1, i16 440, i32 undef, i1 true)

; CHECK:     ret void

  %data = call <112 x i32> @llvm.genx.lsc.load.bti.v112i32.v14i1.v14i32(<14 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <14 x i32> %0, i32 %1)
  ret void
}

declare <3 x i32>   @llvm.genx.lsc.load.bti.v3i32.v3i1.v3i32(<3 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <3 x i32>, i32)
declare <7 x i32>   @llvm.genx.lsc.load.bti.v7i32.v7i1.v7i32(<7 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <7 x i32>, i32)
declare <32 x i32>  @llvm.genx.lsc.load.bti.v32i32.v32i1.v32i32(<32 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <32 x i32>, i32)
declare <128 x i32> @llvm.genx.lsc.load.bti.v128i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)
declare <256 x i32> @llvm.genx.lsc.load.bti.v256i32.v32i1.v32i32(<32 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <32 x i32>, i32)
declare <80 x i32>  @llvm.genx.lsc.load.bti.v80i32.v10i1.v10i32(<10 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <10 x i32>, i32)
declare <112 x i32> @llvm.genx.lsc.load.bti.v112i32.v14i1.v14i32(<14 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <14 x i32>, i32)
