;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --enable-debugify -platformdg2 --igc-arith-funcs-translation -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DpasFuncsResolution
; ------------------------------------------------

; Lower bf converts builtins

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_dpas(float %src, float* %dst) {
; CHECK-LABEL: @test_dpas(
; CHECK:    [[BF_CVT:%.*]] = call i16 @llvm.genx.GenISA.ftobf.i16.f32(float [[SRC:%.*]], i32 0)
; CHECK:    [[BF_CVT1:%.*]] = call float @llvm.genx.GenISA.bftof.f32.i16(i16 [[BF_CVT]])
; CHECK:    store float [[BF_CVT1]], float* [[DST:%.*]], align 4
; CHECK:    ret void
;
  %1 = call i16 @__builtin_IB_ftobf_1(float %src)
  %2 = call float @__builtin_IB_bftof_1(i16 %1)
  store float %2, float* %dst, align 4
  ret void
}

declare i16 @__builtin_IB_ftobf_1(float)
declare float @__builtin_IB_bftof_1(i16)
