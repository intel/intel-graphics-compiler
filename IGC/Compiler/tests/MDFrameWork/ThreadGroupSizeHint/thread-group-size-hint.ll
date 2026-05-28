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
; Verifies that ModuleMetaData::FuncMD[F].threadGroupSizeHint round-trips
; through the autogen MDNode serialization. The input embeds the field via
; the standard !IGCMetadata MD; igc-serialize-metadata re-emits it.
; ------------------------------------------------

define spir_kernel void @bar(i32 %a) {
entry:
  ret void
}

; CHECK-DAG: {!"threadGroupSizeHint", [[D0:![0-9]+]], [[D1:![0-9]+]], [[D2:![0-9]+]]}
; CHECK-DAG: [[D0]] = !{!"dim0", i32 4}
; CHECK-DAG: [[D1]] = !{!"dim1", i32 2}
; CHECK-DAG: [[D2]] = !{!"dim2", i32 1}

!IGCMetadata = !{!2}

!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @bar}
!5 = !{!"FuncMDValue[0]", !6, !7, !11, !15, !16}
!6 = !{!"localOffsets"}
!7 = !{!"workGroupWalkOrder", !8, !9, !10}
!8 = !{!"dim0", i32 0}
!9 = !{!"dim1", i32 1}
!10 = !{!"dim2", i32 2}
!11 = !{!"threadGroupSizeHint", !12, !13, !14}
!12 = !{!"dim0", i32 4}
!13 = !{!"dim1", i32 2}
!14 = !{!"dim2", i32 1}
!15 = !{!"funcArgs"}
!16 = !{!"functionType", !"KernelFunction"}
