;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -platformbmg -opaque-pointers -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s --implicit-check-not=.sat
; RUN: igc_opt -platformdg2 -opaque-pointers -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s --implicit-check-not=.sat
;
; Keep bfloat clamps as selects: saturation is unsupported on bfloat destinations.

define spir_kernel void @test_sat(bfloat %b, ptr %out) {
; CHECK-LABEL: .kernel "test_sat"
; CHECK: sel
; CHECK: sel
; CHECK: ret
  %c0 = fcmp ogt bfloat %b, 0xR0000
  %m0 = select i1 %c0, bfloat %b, bfloat 0xR0000
  %c1 = fcmp olt bfloat %m0, 0xR3F80
  %m1 = select i1 %c1, bfloat %m0, bfloat 0xR3F80
  store bfloat %m1, ptr %out
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!10}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test_sat}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{ptr @test_sat, !11}
!11 = !{!12}
!12 = !{!"function_type", i32 0}
