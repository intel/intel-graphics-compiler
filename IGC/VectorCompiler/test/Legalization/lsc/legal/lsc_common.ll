;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test_baled_in_rdregion(<16 x i32> %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_baled_in_rdregion

; CHECK:      %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %data, i32 %2)
; CHECK-NEXT: ret void

  %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 0, i32 undef)
  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %data, i32 %2)
  ret void
}

define void @test_baled_in_rdregion_offset(<16 x i32> %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_baled_in_rdregion_offset

; CHECK:      %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 32, i32 undef)
; CHECK-NEXT: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %data, i32 %2)
; CHECK-NEXT: ret void

  %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 32, i32 undef)
  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %data, i32 %2)
  ret void
}

define void @test_not_baled_in_rdregion_stride(<16 x i32> %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_not_baled_in_rdregion_stride

; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %1, i32 16, i32 8, i32 2, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %1, i32 16, i32 8, i32 2, i16 64, i32 undef)
; CHECK-DAG: [[ARG1JOIN0:%[^ ]+]]  = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[ARG1SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1:%[^ ]+]]       = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[ARG1JOIN0]], <8 x i32> [[ARG1SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> [[ARG1]], i32 %2)
; CHECK:     ret void

  %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 2, i16 0, i32 undef)
  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %data, i32 %2)
  ret void
}

define void @test_not_baled_in_rdregion_offset(<16 x i32> %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_not_baled_in_rdregion_offset

; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %1, i32 8, i32 8, i32 1, i16 4, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %1, i32 8, i32 8, i32 1, i16 36, i32 undef)
; CHECK-DAG: [[ARG1JOIN0:%[^ ]+]]  = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[ARG1SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1:%[^ ]+]]       = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[ARG1JOIN0]], <8 x i32> [[ARG1SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> [[ARG1]], i32 %2)
; CHECK:     ret void

  %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 4, i32 undef)
  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %data, i32 %2)
  ret void
}

define void @test_baled_into_wrregion(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_baled_into_wrregion

; CHECK:      %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: ret void

  %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
  %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ret void
}

define void @test_baled_into_wrregion_offset(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_baled_into_wrregion_offset

; CHECK:      %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: ret void

  %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
  %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)
  ret void
}

define void @test_not_baled_into_wrregion_stride(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_not_baled_into_wrregion_stride

; CHECK-DAG: [[LSC0:%[^ ]+]]       = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-DAG: [[LSC0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LSC0]], i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LSC0]], i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[RES0:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v8i32.i16.i1(<32 x i32> undef, <8 x i32> [[LSC0SPLIT0]], i32 16, i32 8, i32 2, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v8i32.i16.i1(<32 x i32> [[RES0]], <8 x i32> [[LSC0SPLIT1]], i32 16, i32 8, i32 2, i16 64, i32 undef, i1 true)
; CHECK: ret void

  %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
  %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 2, i16 0, i32 undef, i1 true)
  ret void
}

define void @test_not_baled_into_wrregion_offset(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_not_baled_into_wrregion_offset

; CHECK-DAG: [[LSC0:%[^ ]+]]       = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-DAG: [[LSC0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LSC0]], i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LSC0]], i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[RES0:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v8i32.i16.i1(<32 x i32> undef, <8 x i32> [[LSC0SPLIT0]], i32 8, i32 8, i32 1, i16 4, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v8i32.i16.i1(<32 x i32> [[RES0]], <8 x i32> [[LSC0SPLIT1]], i32 8, i32 8, i32 1, i16 36, i32 undef, i1 true)

  %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
  %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 4, i32 undef, i1 true)
  ret void
}

define void @test_baled_in_rdregion_transposed(i32 %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_baled_in_rdregion_transposed

; CHECK:      %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT: call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> %data, i32 %2)
; CHECK-NEXT: ret void

  %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 0, i32 undef)
  call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> %data, i32 %2)
  ret void
}

define void @test_baled_in_rdregion_offset_transposed(i32 %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_baled_in_rdregion_offset_transposed

; CHECK:      %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 32, i32 undef)
; CHECK-NEXT: call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> %data, i32 %2)
; CHECK-NEXT: ret void

  %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 32, i32 undef)
  call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> %data, i32 %2)
  ret void
}

define void @test_not_baled_in_rdregion_stride_transposed(i32 %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_not_baled_in_rdregion_stride_transposed

; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %1, i32 16, i32 8, i32 2, i16 0, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %1, i32 16, i32 8, i32 2, i16 64, i32 undef)
; CHECK-DAG: [[ARG1JOIN0:%[^ ]+]]  = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[ARG1SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1:%[^ ]+]]       = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[ARG1JOIN0]], <8 x i32> [[ARG1SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> [[ARG1]], i32 %2)
; CHECK:     ret void

  %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 2, i16 0, i32 undef)
  call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> %data, i32 %2)
  ret void
}

define void @test_not_baled_in_rdregion_offset_transposed(i32 %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test_not_baled_in_rdregion_offset_transposed

; CHECK-DAG: [[ARG1SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %1, i32 8, i32 8, i32 1, i16 4, i32 undef)
; CHECK-DAG: [[ARG1SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %1, i32 8, i32 8, i32 1, i16 36, i32 undef)
; CHECK-DAG: [[ARG1JOIN0:%[^ ]+]]  = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[ARG1SPLIT0]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[ARG1:%[^ ]+]]       = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[ARG1JOIN0]], <8 x i32> [[ARG1SPLIT1]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-DAG: call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> [[ARG1]], i32 %2)
; CHECK:     ret void

  %data = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %1, i32 0, i32 16, i32 1, i16 4, i32 undef)
  call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> %data, i32 %2)
  ret void
}

define void @test_baled_into_wrregion_transposed(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test_baled_into_wrregion_transposed

; CHECK:      %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: ret void

  %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
  %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ret void
}

define void @test_baled_into_wrregion_offset_transposed(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test_baled_into_wrregion_offset_transposed

; CHECK:      %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: ret void

  %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
  %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)
  ret void
}

define void @test_not_baled_into_wrregion_stride_transposed(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test_not_baled_into_wrregion_stride_transposed

; CHECK-DAG: [[LSC0:%[^ ]+]]       = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-DAG: [[LSC0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LSC0]], i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LSC0]], i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[RES0:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v8i32.i16.i1(<32 x i32> undef, <8 x i32> [[LSC0SPLIT0]], i32 16, i32 8, i32 2, i16 0, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v8i32.i16.i1(<32 x i32> [[RES0]], <8 x i32> [[LSC0SPLIT1]], i32 16, i32 8, i32 2, i16 64, i32 undef, i1 true)
; CHECK: ret void

  %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
  %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 2, i16 0, i32 undef, i1 true)
  ret void
}

define void @test_not_baled_into_wrregion_offset_transposed(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test_not_baled_into_wrregion_offset_transposed

; CHECK-DAG: [[LSC0:%[^ ]+]]       = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-DAG: [[LSC0SPLIT0:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LSC0]], i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK-DAG: [[LSC0SPLIT1:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[LSC0]], i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK-DAG: [[RES0:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v8i32.i16.i1(<32 x i32> undef, <8 x i32> [[LSC0SPLIT0]], i32 8, i32 8, i32 1, i16 4, i32 undef, i1 true)
; CHECK-DAG: [[RES1:%[^ ]+]]       = call <32 x i32> @llvm.genx.wrregioni.v32i32.v8i32.i16.i1(<32 x i32> [[RES0]], <8 x i32> [[LSC0SPLIT1]], i32 8, i32 8, i32 1, i16 36, i32 undef, i1 true)

  %data = call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
  %res = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %data, i32 0, i32 16, i32 1, i16 4, i32 undef, i1 true)
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
