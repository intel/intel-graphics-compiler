;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify  -igc-rewrite-local-size -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; RewriteLocalSize
; ------------------------------------------------

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_rewrite_local(i32 %src1) {
; CHECK-LABEL: @test_rewrite_local(
; CHECK:    [[TMP1:%.*]] = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 [[SRC1:%.*]])
; CHECK:    ret void
;
  %1 = call spir_func i32 @__builtin_IB_get_local_size(i32 %src1)
  ret void
}

declare spir_func i32 @__builtin_IB_get_local_size(i32)
