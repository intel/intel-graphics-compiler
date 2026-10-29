;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: split-file %s %t
; RUN: igc_opt --opaque-pointers -igc-serialize-metadata -S %t/explicit.ll 2>&1 | FileCheck %s --check-prefix=EXPLICIT
; RUN: igc_opt --opaque-pointers -igc-serialize-metadata -S %t/default.ll 2>&1 | FileCheck %s --check-prefix=DEFAULT
; ------------------------------------------------
; Verifies that CompOptions::FloatDenormMode* round-trip through the autogen
; MDNode serialization, including FLOAT_DENORM_ANY, and that FLOAT_DENORM_ANY
; is the default when the module carries no denorm modes.
; ------------------------------------------------

; EXPLICIT-DAG: !{!"FloatDenormMode16", !"FLOAT_DENORM_ANY"}
; EXPLICIT-DAG: !{!"FloatDenormMode32", !"FLOAT_DENORM_FLUSH_TO_ZERO"}
; EXPLICIT-DAG: !{!"FloatDenormMode64", !"FLOAT_DENORM_RETAIN"}
; EXPLICIT-DAG: !{!"FloatDenormModeBFTF", !"FLOAT_DENORM_ANY"}

; DEFAULT-DAG: !{!"FloatDenormMode16", !"FLOAT_DENORM_ANY"}
; DEFAULT-DAG: !{!"FloatDenormMode32", !"FLOAT_DENORM_ANY"}
; DEFAULT-DAG: !{!"FloatDenormMode64", !"FLOAT_DENORM_ANY"}
; DEFAULT-DAG: !{!"FloatDenormModeBFTF", !"FLOAT_DENORM_ANY"}

;--- explicit.ll
define spir_kernel void @foo() {
entry:
  ret void
}

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3, !4}
!2 = !{!"FloatDenormMode16", !"FLOAT_DENORM_ANY"}
!3 = !{!"FloatDenormMode32", !"FLOAT_DENORM_FLUSH_TO_ZERO"}
!4 = !{!"FloatDenormMode64", !"FLOAT_DENORM_RETAIN"}

;--- default.ll
define spir_kernel void @foo() {
entry:
  ret void
}
