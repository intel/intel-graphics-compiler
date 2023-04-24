;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

; Width is not a power of 2, must be split into 2 parts
define void @test_illegalwidthsplit2(<3 x i32> %0, <3 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_illegalwidthsplit2

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v3i32.i16(<3 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v3i32.i16(<3 x i32> %0, i32 {{[0-9]+}}, i32 1, i32 1, i16 8, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v3i32.i16(<3 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v3i32.i16(<3 x i32> %1, i32 {{[0-9]+}}, i32 1, i32 1, i16 8, i32 undef)

; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> [[ARG0SPLIT0]], <2 x i32> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i32(<1 x i1> {{<(i1 true(, )?){1}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[ARG0SPLIT1]], <1 x i32> [[ARG1SPLIT1]], i32 %2)

; CHECK: ret void

  call void @llvm.genx.lsc.store.bti.v3i1.v3i32.v3i32(<3 x i1> <i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <3 x i32> %0, <3 x i32> %1, i32 %2)
  ret void
}

; Width is not a power of 2, must be split into 3 parts
define void @test_illegalwidthsplit3(<7 x i32> %0, <7 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_illegalwidthsplit3

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v7i32.i16(<7 x i32> %0, i32 {{[0-9]+}}, i32 4, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v7i32.i16(<7 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 16, i32 undef)
; CHECK-DAG: [[ARG0SPLIT2:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v7i32.i16(<7 x i32> %0, i32 {{[0-9]+}}, i32 1, i32 1, i16 24, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v7i32.i16(<7 x i32> %1, i32 {{[0-9]+}}, i32 4, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v7i32.i16(<7 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 16, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v7i32.i16(<7 x i32> %1, i32 {{[0-9]+}}, i32 1, i32 1, i16 24, i32 undef)

; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i32(<4 x i1> {{<(i1 true(, )?){4}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <4 x i32> [[ARG0SPLIT0]], <4 x i32> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> [[ARG0SPLIT1]], <2 x i32> [[ARG1SPLIT1]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i32(<1 x i1> {{<(i1 true(, )?){1}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[ARG0SPLIT2]], <1 x i32> [[ARG1SPLIT2]], i32 %2)

; CHECK: ret void

  call void @llvm.genx.lsc.store.bti.v7i1.v7i32.v7i32(<7 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <7 x i32> %0, <7 x i32> %1, i32 %2)
  ret void
}

; Too wide, must be split into 2 parts
define void @test_toowide(<32 x i32> %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_toowide

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 64, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 {{[0-9]+}}, i32 16, i32 1, i16 64, i32 undef)

; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT0]], <16 x i32> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT1]], <16 x i32> [[ARG1SPLIT1]], i32 %2)

; CHECK: ret void

  call void @llvm.genx.lsc.store.bti.v32i1.v32i32.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <32 x i32> %0, <32 x i32> %1, i32 %2)
  ret void
}

; Takes up 16 registers, must be split into 2 parts of 8 registers
define void @test_toomanyregisters(<16 x i32> %0, <128 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_toomanyregisters

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 256, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 320, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 384, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 448, i32 undef)

; CHECK-DAG: [[ARG1SPLIT1SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 288, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 352, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 416, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v128i32.i16(<128 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 480, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0JOIN0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> undef,               <8 x i32> [[ARG1SPLIT0SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN1:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN0]], <8 x i32> [[ARG1SPLIT0SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN2:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN1]], <8 x i32> [[ARG1SPLIT0SPLIT2]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN3:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN2]], <8 x i32> [[ARG1SPLIT0SPLIT3]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN4:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN3]], <8 x i32> [[ARG1SPLIT0SPLIT4]], i32 0, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN5:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN4]], <8 x i32> [[ARG1SPLIT0SPLIT5]], i32 0, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN6:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN5]], <8 x i32> [[ARG1SPLIT0SPLIT6]], i32 0, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]]      = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN6]], <8 x i32> [[ARG1SPLIT0SPLIT7]], i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)

; CHECK-DAG: [[ARG1SPLIT1JOIN0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> undef,               <8 x i32> [[ARG1SPLIT1SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN1:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN0]], <8 x i32> [[ARG1SPLIT1SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN2:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN1]], <8 x i32> [[ARG1SPLIT1SPLIT2]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN3:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN2]], <8 x i32> [[ARG1SPLIT1SPLIT3]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN4:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN3]], <8 x i32> [[ARG1SPLIT1SPLIT4]], i32 0, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN5:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN4]], <8 x i32> [[ARG1SPLIT1SPLIT5]], i32 0, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN6:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN5]], <8 x i32> [[ARG1SPLIT1SPLIT6]], i32 0, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]]      = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN6]], <8 x i32> [[ARG1SPLIT1SPLIT7]], i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)

; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], <64 x i32> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT1]], <64 x i32> [[ARG1SPLIT1]], i32 %2)

; CHECK:     ret void

  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v128i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <16 x i32> %0, <128 x i32> %1, i32 %2)
  ret void
}

; Too wide and takes up 32 registers, must be split into 4 parts of 8 registers
define void @test_toowidetoomanyregisters(<32 x i32> %0, <256 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_toowidetoomanyregisters

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[ARG0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[ARG0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 128, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 256, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 384, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 512, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 640, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 768, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 896, i32 undef)

; CHECK-DAG: [[ARG1SPLIT1SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 288, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 416, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 544, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 672, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 800, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 928, i32 undef)

; CHECK-DAG: [[ARG1SPLIT2SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 64, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 320, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 448, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 576, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 704, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 832, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 960, i32 undef)

; CHECK-DAG: [[ARG1SPLIT3SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 96, i32 undef)
; CHECK-DAG: [[ARG1SPLIT3SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)
; CHECK-DAG: [[ARG1SPLIT3SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 352, i32 undef)
; CHECK-DAG: [[ARG1SPLIT3SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 480, i32 undef)
; CHECK-DAG: [[ARG1SPLIT3SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 608, i32 undef)
; CHECK-DAG: [[ARG1SPLIT3SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 736, i32 undef)
; CHECK-DAG: [[ARG1SPLIT3SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 864, i32 undef)
; CHECK-DAG: [[ARG1SPLIT3SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v256i32.i16(<256 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 992, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0JOIN0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> undef,               <8 x i32> [[ARG1SPLIT0SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN1:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN0]], <8 x i32> [[ARG1SPLIT0SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN2:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN1]], <8 x i32> [[ARG1SPLIT0SPLIT2]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN3:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN2]], <8 x i32> [[ARG1SPLIT0SPLIT3]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN4:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN3]], <8 x i32> [[ARG1SPLIT0SPLIT4]], i32 0, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN5:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN4]], <8 x i32> [[ARG1SPLIT0SPLIT5]], i32 0, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN6:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN5]], <8 x i32> [[ARG1SPLIT0SPLIT6]], i32 0, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]]      = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN6]], <8 x i32> [[ARG1SPLIT0SPLIT7]], i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)

; CHECK-DAG: [[ARG1SPLIT1JOIN0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> undef,               <8 x i32> [[ARG1SPLIT1SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN1:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN0]], <8 x i32> [[ARG1SPLIT1SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN2:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN1]], <8 x i32> [[ARG1SPLIT1SPLIT2]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN3:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN2]], <8 x i32> [[ARG1SPLIT1SPLIT3]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN4:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN3]], <8 x i32> [[ARG1SPLIT1SPLIT4]], i32 0, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN5:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN4]], <8 x i32> [[ARG1SPLIT1SPLIT5]], i32 0, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN6:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN5]], <8 x i32> [[ARG1SPLIT1SPLIT6]], i32 0, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]]      = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT1JOIN6]], <8 x i32> [[ARG1SPLIT1SPLIT7]], i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)

; CHECK-DAG: [[ARG1SPLIT2JOIN0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> undef,               <8 x i32> [[ARG1SPLIT2SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN1:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT2JOIN0]], <8 x i32> [[ARG1SPLIT2SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN2:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT2JOIN1]], <8 x i32> [[ARG1SPLIT2SPLIT2]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN3:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT2JOIN2]], <8 x i32> [[ARG1SPLIT2SPLIT3]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN4:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT2JOIN3]], <8 x i32> [[ARG1SPLIT2SPLIT4]], i32 0, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN5:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT2JOIN4]], <8 x i32> [[ARG1SPLIT2SPLIT5]], i32 0, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN6:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT2JOIN5]], <8 x i32> [[ARG1SPLIT2SPLIT6]], i32 0, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2:%[^ ]+]]      = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT2JOIN6]], <8 x i32> [[ARG1SPLIT2SPLIT7]], i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)

; CHECK-DAG: [[ARG1SPLIT3JOIN0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> undef,               <8 x i32> [[ARG1SPLIT3SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT3JOIN1:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT3JOIN0]], <8 x i32> [[ARG1SPLIT3SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT3JOIN2:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT3JOIN1]], <8 x i32> [[ARG1SPLIT3SPLIT2]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT3JOIN3:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT3JOIN2]], <8 x i32> [[ARG1SPLIT3SPLIT3]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT3JOIN4:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT3JOIN3]], <8 x i32> [[ARG1SPLIT3SPLIT4]], i32 0, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT3JOIN5:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT3JOIN4]], <8 x i32> [[ARG1SPLIT3SPLIT5]], i32 0, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT3JOIN6:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT3JOIN5]], <8 x i32> [[ARG1SPLIT3SPLIT6]], i32 0, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT3:%[^ ]+]]      = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT3JOIN6]], <8 x i32> [[ARG1SPLIT3SPLIT7]], i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)

; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], <64 x i32> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT1]], <64 x i32> [[ARG1SPLIT1]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT2]], <64 x i32> [[ARG1SPLIT2]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT3]], <64 x i32> [[ARG1SPLIT3]], i32 %2)

; CHECK:     ret void

  call void @llvm.genx.lsc.store.bti.v32i1.v32i32.v256i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <32 x i32> %0, <256 x i32> %1, i32 %2)
  ret void
}

; Width is not a power of 2 and takes up 16 registers, must be split into 2 parts
define void @test_toomanyregisterssplit2(<10 x i32> %0, <80 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_toomanyregisterssplit2

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v10i32.i16(<10 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v10i32.i16(<10 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 32, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 40, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 80, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 120, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 200, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 240, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 280, i32 undef)

; CHECK-DAG: [[ARG1SPLIT1SPLIT0:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 72, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT2:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 112, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT3:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 152, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT4:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 192, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT5:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 232, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT6:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 272, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT7:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v80i32.i16(<80 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 312, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0JOIN0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> undef,               <8 x i32> [[ARG1SPLIT0SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN1:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN0]], <8 x i32> [[ARG1SPLIT0SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN2:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN1]], <8 x i32> [[ARG1SPLIT0SPLIT2]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN3:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN2]], <8 x i32> [[ARG1SPLIT0SPLIT3]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN4:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN3]], <8 x i32> [[ARG1SPLIT0SPLIT4]], i32 0, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN5:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN4]], <8 x i32> [[ARG1SPLIT0SPLIT5]], i32 0, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN6:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN5]], <8 x i32> [[ARG1SPLIT0SPLIT6]], i32 0, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]]      = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN6]], <8 x i32> [[ARG1SPLIT0SPLIT7]], i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)

; CHECK-DAG: [[ARG1SPLIT1JOIN0:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> undef,                      <2 x i32> [[ARG1SPLIT1SPLIT0]], i32 0, i32 2, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN1:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT1JOIN0]], <2 x i32> [[ARG1SPLIT1SPLIT1]], i32 0, i32 2, i32 1, i16 8, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN2:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT1JOIN1]], <2 x i32> [[ARG1SPLIT1SPLIT2]], i32 0, i32 2, i32 1, i16 16, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN3:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT1JOIN2]], <2 x i32> [[ARG1SPLIT1SPLIT3]], i32 0, i32 2, i32 1, i16 24, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN4:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT1JOIN3]], <2 x i32> [[ARG1SPLIT1SPLIT4]], i32 0, i32 2, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN5:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT1JOIN4]], <2 x i32> [[ARG1SPLIT1SPLIT5]], i32 0, i32 2, i32 1, i16 40, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN6:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT1JOIN5]], <2 x i32> [[ARG1SPLIT1SPLIT6]], i32 0, i32 2, i32 1, i16 48, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]]      = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT1JOIN6]], <2 x i32> [[ARG1SPLIT1SPLIT7]], i32 0, i32 2, i32 1, i16 56, i32 undef, i1 true)

; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], <64 x i32> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v16i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> [[ARG0SPLIT1]], <16 x i32> [[ARG1SPLIT1]], i32 %2)

; CHECK:     ret void

  call void @llvm.genx.lsc.store.bti.v10i1.v10i32.v80i32(<10 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <10 x i32> %0, <80 x i32> %1, i32 %2)
  ret void
}

; Width is not a power of 2 and takes up 16 registers, must be split into 3 parts
define void @test_toomanyregisterssplit3(<14 x i32> %0, <112 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_toomanyregisterssplit3

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v14i32.i16(<14 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v14i32.i16(<14 x i32> %0, i32 {{[0-9]+}}, i32 4, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[ARG0SPLIT2:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v14i32.i16(<14 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 48, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 56, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 112, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT3:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 168, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT4:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 224, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT5:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 280, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT6:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 336, i32 undef)
; CHECK-DAG: [[ARG1SPLIT0SPLIT7:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 8, i32 1, i16 392, i32 undef)

; CHECK-DAG: [[ARG1SPLIT1SPLIT0:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 4, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT1:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 4, i32 1, i16 88, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT2:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 4, i32 1, i16 144, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT3:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 4, i32 1, i16 200, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT4:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 4, i32 1, i16 256, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT5:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 4, i32 1, i16 312, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT6:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 4, i32 1, i16 368, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1SPLIT7:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 4, i32 1, i16 424, i32 undef)

; CHECK-DAG: [[ARG1SPLIT2SPLIT0:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 48, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 104, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT2:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 160, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT3:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 216, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT4:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 272, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT5:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 328, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT6:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 384, i32 undef)
; CHECK-DAG: [[ARG1SPLIT2SPLIT7:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v112i32.i16(<112 x i32> %1, i32 {{[0-9]+}}, i32 2, i32 1, i16 440, i32 undef)

; CHECK-DAG: [[ARG1SPLIT0JOIN0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> undef,                    <8 x i32> [[ARG1SPLIT0SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN1:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN0]], <8 x i32> [[ARG1SPLIT0SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN2:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN1]], <8 x i32> [[ARG1SPLIT0SPLIT2]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN3:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN2]], <8 x i32> [[ARG1SPLIT0SPLIT3]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN4:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN3]], <8 x i32> [[ARG1SPLIT0SPLIT4]], i32 0, i32 8, i32 1, i16 128, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN5:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN4]], <8 x i32> [[ARG1SPLIT0SPLIT5]], i32 0, i32 8, i32 1, i16 160, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0JOIN6:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN5]], <8 x i32> [[ARG1SPLIT0SPLIT6]], i32 0, i32 8, i32 1, i16 192, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]]      = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> [[ARG1SPLIT0JOIN6]], <8 x i32> [[ARG1SPLIT0SPLIT7]], i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)

; CHECK-DAG: [[ARG1SPLIT1JOIN0:%[^ ]+]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v4i32.i16.i1(<32 x i32> undef,                    <4 x i32> [[ARG1SPLIT1SPLIT0]], i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN1:%[^ ]+]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v4i32.i16.i1(<32 x i32> [[ARG1SPLIT1JOIN0]], <4 x i32> [[ARG1SPLIT1SPLIT1]], i32 0, i32 4, i32 1, i16 16, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN2:%[^ ]+]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v4i32.i16.i1(<32 x i32> [[ARG1SPLIT1JOIN1]], <4 x i32> [[ARG1SPLIT1SPLIT2]], i32 0, i32 4, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN3:%[^ ]+]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v4i32.i16.i1(<32 x i32> [[ARG1SPLIT1JOIN2]], <4 x i32> [[ARG1SPLIT1SPLIT3]], i32 0, i32 4, i32 1, i16 48, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN4:%[^ ]+]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v4i32.i16.i1(<32 x i32> [[ARG1SPLIT1JOIN3]], <4 x i32> [[ARG1SPLIT1SPLIT4]], i32 0, i32 4, i32 1, i16 64, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN5:%[^ ]+]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v4i32.i16.i1(<32 x i32> [[ARG1SPLIT1JOIN4]], <4 x i32> [[ARG1SPLIT1SPLIT5]], i32 0, i32 4, i32 1, i16 80, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1JOIN6:%[^ ]+]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v4i32.i16.i1(<32 x i32> [[ARG1SPLIT1JOIN5]], <4 x i32> [[ARG1SPLIT1SPLIT6]], i32 0, i32 4, i32 1, i16 96, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]]      = call <32 x i32> @llvm.genx.wrregioni.v32i32.v4i32.i16.i1(<32 x i32> [[ARG1SPLIT1JOIN6]], <4 x i32> [[ARG1SPLIT1SPLIT7]], i32 0, i32 4, i32 1, i16 112, i32 undef, i1 true)

; CHECK-DAG: [[ARG1SPLIT2JOIN0:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> undef,                     <2 x i32> [[ARG1SPLIT2SPLIT0]], i32 0, i32 2, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN1:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT2JOIN0]], <2 x i32> [[ARG1SPLIT2SPLIT1]], i32 0, i32 2, i32 1, i16 8, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN2:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT2JOIN1]], <2 x i32> [[ARG1SPLIT2SPLIT2]], i32 0, i32 2, i32 1, i16 16, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN3:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT2JOIN2]], <2 x i32> [[ARG1SPLIT2SPLIT3]], i32 0, i32 2, i32 1, i16 24, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN4:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT2JOIN3]], <2 x i32> [[ARG1SPLIT2SPLIT4]], i32 0, i32 2, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN5:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT2JOIN4]], <2 x i32> [[ARG1SPLIT2SPLIT5]], i32 0, i32 2, i32 1, i16 40, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2JOIN6:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT2JOIN5]], <2 x i32> [[ARG1SPLIT2SPLIT6]], i32 0, i32 2, i32 1, i16 48, i32 undef, i1 true)
; CHECK-DAG: [[ARG1SPLIT2:%[^ ]+]]      = call <16 x i32> @llvm.genx.wrregioni.v16i32.v2i32.i16.i1(<16 x i32> [[ARG1SPLIT2JOIN6]], <2 x i32> [[ARG1SPLIT2SPLIT7]], i32 0, i32 2, i32 1, i16 56, i32 undef, i1 true)

; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], <64 x i32> [[ARG1SPLIT0]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v32i32(<4 x i1> {{<(i1 true(, )?){4}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <4 x i32> [[ARG0SPLIT1]], <32 x i32> [[ARG1SPLIT1]], i32 %2)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v16i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> [[ARG0SPLIT2]], <16 x i32> [[ARG1SPLIT2]], i32 %2)

; CHECK:     ret void

  call void @llvm.genx.lsc.store.bti.v14i1.v14i32.v112i32(<14 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <14 x i32> %0, <112 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v3i1.v3i32.v3i32(<3 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <3 x i32>, <3 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.v7i1.v7i32.v7i32(<7 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <7 x i32>, <7 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.v32i1.v32i32.v32i32(<32 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <32 x i32>, <32 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v128i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <128 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.v32i1.v32i32.v256i32(<32 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <32 x i32>, <256 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.v10i1.v10i32.v80i32(<10 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <10 x i32>, <80 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.v14i1.v14i32.v112i32(<14 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <14 x i32>, <112 x i32>, i32)
