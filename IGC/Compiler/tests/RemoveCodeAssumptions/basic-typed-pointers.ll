;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -igc-remove-code-assumptions | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define internal spir_func void @test(i64 %in) {
  ; BASIC TEST CASE
  ; CHECK-NOT: @llvm.assume
  ; CHECK: ret void
  %cmp = icmp sgt i64 %in, -1
  call void @llvm.assume(i1 %cmp) #3
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0
attributes #0 = { nounwind }

