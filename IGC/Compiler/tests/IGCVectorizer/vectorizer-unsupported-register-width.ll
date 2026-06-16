;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S -opaque-pointers -platformdg2 --igc-vectorizer -dce --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 --regkey=VectorizerAllowWAVEALL=1 --regkey=VectorizerAllowWAVEALLJoint=1 < %s 2>&1 | FileCheck %s
; RUN: igc_opt -S -opaque-pointers -platformdg1 --igc-vectorizer -dce --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 --regkey=VectorizerAllowWAVEALL=1 --regkey=VectorizerAllowWAVEALLJoint=1 < %s 2>&1 | FileCheck %s
; RUN: igc_opt -S -opaque-pointers -platformmtl --igc-vectorizer -dce --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 --regkey=VectorizerAllowWAVEALL=1 --regkey=VectorizerAllowWAVEALLJoint=1 < %s 2>&1 | FileCheck %s
; CHECK: Unsupported register width

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @widget() {
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @widget, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
