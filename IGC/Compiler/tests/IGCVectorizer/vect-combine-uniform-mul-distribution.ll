;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vect-combine -dce -verify < %s | FileCheck %s
; RUN: igc_opt -S --opaque-pointers --igc-vect-combine --regkey=VectorizerAllowFMADMatching=0 -dce -verify < %s | FileCheck %s --check-prefix=CHECK-OFF
; RUN: igc_opt -S --opaque-pointers --igc-vect-combine -instcombine -verify < %s | FileCheck %s --check-prefix=CHECK-IC

; (a - b) * c with uniform b, c becomes a * c - b * c, which the emitter matches
; into a mad. %a is made non uniform by inserting the lane id.
; reassoc is dropped from the new fsub so InstCombine can't fold it back.

; CHECK-LABEL: define spir_kernel void @fsub_exp2(
; CHECK: %uniform_mul = fmul fast <16 x float> %b, %c
; CHECK-NEXT: %distributed_mul = fmul fast <16 x float> %a_lane, %c
; CHECK-NEXT: %distributed_fsub = fsub nnan ninf nsz arcp contract afn <16 x float> %distributed_mul, %uniform_mul
; CHECK-NEXT: call <16 x float> @llvm.exp2.v16f32(<16 x float> %distributed_fsub)
; CHECK-IC-LABEL: define spir_kernel void @fsub_exp2(
; CHECK-IC: %uniform_mul = fmul fast <16 x float> %b, %c
; CHECK-IC-NEXT: %distributed_mul = fmul fast <16 x float> %a_lane, %c
; CHECK-IC-NEXT: %distributed_fsub = fsub nnan ninf nsz arcp contract afn <16 x float> %distributed_mul, %uniform_mul
; CHECK-IC-NEXT: call <16 x float> @llvm.exp2.v16f32(<16 x float> %distributed_fsub)
; CHECK-OFF-LABEL: define spir_kernel void @fsub_exp2(
; CHECK-OFF-NOT: distributed
; CHECK-OFF: %sub = fsub fast <16 x float> %a_lane, %b
; CHECK-OFF: %mul = fmul fast <16 x float> %sub, %c
define spir_kernel void @fsub_exp2(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <16 x float> %a, float %lane_f, i32 0
  %sub = fsub fast <16 x float> %a_lane, %b
  %mul = fmul fast <16 x float> %sub, %c
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %mul)
  store <16 x float> %exp, ptr addrspace(1) %p
  ret void
}

; commuted fmul and fadd operands
; CHECK-LABEL: define spir_kernel void @fadd_commuted(
; CHECK: %uniform_mul = fmul fast <16 x float> %b, %c
; CHECK-NEXT: %distributed_mul = fmul fast <16 x float> %a_lane, %c
; CHECK-NEXT: %distributed_fadd = fadd nnan ninf nsz arcp contract afn <16 x float> %distributed_mul, %uniform_mul
; CHECK-NEXT: store <16 x float> %distributed_fadd
; CHECK-IC-LABEL: define spir_kernel void @fadd_commuted(
; CHECK-IC: %uniform_mul = fmul fast <16 x float> %b, %c
; CHECK-IC-NEXT: %distributed_mul = fmul fast <16 x float> %a_lane, %c
; CHECK-IC-NEXT: %distributed_fadd = fadd nnan ninf nsz arcp contract afn <16 x float> %distributed_mul, %uniform_mul
define spir_kernel void @fadd_commuted(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <16 x float> %a, float %lane_f, i32 0
  %add = fadd fast <16 x float> %b, %a_lane
  %mul = fmul fast <16 x float> %c, %add
  store <16 x float> %mul, ptr addrspace(1) %p
  ret void
}

; only the common flags are kept, reassoc, nsz and contract are required
; CHECK-LABEL: define spir_kernel void @flags_intersection(
; CHECK: %uniform_mul = fmul reassoc nsz contract <16 x float> %b, %c
; CHECK-NEXT: %distributed_mul = fmul reassoc nsz contract <16 x float> %a_lane, %c
; CHECK-NEXT: %distributed_fsub = fsub nsz contract <16 x float> %distributed_mul, %uniform_mul
define spir_kernel void @flags_intersection(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <16 x float> %a, float %lane_f, i32 0
  %sub = fsub reassoc nsz contract <16 x float> %a_lane, %b
  %mul = fmul fast <16 x float> %sub, %c
  store <16 x float> %mul, ptr addrspace(1) %p
  ret void
}

; constant factor
; CHECK-LABEL: define spir_kernel void @constant_factor(
; CHECK: %uniform_mul = fmul fast <4 x float> %b, {{.*}}2.000000e+00
; CHECK-NEXT: %distributed_mul = fmul fast <4 x float> %a_lane, {{.*}}2.000000e+00
; CHECK-NEXT: %distributed_fsub = fsub nnan ninf nsz arcp contract afn <4 x float> %distributed_mul, %uniform_mul
define spir_kernel void @constant_factor(<4 x float> %a, <4 x float> %b, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <4 x float> %a, float %lane_f, i32 0
  %sub = fsub fast <4 x float> %a_lane, %b
  %mul = fmul fast <4 x float> %sub, <float 2.0, float 2.0, float 2.0, float 2.0>
  store <4 x float> %mul, ptr addrspace(1) %p
  ret void
}

; ---------------------------------------------------------------------------
; Not distributed
; ---------------------------------------------------------------------------

; b - a needs a negated product
; CHECK-LABEL: define spir_kernel void @uniform_minus_nonuniform(
; CHECK-NOT: distributed
; CHECK: %sub = fsub fast <16 x float> %b, %a_lane
; CHECK: %mul = fmul fast <16 x float> %sub, %c
define spir_kernel void @uniform_minus_nonuniform(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <16 x float> %a, float %lane_f, i32 0
  %sub = fsub fast <16 x float> %b, %a_lane
  %mul = fmul fast <16 x float> %sub, %c
  store <16 x float> %mul, ptr addrspace(1) %p
  ret void
}

; the fsub has another user
; CHECK-LABEL: define spir_kernel void @fsub_multiple_uses(
; CHECK-NOT: distributed
; CHECK: %sub = fsub fast <16 x float> %a_lane, %b
; CHECK: %mul = fmul fast <16 x float> %sub, %c
define spir_kernel void @fsub_multiple_uses(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p, ptr addrspace(1) %q) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <16 x float> %a, float %lane_f, i32 0
  %sub = fsub fast <16 x float> %a_lane, %b
  %mul = fmul fast <16 x float> %sub, %c
  store <16 x float> %mul, ptr addrspace(1) %p
  store <16 x float> %sub, ptr addrspace(1) %q
  ret void
}

; non uniform factor
; CHECK-LABEL: define spir_kernel void @nonuniform_factor(
; CHECK-NOT: distributed
; CHECK: %sub = fsub fast <16 x float> %a_lane, %b
; CHECK: %mul = fmul fast <16 x float> %sub, %c_lane
define spir_kernel void @nonuniform_factor(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <16 x float> %a, float %lane_f, i32 0
  %c_lane = insertelement <16 x float> %c, float %lane_f, i32 1
  %sub = fsub fast <16 x float> %a_lane, %b
  %mul = fmul fast <16 x float> %sub, %c_lane
  store <16 x float> %mul, ptr addrspace(1) %p
  ret void
}

; both fsub operands are non uniform
; CHECK-LABEL: define spir_kernel void @nonuniform_subtrahend(
; CHECK-NOT: distributed
; CHECK: %sub = fsub fast <16 x float> %a_lane, %b_lane
; CHECK: %mul = fmul fast <16 x float> %sub, %c
define spir_kernel void @nonuniform_subtrahend(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <16 x float> %a, float %lane_f, i32 0
  %b_lane = insertelement <16 x float> %b, float %lane_f, i32 1
  %sub = fsub fast <16 x float> %a_lane, %b_lane
  %mul = fmul fast <16 x float> %sub, %c
  store <16 x float> %mul, ptr addrspace(1) %p
  ret void
}

; all uniform, nothing to gain
; CHECK-LABEL: define spir_kernel void @all_uniform(
; CHECK-NOT: distributed
; CHECK: %sub = fsub fast <16 x float> %a, %b
; CHECK: %mul = fmul fast <16 x float> %sub, %c
define spir_kernel void @all_uniform(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %sub = fsub fast <16 x float> %a, %b
  %mul = fmul fast <16 x float> %sub, %c
  store <16 x float> %mul, ptr addrspace(1) %p
  ret void
}

; no nsz on the fsub, a == b would turn -0 into +0 for negative c
; CHECK-LABEL: define spir_kernel void @missing_nsz(
; CHECK-NOT: distributed
; CHECK: %sub = fsub reassoc contract <16 x float> %a_lane, %b
; CHECK: %mul = fmul fast <16 x float> %sub, %c
define spir_kernel void @missing_nsz(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <16 x float> %a, float %lane_f, i32 0
  %sub = fsub reassoc contract <16 x float> %a_lane, %b
  %mul = fmul fast <16 x float> %sub, %c
  store <16 x float> %mul, ptr addrspace(1) %p
  ret void
}

; no reassoc on the fmul
; CHECK-LABEL: define spir_kernel void @missing_reassoc(
; CHECK-NOT: distributed
; CHECK: %sub = fsub fast <16 x float> %a_lane, %b
; CHECK: %mul = fmul nsz contract <16 x float> %sub, %c
define spir_kernel void @missing_reassoc(<16 x float> %a, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a_lane = insertelement <16 x float> %a, float %lane_f, i32 0
  %sub = fsub fast <16 x float> %a_lane, %b
  %mul = fmul nsz contract <16 x float> %sub, %c
  store <16 x float> %mul, ptr addrspace(1) %p
  ret void
}

; only float is handled
; CHECK-LABEL: define spir_kernel void @half_type(
; CHECK-NOT: distributed
; CHECK: %sub = fsub fast <16 x half> %a_lane, %b
; CHECK: %mul = fmul fast <16 x half> %sub, %c
define spir_kernel void @half_type(<16 x half> %a, <16 x half> %b, <16 x half> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to half
  %a_lane = insertelement <16 x half> %a, half %lane_f, i32 0
  %sub = fsub fast <16 x half> %a_lane, %b
  %mul = fmul fast <16 x half> %sub, %c
  store <16 x half> %mul, ptr addrspace(1) %p
  ret void
}

; a is a product, a * x - b is already a mad
; CHECK-LABEL: define spir_kernel void @nonuniform_is_fmul(
; CHECK-NOT: distributed
; CHECK: %sub = fsub fast <16 x float> %a, %b
; CHECK: %mul = fmul fast <16 x float> %sub, %c
define spir_kernel void @nonuniform_is_fmul(<16 x float> %x, <16 x float> %y, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %x_lane = insertelement <16 x float> %x, float %lane_f, i32 0
  %a = fmul fast <16 x float> %x_lane, %y
  %sub = fsub fast <16 x float> %a, %b
  %mul = fmul fast <16 x float> %sub, %c
  store <16 x float> %mul, ptr addrspace(1) %p
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <16 x float> @llvm.exp2.v16f32(<16 x float>)

attributes #0 = { convergent nounwind }

!igc.functions = !{!0, !3, !4, !5, !6, !7, !8, !9, !100, !101, !102, !103, !104}

!0 = !{ptr @fsub_exp2, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @fadd_commuted, !1}
!4 = !{ptr @flags_intersection, !1}
!5 = !{ptr @constant_factor, !1}
!6 = !{ptr @uniform_minus_nonuniform, !1}
!7 = !{ptr @fsub_multiple_uses, !1}
!8 = !{ptr @nonuniform_factor, !1}
!9 = !{ptr @nonuniform_subtrahend, !1}
!100 = !{ptr @all_uniform, !1}
!101 = !{ptr @missing_nsz, !1}
!102 = !{ptr @missing_reassoc, !1}
!103 = !{ptr @half_type, !1}
!104 = !{ptr @nonuniform_is_fmul, !1}

!10 = !{!"requiredSubGroupSize", i32 16}
!11 = !{!"FuncMDMap[0]", ptr @fsub_exp2}
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"FuncMDMap[1]", ptr @fadd_commuted}
!14 = !{!"FuncMDValue[1]", !10}
!15 = !{!"FuncMDMap[2]", ptr @flags_intersection}
!16 = !{!"FuncMDValue[2]", !10}
!17 = !{!"FuncMDMap[3]", ptr @constant_factor}
!18 = !{!"FuncMDValue[3]", !10}
!19 = !{!"FuncMDMap[4]", ptr @uniform_minus_nonuniform}
!20 = !{!"FuncMDValue[4]", !10}
!21 = !{!"FuncMDMap[5]", ptr @fsub_multiple_uses}
!22 = !{!"FuncMDValue[5]", !10}
!23 = !{!"FuncMDMap[6]", ptr @nonuniform_factor}
!24 = !{!"FuncMDValue[6]", !10}
!25 = !{!"FuncMDMap[7]", ptr @nonuniform_subtrahend}
!26 = !{!"FuncMDValue[7]", !10}
!27 = !{!"FuncMDMap[8]", ptr @all_uniform}
!28 = !{!"FuncMDValue[8]", !10}
!29 = !{!"FuncMDMap[9]", ptr @missing_nsz}
!30 = !{!"FuncMDValue[9]", !10}
!31 = !{!"FuncMDMap[10]", ptr @missing_reassoc}
!32 = !{!"FuncMDValue[10]", !10}
!33 = !{!"FuncMDMap[11]", ptr @half_type}
!34 = !{!"FuncMDValue[11]", !10}
!37 = !{!"FuncMDMap[12]", ptr @nonuniform_is_fmul}
!38 = !{!"FuncMDValue[12]", !10}
!35 = !{!"FuncMD", !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !37, !38}
!36 = !{!"ModuleMD", !35}
!IGCMetadata = !{!36}
