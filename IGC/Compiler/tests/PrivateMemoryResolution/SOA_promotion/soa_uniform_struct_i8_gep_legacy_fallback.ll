;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; The SOA checker accepts the struct only for the new SoA transpose without the !uniform metadata.
; With the !uniform metadata, the alloca should be rejected early so it doesn't fallback to legacy
; SoA which will miscompile it.
;
; RUN: igc_opt --opaque-pointers --ocl --platformNvl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=0 -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --ocl --platformNvl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.S = type { float, i32 }

; CHECK-LABEL: @test(
; The legacy transposed layout would scale offsets by simdSize * sizeof(struct).
; CHECK-NOT:   mul i32 {{.*}}, 8
; CHECK:       [[P:%.*]] = getelementptr i8, ptr {{.*}}, i32 4
; CHECK:       store i32 %v, ptr [[P]]
; CHECK:       ret void

define spir_kernel void @test(i32 %v, ptr %privateBase) {
  %a = alloca %struct.S, align 4, !uniform !0
  %p = getelementptr i8, ptr %a, i32 4
  store i32 %v, ptr %p, align 4
  ret void
}

!igc.functions = !{!1}

!0 = !{i1 true}
!1 = !{ptr @test, !2}
!2 = !{!3, !4}
!3 = !{!"function_type", i32 0}
!4 = !{!"implicit_arg_desc", !5}
!5 = !{i32 13}
