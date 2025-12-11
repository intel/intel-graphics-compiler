;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXTranslateIntrinsics -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; REQUIRES: llvm_16_or_greater
; This test verifies that internal intrinsics have correct attributes in LLVM 16+:
; - Operations with NoWillReturn attribute should NOT have 'willreturn'
; - Regular operations should have 'willreturn' attribute

declare <16 x i8> @llvm.genx.lsc.load2d.typed.bti.v16i8(i8, i8, i32, i32, i32, i32, i32)
declare void @llvm.genx.lsc.store2d.typed.bti.v16i8(i8, i8, i32, i32, i32, i32, i32, <16 x i8>)

declare <32 x float> @llvm.genx.absf.v32f32(<32 x float>)
declare <32 x float> @llvm.genx.fmin.v32f32(<32 x float>, <32 x float>)
declare <32 x float> @llvm.genx.fmax.v32f32(<32 x float>, <32 x float>)

; CHECK-LABEL: @test_lsc_load2d_typed
define <16 x i8> @test_lsc_load2d_typed(i32 %bti, i32 %x, i32 %y) {
  ; CHECK: call <16 x i8> @llvm.vc.internal.lsc.load.2d.tgm.bti.v16i8.v2i8
  %res = call <16 x i8> @llvm.genx.lsc.load2d.typed.bti.v16i8(i8 0, i8 0, i32 %bti, i32 2, i32 8, i32 %x, i32 %y)
  ret <16 x i8> %res
}

; CHECK-LABEL: @test_lsc_store2d_typed
define void @test_lsc_store2d_typed(i32 %bti, i32 %x, i32 %y, <16 x i8> %data) {
  ; CHECK: call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v16i8
  call void @llvm.genx.lsc.store2d.typed.bti.v16i8(i8 0, i8 0, i32 %bti, i32 2, i32 8, i32 %x, i32 %y, <16 x i8> %data)
  ret void
}

; CHECK-LABEL: @test_math_absf
define <32 x float> @test_math_absf(<32 x float> %arg) {
  ; CHECK: call <32 x float> @llvm.fabs.v32f32
  %res = call <32 x float> @llvm.genx.absf.v32f32(<32 x float> %arg)
  ret <32 x float> %res
}

; CHECK-LABEL: @test_math_fmin
define <32 x float> @test_math_fmin(<32 x float> %a, <32 x float> %b) {
  ; CHECK: call <32 x float> @llvm.minnum.v32f32
  %res = call <32 x float> @llvm.genx.fmin.v32f32(<32 x float> %a, <32 x float> %b)
  ret <32 x float> %res
}

; CHECK-LABEL: @test_math_fmax
define <32 x float> @test_math_fmax(<32 x float> %a, <32 x float> %b) {
  ; CHECK: call <32 x float> @llvm.maxnum.v32f32
  %res = call <32 x float> @llvm.genx.fmax.v32f32(<32 x float> %a, <32 x float> %b)
  ret <32 x float> %res
}

; Verify that generated intrinsics have willreturn attribute in their declarations
; CHECK-DAG: declare {{.*}} @llvm.vc.internal.lsc.load.2d.tgm.bti{{.*}} #[[ATTR_LOAD:[0-9]+]]
; CHECK-DAG: declare {{.*}} @llvm.vc.internal.lsc.store.2d.tgm.bti{{.*}} #[[ATTR_STORE:[0-9]+]]
; CHECK-DAG: declare {{.*}} @llvm.fabs.v32f32{{.*}} #[[ATTR_MATH:[0-9]+]]
; CHECK-DAG: declare {{.*}} @llvm.minnum.v32f32{{.*}} #[[ATTR_MATH]]
; CHECK-DAG: declare {{.*}} @llvm.maxnum.v32f32{{.*}} #[[ATTR_MATH]]

; Verify attributes contain willreturn
; CHECK-DAG: attributes #[[ATTR_LOAD]] = {{.*}}willreturn{{.*}}
; CHECK-DAG: attributes #[[ATTR_STORE]] = {{.*}}willreturn{{.*}}
; CHECK-DAG: attributes #[[ATTR_MATH]] = {{.*}}willreturn{{.*}}
