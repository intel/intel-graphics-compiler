;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test verifies EnableScratchMessageD64WA flag

; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers --regkey EnableScratchMessageD64WA=1 -igc-int-type-legalizer -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define void @src(i64* %in, i64* %out) {
  %a = load i64, i64* %in, align 8
  store i64 %a, i64* %out, align 8
  ret void
}

; CHECK-LABEL: define void @src(
; CHECK: load <2 x i32>, <2 x i32>
; CHECK: store <2 x i32>
