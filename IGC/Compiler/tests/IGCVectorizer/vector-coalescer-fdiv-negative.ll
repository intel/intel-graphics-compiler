;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer -dce < %s | FileCheck %s
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer --regkey=VectorizerAllowFDIV=0 -dce < %s | FileCheck %s --check-prefix=CHECK-OFF

; only uniform float fdivs that fit into SIMD16 are coalesced

; positive control, coalesced by default, kept as is with VectorizerAllowFDIV=0
; CHECK-LABEL: define spir_kernel void @fdiv_float(
; CHECK: fdiv fast <16 x float>
; CHECK-OFF-LABEL: define spir_kernel void @fdiv_float(
; CHECK-OFF-NOT: coalesced
; CHECK-OFF: fdiv fast <8 x float> %a, %b
; CHECK-OFF: fdiv fast <8 x float> %c, %d
; CHECK-OFF: ret void

define spir_kernel void @fdiv_float(<8 x float> %a, <8 x float> %b, <8 x float> %c, <8 x float> %d, ptr addrspace(1) %p0, ptr addrspace(1) %p1) #0 {
entry:
  %div0 = fdiv fast <8 x float> %a, %b
  %div1 = fdiv fast <8 x float> %c, %d
  store <8 x float> %div0, ptr addrspace(1) %p0
  store <8 x float> %div1, ptr addrspace(1) %p1
  ret void
}

; double fdivs are not supported by the emitter
; CHECK-LABEL: define spir_kernel void @fdiv_double(
; CHECK-NOT: coalesced
; CHECK: fdiv fast <4 x double> %a, %b
; CHECK: fdiv fast <4 x double> %c, %d
; CHECK: ret void

define spir_kernel void @fdiv_double(<4 x double> %a, <4 x double> %b, <4 x double> %c, <4 x double> %d, ptr addrspace(1) %p0, ptr addrspace(1) %p1) #0 {
entry:
  %div0 = fdiv fast <4 x double> %a, %b
  %div1 = fdiv fast <4 x double> %c, %d
  store <4 x double> %div0, ptr addrspace(1) %p0
  store <4 x double> %div1, ptr addrspace(1) %p1
  ret void
}

; half fdivs are not supported by the emitter
; CHECK-LABEL: define spir_kernel void @fdiv_half(
; CHECK-NOT: coalesced
; CHECK: fdiv fast <8 x half> %a, %b
; CHECK: fdiv fast <8 x half> %c, %d
; CHECK: ret void

define spir_kernel void @fdiv_half(<8 x half> %a, <8 x half> %b, <8 x half> %c, <8 x half> %d, ptr addrspace(1) %p0, ptr addrspace(1) %p1) #0 {
entry:
  %div0 = fdiv fast <8 x half> %a, %b
  %div1 = fdiv fast <8 x half> %c, %d
  store <8 x half> %div0, ptr addrspace(1) %p0
  store <8 x half> %div1, ptr addrspace(1) %p1
  ret void
}

; the second fdiv depends on the lane id, so it is not uniform
; CHECK-LABEL: define spir_kernel void @fdiv_nonuniform(
; CHECK-NOT: coalesced
; CHECK: fdiv fast <8 x float> %a, %b
; CHECK: fdiv fast <8 x float> %c, %d_lane
; CHECK: ret void

define spir_kernel void @fdiv_nonuniform(<8 x float> %a, <8 x float> %b, <8 x float> %c, <8 x float> %d, ptr addrspace(1) %p0, ptr addrspace(1) %p1) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %d_lane = insertelement <8 x float> %d, float %lane_f, i32 0
  %div0 = fdiv fast <8 x float> %a, %b
  %div1 = fdiv fast <8 x float> %c, %d_lane
  store <8 x float> %div0, ptr addrspace(1) %p0
  store <8 x float> %div1, ptr addrspace(1) %p1
  ret void
}

; 16 + 16 elements exceed SIMD16
; CHECK-LABEL: define spir_kernel void @fdiv_too_wide(
; CHECK-NOT: coalesced
; CHECK: fdiv fast <16 x float> %a, %b
; CHECK: fdiv fast <16 x float> %c, %d
; CHECK: ret void

define spir_kernel void @fdiv_too_wide(<16 x float> %a, <16 x float> %b, <16 x float> %c, <16 x float> %d, ptr addrspace(1) %p0, ptr addrspace(1) %p1) #0 {
entry:
  %div0 = fdiv fast <16 x float> %a, %b
  %div1 = fdiv fast <16 x float> %c, %d
  store <16 x float> %div0, ptr addrspace(1) %p0
  store <16 x float> %div1, ptr addrspace(1) %p1
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()

attributes #0 = { convergent nounwind }

!igc.functions = !{!0, !3, !4, !5, !6}

!0 = !{ptr @fdiv_float, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @fdiv_double, !1}
!4 = !{ptr @fdiv_half, !1}
!5 = !{ptr @fdiv_nonuniform, !1}
!6 = !{ptr @fdiv_too_wide, !1}

!10 = !{!"requiredSubGroupSize", i32 16}
!11 = !{!"FuncMDMap[0]", ptr @fdiv_float}
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"FuncMDMap[1]", ptr @fdiv_double}
!14 = !{!"FuncMDValue[1]", !10}
!15 = !{!"FuncMDMap[2]", ptr @fdiv_half}
!16 = !{!"FuncMDValue[2]", !10}
!17 = !{!"FuncMDMap[3]", ptr @fdiv_nonuniform}
!18 = !{!"FuncMDValue[3]", !10}
!19 = !{!"FuncMDMap[4]", ptr @fdiv_too_wide}
!20 = !{!"FuncMDValue[4]", !10}
!21 = !{!"FuncMD", !11, !12, !13, !14, !15, !16, !17, !18, !19, !20}
!22 = !{!"ModuleMD", !21}
!IGCMetadata = !{!22}
