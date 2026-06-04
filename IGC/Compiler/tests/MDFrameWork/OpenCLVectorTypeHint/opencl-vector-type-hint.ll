;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-serialize-metadata -S < %s | FileCheck %s
; ------------------------------------------------
; Verifies that ModuleMetaData::FuncMD[F].vecTypeHint (the OpenCL vec_type_hint
; kernel attribute, stored as its ZEBinary string form) round-trips through the
; autogen MDNode serialization. The input embeds the field via the standard
; !IGCMetadata MD; igc-serialize-metadata re-emits it.
; ------------------------------------------------

define spir_kernel void @bar(i32 %a) {
entry:
  ret void
}

; CHECK-DAG: {!"vecTypeHint", !"uchar4"}

!IGCMetadata = !{!2}

!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @bar}
!5 = !{!"FuncMDValue[0]", !6, !7, !11, !12, !13}
!6 = !{!"localOffsets"}
!7 = !{!"workGroupWalkOrder", !8, !9, !10}
!8 = !{!"dim0", i32 0}
!9 = !{!"dim1", i32 1}
!10 = !{!"dim2", i32 2}
!11 = !{!"vecTypeHint", !"uchar4"}
!12 = !{!"funcArgs"}
!13 = !{!"functionType", !"KernelFunction"}
