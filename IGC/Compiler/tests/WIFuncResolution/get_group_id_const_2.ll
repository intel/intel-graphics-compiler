;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-wi-func-resolution -instsimplify -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_group_id(i32 %dim)

define i32 @foo(i32 %dim, <8 x i32> %r0, <8 x i32> %payloadHeader) nounwind {
  %id = call i32 @__builtin_IB_get_group_id(i32 2)
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (i32, <8 x i32>, <8 x i32>)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!6 = !{!"argId", i32 0}
!7 = !{!"implicitArgInfoListVec[0]", !6}
!8 = !{!"argId", i32 1}
!9 = !{!"implicitArgInfoListVec[1]", !8}
!10 = !{!"implicitArgInfoList", !7, !9}
!11 = !{!"FuncMDMap[0]", i32 (i32, <8 x i32>, <8 x i32>)* @foo}
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"FuncMD", !11, !12}
!14 = !{!"ModuleMD", !13}
!IGCMetadata = !{!14}

; CHECK:         %groupId = extractelement <8 x i32> %r0, i32 7

; CHECK-NOT:     call i32 @__builtin_IB_get_group_id(i32 2)
