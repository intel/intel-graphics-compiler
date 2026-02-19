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

; Lower idpas builtin

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_dpas(<8 x i32> %src, i32 %src2, i32* %dst) {
; CHECK-LABEL: @test_dpas(
; CHECK:    [[TMP1:%.*]] = load i32, i32* [[DST:%.*]], align 4
; CHECK:    [[DPAS:%.*]] = call i32 @llvm.genx.GenISA.sub.group.dpas.i32.i32.i32.v8i32(i32 [[SRC2:%.*]], i32 [[TMP1]], <8 x i32> [[SRC:%.*]], i32 4, i32 4, i32 8, i32 1, i1 false)
; CHECK:    store i32 [[DPAS]], i32* [[DST]], align 4
; CHECK:    ret void
;
  %1 = load i32, i32* %dst, align 4
  %2 = call i32 @__builtin_IB_sub_group_idpas_s8_s8_8_1(i32 %src2, i32 %1, <8 x i32> %src)
  store i32 %2, i32* %dst, align 4
  ret void
}
declare i32 @__builtin_IB_sub_group_idpas_s8_s8_8_1(i32, i32, <8 x i32>)
