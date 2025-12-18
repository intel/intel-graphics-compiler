;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=Xe3P -S < %s 2>&1 | FileCheck %s

declare <32 x i16> @llvm.genx.mxfp.reduce.32x32.v32i16.v1024i16(<1024 x i16>)
declare <32 x half> @llvm.genx.mxfp.reduce.32x32.v32f16.v1024f16(<1024 x half>)
declare <32 x bfloat> @llvm.genx.mxfp.reduce.32x32.v32bf16.v1024bf16(<1024 x bfloat>)

; CHECK-LABEL: @test_mxfp_reduce_32x32_i16(
define <32 x i16> @test_mxfp_reduce_32x32_i16(<1024 x i16> %src) {
; CHECK: [[RES:%[^ ]+]] = call <32 x i16> @__vc_builtin_mxfp_reduce_32x32(<1024 x i16> %src)
; CHECK: ret <32 x i16> [[RES]]
  %1 = call <32 x i16> @llvm.genx.mxfp.reduce.32x32.v32i16.v1024i16(<1024 x i16> %src)
  ret <32 x i16> %1
}

; CHECK-LABEL: @test_mxfp_reduce_32x32_f16(
define <32 x half> @test_mxfp_reduce_32x32_f16(<1024 x half> %src) {
; CHECK: [[SRC:%[^ ]+]] = bitcast <1024 x half> %src to <1024 x i16>
; CHECK: [[RES:%[^ ]+]] = call <32 x i16> @__vc_builtin_mxfp_reduce_32x32(<1024 x i16> [[SRC]])
; CHECK: [[CAST:%[^ ]+]] = bitcast <32 x i16> [[RES]] to <32 x half>
; CHECK: ret <32 x half> [[CAST]]
  %1 = call <32 x half> @llvm.genx.mxfp.reduce.32x32.v32f16.v1024f16(<1024 x half> %src)
  ret <32 x half> %1
}

; CHECK-LABEL: @test_mxfp_reduce_32x32_bf16(
define <32 x bfloat> @test_mxfp_reduce_32x32_bf16(<1024 x bfloat> %src) {
; CHECK: [[SRC:%[^ ]+]] = bitcast <1024 x bfloat> %src to <1024 x i16>
; CHECK: [[RES:%[^ ]+]] = call <32 x i16> @__vc_builtin_mxfp_reduce_32x32(<1024 x i16> [[SRC]])
; CHECK: [[CAST:%[^ ]+]] = bitcast <32 x i16> [[RES]] to <32 x bfloat>
; CHECK: ret <32 x bfloat> [[CAST]]
  %1 = call <32 x bfloat> @llvm.genx.mxfp.reduce.32x32.v32bf16.v1024bf16(<1024 x bfloat> %src)
  ret <32 x bfloat> %1
}

declare <32 x i16> @llvm.genx.mxfp.linearize.v32i16(<32 x i16>)
declare <32 x half> @llvm.genx.mxfp.linearize.v32f16(<32 x half>)
declare <32 x bfloat> @llvm.genx.mxfp.linearize.v32bf16(<32 x bfloat>)

; CHECK-LABEL: @test_mxfp_linearize_i16(
define <32 x i16> @test_mxfp_linearize_i16(<32 x i16> %src) {
; CHECK: [[RES:%[^ ]+]] = call <32 x i16> @__vc_builtin_mxfp_linearize(<32 x i16> %src)
; CHECK: ret <32 x i16> [[RES]]
  %1 = call <32 x i16> @llvm.genx.mxfp.linearize.v32i16(<32 x i16> %src)
  ret <32 x i16> %1
}

; CHECK-LABEL: @test_mxfp_linearize_f16(
define <32 x half> @test_mxfp_linearize_f16(<32 x half> %src) {
; CHECK: [[SRC:%[^ ]+]] = bitcast <32 x half> %src to <32 x i16>
; CHECK: [[RES:%[^ ]+]] = call <32 x i16> @__vc_builtin_mxfp_linearize(<32 x i16> [[SRC]])
; CHECK: [[CAST:%[^ ]+]] = bitcast <32 x i16> [[RES]] to <32 x half>
; CHECK: ret <32 x half> [[CAST]]
  %1 = call <32 x half> @llvm.genx.mxfp.linearize.v32f16(<32 x half> %src)
  ret <32 x half> %1
}

; CHECK-LABEL: @test_mxfp_linearize_bf16(
define <32 x bfloat> @test_mxfp_linearize_bf16(<32 x bfloat> %src) {
; CHECK: [[SRC:%[^ ]+]] = bitcast <32 x bfloat> %src to <32 x i16>
; CHECK: [[RES:%[^ ]+]] = call <32 x i16> @__vc_builtin_mxfp_linearize(<32 x i16> [[SRC]])
; CHECK: [[CAST:%[^ ]+]] = bitcast <32 x i16> [[RES]] to <32 x bfloat>
; CHECK: ret <32 x bfloat> [[CAST]]
  %1 = call <32 x bfloat> @llvm.genx.mxfp.linearize.v32bf16(<32 x bfloat> %src)
  ret <32 x bfloat> %1
}

; COM: The presence of these __vc_builtin_* funcitions is a HACK to trick VC
; COM: backend into thinking that we have built-in routines
define <32 x i16> @__vc_builtin_mxfp_reduce_32x32(<1024 x i16> %src) #0 {
  ret <32 x i16> zeroinitializer
}

define <32 x i16> @__vc_builtin_mxfp_linearize(<32 x i16> %src) #0 {
  ret <32 x i16> zeroinitializer
}

attributes #0 = { "VC.Builtin" }
