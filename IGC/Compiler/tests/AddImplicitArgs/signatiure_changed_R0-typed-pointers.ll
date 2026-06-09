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
!4 =  !{ !"implicit_arg_desc",  !6}
!6 =  !{i32 0}
!7 = !{!"argId", i32 0}
!8 = !{!"implicitArgInfoListVec[0]", !7}
!9 = !{!"implicitArgInfoList", !8}
!10 = !{!"FuncMDMap[0]", i32 (i32)* @foo}
!11 = !{!"FuncMDValue[0]", !9}
!12 = !{!"FuncMD", !10, !11}
!13 = !{!"ModuleMD", !12}
!IGCMetadata = !{!13}

; CHECK:         define i32 @foo(i32 %x, <8 x i32> %r0)
; CHECK-NOT:    define i32 @foo(i32 %x)
