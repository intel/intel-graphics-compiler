;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: split-file %s %t
; RUN: igc_opt -platformdg2 -igc-bif-flag-control-resolution -S %t/any.ll | FileCheck %s --check-prefix=FLUSH
; RUN: igc_opt -platformdg2 -igc-bif-flag-control-resolution -S %t/ftz.ll | FileCheck %s --check-prefix=FLUSH
; RUN: igc_opt -platformdg2 -igc-bif-flag-control-resolution -S %t/retain.ll | FileCheck %s --check-prefix=RETAIN
; RUN: igc_opt -platformdg2 --retain-denormals -igc-bif-flag-control-resolution -S %t/any.ll | FileCheck %s --check-prefix=RETAIN
; ------------------------------------------------
; The BiF FlushDenormals flag follows CompOptions::FloatDenormMode32.
; FLOAT_DENORM_ANY (the CompOptions default) is treated like
; FLOAT_DENORM_FLUSH_TO_ZERO; only FLOAT_DENORM_RETAIN clears the flag.
; ------------------------------------------------

; FLUSH: @__bif_flag_FlushDenormals = addrspace(2) constant i8 1
; RETAIN: @__bif_flag_FlushDenormals = addrspace(2) constant i8 0

;--- any.ll
@__bif_flag_FlushDenormals = external addrspace(2) constant i8

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FloatDenormMode32", !"FLOAT_DENORM_ANY"}

;--- ftz.ll
@__bif_flag_FlushDenormals = external addrspace(2) constant i8

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FloatDenormMode32", !"FLOAT_DENORM_FLUSH_TO_ZERO"}

;--- retain.ll
@__bif_flag_FlushDenormals = external addrspace(2) constant i8

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FloatDenormMode32", !"FLOAT_DENORM_RETAIN"}
