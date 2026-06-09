;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-wi-func-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_local_id_z()

define i32 @foo(i32 %dim, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) nounwind {
  %id = call i32 @__builtin_IB_get_local_id_z()
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (i32, <8 x i32>, <8 x i32>, i16, i16, i16)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!9 = !{!"argId", i32 0}
!10 = !{!"implicitArgInfoListVec[0]", !9}
!11 = !{!"argId", i32 1}
!12 = !{!"implicitArgInfoListVec[1]", !11}
!13 = !{!"argId", i32 8}
!14 = !{!"implicitArgInfoListVec[2]", !13}
!15 = !{!"argId", i32 9}
!16 = !{!"implicitArgInfoListVec[3]", !15}
!17 = !{!"argId", i32 10}
!18 = !{!"implicitArgInfoListVec[4]", !17}
!19 = !{!"implicitArgInfoList", !10, !12, !14, !16, !18}
!20 = !{!"FuncMDMap[0]", i32 (i32, <8 x i32>, <8 x i32>, i16, i16, i16)* @foo}
!21 = !{!"FuncMDValue[0]", !19}
!22 = !{!"FuncMD", !20, !21}
!23 = !{!"ModuleMD", !22}
!IGCMetadata = !{!23}

; CHECK:         ret i32 %localIdZ

; CHECK-NOT:     call i32 @__builtin_IB_get_local_id_z()
