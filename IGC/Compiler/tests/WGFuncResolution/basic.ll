;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-wg-resolution -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; WGFuncResolution
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_wg(i32* %dst, i32 %src) {
; CHECK-LABEL: @test_wg(
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.WorkGroupAny(i32 %src)
; CHECK:    store i32 [[TMP1]], i32* %dst, align 4
; CHECK:    ret void
;
  %1 = call i32 @__builtin_IB_work_group_any(i32 %src)
  store i32 %1, i32* %dst, align 4
  ret void
}

declare i32 @__builtin_IB_work_group_any(i32)
