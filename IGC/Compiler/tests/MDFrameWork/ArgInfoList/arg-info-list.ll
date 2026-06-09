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
; Verifies that ModuleMetaData::FuncMD[F].argInfoList and .implicitArgInfoList
; round-trip through the autogen MDNode serialization. Each ArgInfoMD carries the
; five POD fields (argId/explicitArgNum/structArgOffset/imgAccessFloatCoords/
; imgAccessIntCoords); -1 is the "unset" sentinel. The input embeds the lists via
; the standard !IGCMetadata MD; igc-serialize-metadata re-emits them.
; ------------------------------------------------

define spir_kernel void @test(i32 %a) {
entry:
  ret void
}

; CHECK-DAG: {!"argInfoList", [[EARG:![0-9]+]]}
; CHECK-DAG: [[EARG]] = !{!"argInfoListVec[0]", [[EID:![0-9]+]], [[EEXP:![0-9]+]], [[EOFF:![0-9]+]], [[EFLT:![0-9]+]], [[EINT:![0-9]+]]}
; CHECK-DAG: [[EID]] = !{!"argId", i32 -1}
; CHECK-DAG: [[EEXP]] = !{!"explicitArgNum", i32 0}
; CHECK-DAG: [[EOFF]] = !{!"structArgOffset", i32 4}
; CHECK-DAG: [[EFLT]] = !{!"imgAccessFloatCoords", i32 1}
; CHECK-DAG: [[EINT]] = !{!"imgAccessIntCoords", i32 0}

; CHECK-DAG: {!"implicitArgInfoList", [[IARG:![0-9]+]]}
; CHECK-DAG: [[IARG]] = !{!"implicitArgInfoListVec[0]", [[IID:![0-9]+]], [[IEXP:![0-9]+]], [[IOFF:![0-9]+]], [[IFLT:![0-9]+]], [[IINT:![0-9]+]]}
; CHECK-DAG: [[IID]] = !{!"argId", i32 15}
; CHECK-DAG: [[IEXP]] = !{!"explicitArgNum", i32 2}
; CHECK-DAG: [[IOFF]] = !{!"structArgOffset", i32 -1}
; CHECK-DAG: [[IFLT]] = !{!"imgAccessFloatCoords", i32 -1}
; CHECK-DAG: [[IINT]] = !{!"imgAccessIntCoords", i32 -1}

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test}
!3 = !{!"FuncMDValue[0]", !4, !11}
!4 = !{!"argInfoList", !5}
!5 = !{!"argInfoListVec[0]", !6, !7, !8, !9, !10}
!6 = !{!"argId", i32 -1}
!7 = !{!"explicitArgNum", i32 0}
!8 = !{!"structArgOffset", i32 4}
!9 = !{!"imgAccessFloatCoords", i32 1}
!10 = !{!"imgAccessIntCoords", i32 0}
!11 = !{!"implicitArgInfoList", !12}
!12 = !{!"implicitArgInfoListVec[0]", !13, !14, !15, !16, !17}
!13 = !{!"argId", i32 15}
!14 = !{!"explicitArgNum", i32 2}
!15 = !{!"structArgOffset", i32 -1}
!16 = !{!"imgAccessFloatCoords", i32 -1}
!17 = !{!"imgAccessIntCoords", i32 -1}
