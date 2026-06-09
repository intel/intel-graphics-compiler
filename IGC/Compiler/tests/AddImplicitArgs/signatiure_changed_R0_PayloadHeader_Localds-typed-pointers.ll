;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-add-implicit-args -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define i32 @foo(i32 %x) nounwind {
  ret i32 %x
}

!igc.functions = !{!0}
!0 =  !{i32 (i32)* @foo,  !1}
!1 =  !{ !2,  !3,  !4}
!2 =  !{ !"function_type", i32 0}
!3 =  !{ !"arg_desc"}
!4 =  !{ !"implicit_arg_desc",  !6,  !7,  !8,  !9,  !10}
!6 =  !{i32 0}
!7 =  !{i32 1}
!8 =  !{i32 8}
!9 =  !{i32 9}
!10 =  !{i32 10}
!11 = !{!"argId", i32 0}
!12 = !{!"implicitArgInfoListVec[0]", !11}
!13 = !{!"argId", i32 1}
!14 = !{!"implicitArgInfoListVec[1]", !13}
!15 = !{!"argId", i32 8}
!16 = !{!"implicitArgInfoListVec[2]", !15}
!17 = !{!"argId", i32 9}
!18 = !{!"implicitArgInfoListVec[3]", !17}
!19 = !{!"argId", i32 10}
!20 = !{!"implicitArgInfoListVec[4]", !19}
!21 = !{!"implicitArgInfoList", !12, !14, !16, !18, !20}
!22 = !{!"FuncMDMap[0]", i32 (i32)* @foo}
!23 = !{!"FuncMDValue[0]", !21}
!24 = !{!"FuncMD", !22, !23}
!25 = !{!"ModuleMD", !24}
!IGCMetadata = !{!25}

; CHECK:         define i32 @foo(i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ)
; CHECK-NOT:    define i32 @foo(i32 %x)
