;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @some_func(<16 x i32>)

define <32 x i16> @test(i1 %pred) {
.entry:
; CHECK:        [[CONSTSPLAT:%.*]] = call <16 x i32> @llvm.genx.rdregioni
; CHECK-NEXT:   [[CONSTSPLAT_CAST:%.*]] = bitcast <16 x i32> [[CONSTSPLAT]] to <32 x i16>
  br i1 %pred, label %.loop_body, label %._crit_edge

.loop_body:
; CHECK: %phi1 = phi <16 x i32> [ [[CONSTSPLAT]], %.entry ], [ %some_init, %.loop_body ]
  %phi1 = phi <16 x i32> [ zeroinitializer, %.entry ], [%some_init, %.loop_body]
  %some_init = call <16 x i32> @some_func(<16 x i32> %phi1)
  br i1 %pred, label %.loop_body, label %._crit_edge.loopexit

._crit_edge.loopexit:                             ; preds = %.loop_body
  %bitcast49 = bitcast <16 x i32> %some_init to <32 x i16>
  br label %._crit_edge

._crit_edge:
; CHECK: %phi2 = phi <32 x i16> [ [[CONSTSPLAT_CAST]], %.entry ], [ %bitcast49, %._crit_edge.loopexit ]
  %phi2 = phi <32 x i16> [ zeroinitializer, %.entry ], [ %bitcast49, %._crit_edge.loopexit ]
  ret <32 x i16> %phi2
}
