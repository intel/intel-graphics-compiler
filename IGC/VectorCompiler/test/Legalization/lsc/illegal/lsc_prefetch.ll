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

; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> {{<(i1 true(, )?){1}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[ARG0SPLIT1]], i32 %1)

; CHECK: ret void

  call void @llvm.genx.lsc.prefetch.bti.v3i1.v3i32(<3 x i1> <i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <3 x i32> %0, i32 %1)
  ret void
}

; Width is not a power of 2, must be split into 3 parts
define void @test_illegalwidthsplit3(<7 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_illegalwidthsplit3

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v7i32.i16(<7 x i32> %0, i32 {{[0-9]+}}, i32 4, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v7i32.i16(<7 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 16, i32 undef)
; CHECK-DAG: [[ARG0SPLIT2:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v7i32.i16(<7 x i32> %0, i32 {{[0-9]+}}, i32 1, i32 1, i16 24, i32 undef)

; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> {{<(i1 true(, )?){4}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <4 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> [[ARG0SPLIT1]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> {{<(i1 true(, )?){1}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[ARG0SPLIT2]], i32 %1)

; CHECK: ret void

  call void @llvm.genx.lsc.prefetch.bti.v7i1.v7i32(<7 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <7 x i32> %0, i32 %1)
  ret void
}

; Too wide, must be split into 2 parts
define void @test_toowide(<32 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_toowide

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %0, i32 {{[0-9]+}}, i32 16, i32 1, i16 64, i32 undef)

; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> [[ARG0SPLIT1]], i32 %1)

; CHECK: ret void

  call void @llvm.genx.lsc.prefetch.bti.v32i1.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <32 x i32> %0, i32 %1)
  ret void
}

; Takes up 16 registers, must be split into 2 parts of 8 registers
define void @test_toomanyregisters(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_toomanyregisters

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 32, i32 undef)

; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT1]], i32 %1)

; CHECK:     ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <16 x i32> %0, i32 %1)
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

; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT1]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT2]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT3]], i32 %1)

; CHECK    : ret void

  call void @llvm.genx.lsc.prefetch.bti.v32i1.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <32 x i32> %0, i32 %1)
  ret void
}

; Width is not a power of 2 and takes up 16 registers, must be split into 2 parts
define void @test_toomanyregisterssplit2(<10 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_toomanyregisterssplit2

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v10i32.i16(<10 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v10i32.i16(<10 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 32, i32 undef)

; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> [[ARG0SPLIT1]], i32 %1)

; CHECK:     ret void

  call void @llvm.genx.lsc.prefetch.bti.v10i1.v10i32(<10 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <10 x i32> %0, i32 %1)
  ret void
}

; Width is not a power of 2 and takes up 16 registers, must be split into 3 parts
define void @test_toomanyregisterssplit3(<14 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_toomanyregisterssplit3

; CHECK-DAG: [[ARG0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v14i32.i16(<14 x i32> %0, i32 {{[0-9]+}}, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[ARG0SPLIT1:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v14i32.i16(<14 x i32> %0, i32 {{[0-9]+}}, i32 4, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[ARG0SPLIT2:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v14i32.i16(<14 x i32> %0, i32 {{[0-9]+}}, i32 2, i32 1, i16 48, i32 undef)

; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> {{<(i1 true(, )?){8}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> [[ARG0SPLIT0]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> {{<(i1 true(, )?){4}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <4 x i32> [[ARG0SPLIT1]], i32 %1)
; CHECK-DAG: call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> {{<(i1 true(, )?){2}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> [[ARG0SPLIT2]], i32 %1)

; CHECK:     ret void

  call void @llvm.genx.lsc.prefetch.bti.v14i1.v14i32(<14 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <14 x i32> %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v3i1.v3i32(<3 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <3 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v7i1.v7i32(<7 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <7 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v32i1.v32i32(<32 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <32 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v10i1.v10i32(<10 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <10 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v14i1.v14i32(<14 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <14 x i32>, i32)
