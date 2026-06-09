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
!4 =  !{ !"implicit_arg_desc",  !6,  !7,  !8}
!6 =  !{i32 8}
!7 =  !{i32 9}
!8 =  !{i32 10}
!9 = !{!"argId", i32 8}
!10 = !{!"implicitArgInfoListVec[0]", !9}
!11 = !{!"argId", i32 9}
!12 = !{!"implicitArgInfoListVec[1]", !11}
!13 = !{!"argId", i32 10}
!14 = !{!"implicitArgInfoListVec[2]", !13}
!15 = !{!"implicitArgInfoList", !10, !12, !14}
!16 = !{!"FuncMDMap[0]", i32 (i32)* @foo}
!17 = !{!"FuncMDValue[0]", !15}
!18 = !{!"FuncMD", !16, !17}
!19 = !{!"ModuleMD", !18}
!IGCMetadata = !{!19}

; CHECK:         define i32 @foo(i32 %x, i16 %localIdX, i16 %localIdY, i16 %localIdZ)
; CHECK-NOT:    define i32 @foo(i32 %x)
