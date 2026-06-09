;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S --platformPtl -simd-mode 32 -igc-emit-visa -regkey DumpVISAASMToConsole=1 | FileCheck %s

; CHECK-LABEL: .kernel "test"
; CHECK:       _main_0:
; CHECK: -autoGRFSelection
; CHECK-NOT: -maxGRFNum 128

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(ptr addrspace(1) %output, ptr addrspace(1) %eltwise0_input0, ptr addrspace(1) %eltwise1_input0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!100}

!0 = !{ptr @test, !1}
!1 = !{!2}
!100 = !{!"ModuleMD", !101}
!101 = !{!"FuncMD", !102, !103}
!102 = !{!"FuncMDMap[0]", ptr @test}
!103 = !{!"FuncMDValue[0]", !104, !136}
!104 = !{!"threadGroupSize", !105, !106, !107}
!105 = !{!"dim0", i32 1024}
!106 = !{!"dim1", i32 1}
!107 = !{!"dim2", i32 1}
!2 = !{!"function_type", i32 0}
!108 = !{!"argId", i32 0}
!109 = !{!"implicitArgInfoListVec[0]", !108}
!110 = !{!"argId", i32 2}
!111 = !{!"implicitArgInfoListVec[1]", !110}
!112 = !{!"argId", i32 6}
!113 = !{!"implicitArgInfoListVec[2]", !112}
!114 = !{!"argId", i32 7}
!115 = !{!"implicitArgInfoListVec[3]", !114}
!116 = !{!"argId", i32 8}
!117 = !{!"implicitArgInfoListVec[4]", !116}
!118 = !{!"argId", i32 9}
!119 = !{!"implicitArgInfoListVec[5]", !118}
!120 = !{!"argId", i32 10}
!121 = !{!"implicitArgInfoListVec[6]", !120}
!122 = !{!"argId", i32 13}
!123 = !{!"implicitArgInfoListVec[7]", !122}
!124 = !{!"argId", i32 15}
!125 = !{!"explicitArgNum", i32 0}
!126 = !{!"implicitArgInfoListVec[8]", !124, !125}
!127 = !{!"argId", i32 15}
!128 = !{!"explicitArgNum", i32 1}
!129 = !{!"implicitArgInfoListVec[9]", !127, !128}
!130 = !{!"argId", i32 15}
!131 = !{!"explicitArgNum", i32 2}
!132 = !{!"implicitArgInfoListVec[10]", !130, !131}
!133 = !{!"argId", i32 15}
!134 = !{!"explicitArgNum", i32 3}
!135 = !{!"implicitArgInfoListVec[11]", !133, !134}
!136 = !{!"implicitArgInfoList", !109, !111, !113, !115, !117, !119, !121, !123, !126, !129, !132, !135}
