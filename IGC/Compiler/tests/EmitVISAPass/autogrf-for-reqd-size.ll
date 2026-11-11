;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; Kernels with reqd_work_group_size are capped at the largest GRF number whose
; HW threads per EU still fit the work group (full size at 8 threads for SIMD16,
; 4 for SIMD32). NVL-P raises the heuristic 256 ceiling to 512, so only
; the HW restrictions show.
;
; Expected -maxGRFNum per reqd_work_group_size (- = none, * = ceiling, not a cap):
;
; SIMD16                    | 1024 | 768 | 640 | 512  | 384  | 256
; PTL                       | 128  | 160 | 192 | -    | -    | -
; NVL-P, Eff64b, ceil. 512  | 128  | 160 | 192 | 256  | 256  | 512*
; CRI, with/without Eff64b  | 192  | 192 | 192 | -    | -    | -
;
; SIMD32                    | 1024 | 768 | 512
; PTL                       | -    | -   | -
; NVL-P, Eff64b, ceil. 512  | 256  | 256 | 512*
; CRI, Eff64b               | -    | -   | -
;
; A work group above the maximum fits no mode, so it takes the smallest GRF number
; (96 for PTL and NVL, 192 for CRI) instead of no cap, which would let vISA pick
; the largest mode.

; RUN: igc_opt --opaque-pointers %s -S --platformPtl -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole=1 \
; RUN:   | FileCheck %s --check-prefixes=CHECK,XE3,PTL

; RUN: igc_opt --opaque-pointers %s -S --platformNvl -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole=1 \
; RUN:   -regkey EnableEfficient64b -regkey ForceVRTGRFCeiling=512 | FileCheck %s --check-prefixes=CHECK,XE3,E64,NVL512



; RUN: igc_opt --opaque-pointers %s -S --platformCri -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole=1 \
; RUN:   | FileCheck %s --check-prefixes=CHECK,CRI

; RUN: igc_opt --opaque-pointers %s -S --platformCri -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole=1 \
; RUN:   -regkey EnableEfficient64b | FileCheck %s --check-prefixes=CHECK,CRI,E64

; RUN: igc_opt --opaque-pointers %s -S --platformPtl -simd-mode 32 -igc-emit-visa -regkey DumpVISAASMToConsole=1 \
; RUN:   | FileCheck %s --check-prefixes=S32,S32-NOCAP

; RUN: igc_opt --opaque-pointers %s -S --platformNvl -simd-mode 32 -igc-emit-visa -regkey DumpVISAASMToConsole=1 \
; RUN:   -regkey EnableEfficient64b -regkey ForceVRTGRFCeiling=512 | FileCheck %s --check-prefixes=S32,S32-NVL512



; RUN: igc_opt --opaque-pointers %s -S --platformCri -simd-mode 32 -igc-emit-visa -regkey DumpVISAASMToConsole=1 \
; RUN:   -regkey EnableEfficient64b | FileCheck %s --check-prefixes=S32,S32-NOCAP

; CHECK-LABEL: .kernel "test1024"
; CHECK:       -autoGRFSelection
; XE3-SAME:    -maxGRFNum 128
; CRI-SAME:    -maxGRFNum 192
; E64-SAME:    -enableEfficient64b

; S32-LABEL:      .kernel "test1024"
; S32:            SimdSize=32
; S32-NOCAP-NOT:  -maxGRFNum
; S32-NVL512:     -maxGRFNum 256

; CHECK-LABEL: .kernel "test768"
; XE3:         -maxGRFNum 160
; CRI:         -maxGRFNum 192

; S32-LABEL:      .kernel "test768"
; S32:            SimdSize=32
; S32-NOCAP-NOT:  -maxGRFNum
; S32-NVL512:     -maxGRFNum 256

; CHECK-LABEL: .kernel "test640"
; CHECK:       -maxGRFNum 192

; S32-LABEL:   .kernel "test640"

; CHECK-LABEL: .kernel "test512"
; PTL-NOT:     -maxGRFNum
; NVL512:      -maxGRFNum 256
; CRI-NOT:     -maxGRFNum

; S32-LABEL:      .kernel "test512"
; S32:            SimdSize=32
; S32-NOCAP-NOT:  -maxGRFNum
; S32-NVL512:     -maxGRFNum 512

; S32-LABEL:   .kernel "test384"

; CHECK-LABEL: .kernel "test384"
; PTL-NOT:     -maxGRFNum
; NVL512:      -maxGRFNum 256
; CRI-NOT:     -maxGRFNum

; CHECK-LABEL: .kernel "test256"
; PTL-NOT:     -maxGRFNum
; NVL512:      -maxGRFNum 512
; CRI-NOT:     -maxGRFNum

; CHECK-LABEL: .kernel "test2048"
; XE3:         -maxGRFNum 96
; CRI:         -maxGRFNum 192

; S32-LABEL:   .kernel "test2048"
; S32:         -maxGRFNum {{96|192}}

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test1024(ptr addrspace(1) %output, ptr addrspace(1) %eltwise0_input0, ptr addrspace(1) %eltwise1_input0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ret void
}

define spir_kernel void @test768(ptr addrspace(1) %output, ptr addrspace(1) %eltwise0_input0, ptr addrspace(1) %eltwise1_input0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ret void
}

define spir_kernel void @test640(ptr addrspace(1) %output, ptr addrspace(1) %eltwise0_input0, ptr addrspace(1) %eltwise1_input0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ret void
}

define spir_kernel void @test512(ptr addrspace(1) %output, ptr addrspace(1) %eltwise0_input0, ptr addrspace(1) %eltwise1_input0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ret void
}

define spir_kernel void @test384(ptr addrspace(1) %output, ptr addrspace(1) %eltwise0_input0, ptr addrspace(1) %eltwise1_input0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ret void
}

define spir_kernel void @test256(ptr addrspace(1) %output, ptr addrspace(1) %eltwise0_input0, ptr addrspace(1) %eltwise1_input0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ret void
}

define spir_kernel void @test2048(ptr addrspace(1) %output, ptr addrspace(1) %eltwise0_input0, ptr addrspace(1) %eltwise1_input0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ret void
}

!igc.functions = !{!0, !3, !4, !5, !6, !7, !8}
!IGCMetadata = !{!100}

!0 = !{ptr @test1024, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @test768, !1}
!4 = !{ptr @test640, !1}
!5 = !{ptr @test512, !1}
!6 = !{ptr @test384, !1}
!7 = !{ptr @test256, !1}
!8 = !{ptr @test2048, !1}

!100 = !{!"ModuleMD", !101}
!101 = !{!"FuncMD", !210, !211, !220, !221, !230, !231, !240, !241, !250, !251, !260, !261, !270, !271}
!106 = !{!"dim1", i32 1}
!107 = !{!"dim2", i32 1}
!210 = !{!"FuncMDMap[0]", ptr @test1024}
!211 = !{!"FuncMDValue[0]", !212, !136}
!212 = !{!"threadGroupSize", !213, !106, !107}
!213 = !{!"dim0", i32 1024}
!220 = !{!"FuncMDMap[1]", ptr @test768}
!221 = !{!"FuncMDValue[1]", !222, !136}
!222 = !{!"threadGroupSize", !223, !106, !107}
!223 = !{!"dim0", i32 768}
!230 = !{!"FuncMDMap[2]", ptr @test640}
!231 = !{!"FuncMDValue[2]", !232, !136}
!232 = !{!"threadGroupSize", !233, !106, !107}
!233 = !{!"dim0", i32 640}
!240 = !{!"FuncMDMap[3]", ptr @test512}
!241 = !{!"FuncMDValue[3]", !242, !136}
!242 = !{!"threadGroupSize", !243, !106, !107}
!243 = !{!"dim0", i32 512}
!250 = !{!"FuncMDMap[4]", ptr @test384}
!251 = !{!"FuncMDValue[4]", !252, !136}
!252 = !{!"threadGroupSize", !253, !106, !107}
!253 = !{!"dim0", i32 384}
!260 = !{!"FuncMDMap[5]", ptr @test256}
!261 = !{!"FuncMDValue[5]", !262, !136}
!262 = !{!"threadGroupSize", !263, !106, !107}
!263 = !{!"dim0", i32 256}
!270 = !{!"FuncMDMap[6]", ptr @test2048}
!271 = !{!"FuncMDValue[6]", !272, !136}
!272 = !{!"threadGroupSize", !273, !106, !107}
!273 = !{!"dim0", i32 2048}

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
