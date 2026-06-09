;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-wi-func-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_num_groups(i32 %dim)

define i32 @foo(i32 %dim, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %numGroups) nounwind {
  %id = call i32 @__builtin_IB_get_num_groups(i32 %dim)
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (i32, <8 x i32>, <8 x i32>, <3 x i32>)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!7 = !{!"argId", i32 0}
!8 = !{!"implicitArgInfoListVec[0]", !7}
!9 = !{!"argId", i32 1}
!10 = !{!"implicitArgInfoListVec[1]", !9}
!11 = !{!"argId", i32 4}
!12 = !{!"implicitArgInfoListVec[2]", !11}
!13 = !{!"implicitArgInfoList", !8, !10, !12}
!14 = !{!"FuncMDMap[0]", i32 (i32, <8 x i32>, <8 x i32>, <3 x i32>)* @foo}
!15 = !{!"FuncMDValue[0]", !13}
!16 = !{!"FuncMD", !14, !15}
!17 = !{!"ModuleMD", !16}
!IGCMetadata = !{!17}

; CHECK:         %numGroups1 = extractelement <3 x i32> %numGroups, i32 %dim

; CHECK-NOT:     call i32 @__builtin_IB_get_num_groups(i32 2)
