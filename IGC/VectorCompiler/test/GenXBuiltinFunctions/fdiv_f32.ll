;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -enable-debugify -GenXBuiltinFunctions \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLPG -S < %s 2>&1 \
; RUN: | FileCheck %s

; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; Function Attrs: nofree nosync nounwind readnone
declare <32 x float> @llvm.genx.ieee.div.v32f32(<32 x float>, <32 x float>)

define dllexport spir_kernel void @test_kernel(<32 x float> %l, <32 x float> %r) {
  ; CHECK: = fdiv <32 x float> %l, %r
  %1 = fdiv <32 x float> %l, %r
  ; CHECK: = call <32 x float> @__vc_builtin_fdiv_v32f32(<32 x float> %l, <32 x float> %r)
  %2 = call <32 x float> @llvm.genx.ieee.div.v32f32(<32 x float> %l, <32 x float> %r)
  ret void
}

; COM: The presence of these __vc_builtin_* funcitions is a HACK to trick VC
; COM: backend into thinking that we have built-in routines
define <32 x float> @__vc_builtin_fdiv_v32f32(<32 x float> %l, <32 x float> %r) #0 {
  ret <32 x float> zeroinitializer
}

attributes #0 = { "VC.Builtin" }