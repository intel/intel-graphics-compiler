;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-serialize-metadata -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Verifies that ModuleMetaData::FuncMD[F].requiredSubGroupSize round-trips
; through the autogen MDNode serialization. The input embeds the field via
; the standard !IGCMetadata MD; igc-serialize-metadata re-emits it.
; ------------------------------------------------

define spir_kernel void @bar(i32 %a) {
entry:
  ret void
}

; CHECK-DAG: {!"requiredSubGroupSize", i32 16}

!IGCMetadata = !{!2}

!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @bar}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"requiredSubGroupSize", i32 16}
