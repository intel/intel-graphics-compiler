;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer -dce < %s 2>&1 | FileCheck %s

; The coalescer is free to put the two halves in either order. It picks the
; order in which more operands are adjacent lanes of one vector, so that a later
; instcombine can take those lanes straight from that vector. On a tie the
; program order is kept.

; CHECK-LABEL: @rotate_whole(
; CHECK: [[IN:%.*]] = shufflevector <8 x float> %lo, <8 x float> %hi,
; CHECK: [[WIDE:%.*]] = fsub <16 x float> [[IN]],
; CHECK: [[LO:%.*]] = shufflevector <16 x float> [[WIDE]], <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK: [[HI:%.*]] = shufflevector <16 x float> [[WIDE]], <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK: LSC2DBlockWrite.v8f32({{.*}}<8 x float> [[HI]])
; CHECK: LSC2DBlockWrite.v8f32({{.*}}<8 x float> [[LO]])

define spir_kernel void @rotate_whole(<16 x float> %w, <8 x float> %y) #0 {
entry:
  %hi = shufflevector <16 x float> %w, <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %lo = shufflevector <16 x float> %w, <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %a = fsub <8 x float> %hi, %y
  %b = fsub <8 x float> %lo, %y
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %a)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %b)
  ret void
}

; CHECK-LABEL: @rotate_partial(
; CHECK: [[IN:%.*]] = shufflevector <4 x float> %lo, <4 x float> %hi,
; CHECK: [[WIDE:%.*]] = fmul <8 x float> [[IN]],
; CHECK: [[LO:%.*]] = shufflevector <8 x float> [[WIDE]], <8 x float> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK: [[HI:%.*]] = shufflevector <8 x float> [[WIDE]], <8 x float> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
; CHECK: LSC2DBlockWrite.v4f32({{.*}}<4 x float> [[HI]])
; CHECK: LSC2DBlockWrite.v4f32({{.*}}<4 x float> [[LO]])

define spir_kernel void @rotate_partial(<16 x float> %w, <4 x float> %y) #0 {
entry:
  %hi = shufflevector <16 x float> %w, <16 x float> undef, <4 x i32> <i32 8, i32 9, i32 10, i32 11>
  %lo = shufflevector <16 x float> %w, <16 x float> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
  %a = fmul <4 x float> %hi, %y
  %b = fmul <4 x float> %lo, %y
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <4 x float> %a)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <4 x float> %b)
  ret void
}

; CHECK-LABEL: @rotate_insert_chain(
; CHECK: [[IN:%.*]] = shufflevector <8 x float> %lo, <8 x float> %hi,
; CHECK: fadd <16 x float> [[IN]],

define spir_kernel void @rotate_insert_chain(<16 x float> %w, <8 x float> %y) #0 {
entry:
  %hi_e0 = extractelement <16 x float> %w, i32 8
  %hi_i0 = insertelement <8 x float> undef, float %hi_e0, i32 0
  %hi_e1 = extractelement <16 x float> %w, i32 9
  %hi_i1 = insertelement <8 x float> %hi_i0, float %hi_e1, i32 1
  %hi_e2 = extractelement <16 x float> %w, i32 10
  %hi_i2 = insertelement <8 x float> %hi_i1, float %hi_e2, i32 2
  %hi_e3 = extractelement <16 x float> %w, i32 11
  %hi_i3 = insertelement <8 x float> %hi_i2, float %hi_e3, i32 3
  %hi_e4 = extractelement <16 x float> %w, i32 12
  %hi_i4 = insertelement <8 x float> %hi_i3, float %hi_e4, i32 4
  %hi_e5 = extractelement <16 x float> %w, i32 13
  %hi_i5 = insertelement <8 x float> %hi_i4, float %hi_e5, i32 5
  %hi_e6 = extractelement <16 x float> %w, i32 14
  %hi_i6 = insertelement <8 x float> %hi_i5, float %hi_e6, i32 6
  %hi_e7 = extractelement <16 x float> %w, i32 15
  %hi = insertelement <8 x float> %hi_i6, float %hi_e7, i32 7
  %lo_e0 = extractelement <16 x float> %w, i32 0
  %lo_i0 = insertelement <8 x float> undef, float %lo_e0, i32 0
  %lo_e1 = extractelement <16 x float> %w, i32 1
  %lo_i1 = insertelement <8 x float> %lo_i0, float %lo_e1, i32 1
  %lo_e2 = extractelement <16 x float> %w, i32 2
  %lo_i2 = insertelement <8 x float> %lo_i1, float %lo_e2, i32 2
  %lo_e3 = extractelement <16 x float> %w, i32 3
  %lo_i3 = insertelement <8 x float> %lo_i2, float %lo_e3, i32 3
  %lo_e4 = extractelement <16 x float> %w, i32 4
  %lo_i4 = insertelement <8 x float> %lo_i3, float %lo_e4, i32 4
  %lo_e5 = extractelement <16 x float> %w, i32 5
  %lo_i5 = insertelement <8 x float> %lo_i4, float %lo_e5, i32 5
  %lo_e6 = extractelement <16 x float> %w, i32 6
  %lo_i6 = insertelement <8 x float> %lo_i5, float %lo_e6, i32 6
  %lo_e7 = extractelement <16 x float> %w, i32 7
  %lo = insertelement <8 x float> %lo_i6, float %lo_e7, i32 7
  %a = fadd <8 x float> %hi, %y
  %b = fadd <8 x float> %lo, %y
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %a)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %b)
  ret void
}

; CHECK-LABEL: @tie_keeps_order(
; CHECK: [[IN0:%.*]] = shufflevector <8 x float> %hi, <8 x float> %lo,
; CHECK: [[IN1:%.*]] = shufflevector <8 x float> %vlo, <8 x float> %vhi,
; CHECK: fsub <16 x float> [[IN0]], [[IN1]]

define spir_kernel void @tie_keeps_order(<16 x float> %w, <16 x float> %v) #0 {
entry:
  %hi = shufflevector <16 x float> %w, <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %lo = shufflevector <16 x float> %w, <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %vlo = shufflevector <16 x float> %v, <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %vhi = shufflevector <16 x float> %v, <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %a = fsub <8 x float> %hi, %vlo
  %b = fsub <8 x float> %lo, %vhi
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %a)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %b)
  ret void
}

; CHECK-LABEL: @dynamic_index(
; CHECK: [[IN0:%.*]] = shufflevector <2 x float> %hi, <2 x float> %lo,
; CHECK: fadd <4 x float> [[IN0]],

define spir_kernel void @dynamic_index(<16 x float> %w, <2 x float> %y, i32 %idx) #0 {
entry:
  %hi_e0 = extractelement <16 x float> %w, i32 %idx
  %hi_e1 = extractelement <16 x float> %w, i32 9
  %hi_i0 = insertelement <2 x float> undef, float %hi_e0, i32 0
  %hi = insertelement <2 x float> %hi_i0, float %hi_e1, i32 1
  %lo = shufflevector <16 x float> %w, <16 x float> undef, <2 x i32> <i32 0, i32 1>
  %a = fadd <2 x float> %hi, %y
  %b = fadd <2 x float> %lo, %y
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <2 x float> %a)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <2 x float> %b)
  ret void
}

; CHECK-LABEL: @rotate_intrinsic(
; CHECK: [[IN:%.*]] = shufflevector <8 x float> %lo, <8 x float> %hi,
; CHECK: call <16 x float> @llvm.maxnum.v16f32(<16 x float> [[IN]],

define spir_kernel void @rotate_intrinsic(<16 x float> %w, <8 x float> %y) #0 {
entry:
  %hi = shufflevector <16 x float> %w, <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %lo = shufflevector <16 x float> %w, <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %a = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %hi, <8 x float> %y)
  %b = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %lo, <8 x float> %y)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %a)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %b)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v2f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <2 x float>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v4f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <4 x float>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x float>)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>)

attributes #0 = { convergent nounwind }

!igc.functions = !{!0, !1, !2, !3, !4, !5}
!0 = !{ptr @rotate_whole, !6}
!1 = !{ptr @rotate_partial, !6}
!2 = !{ptr @rotate_insert_chain, !6}
!3 = !{ptr @tie_keeps_order, !6}
!4 = !{ptr @dynamic_index, !6}
!5 = !{ptr @rotate_intrinsic, !6}
!6 = !{!7}
!7 = !{!"function_type", i32 0}
!8 = !{!"requiredSubGroupSize", i32 16}
!9 = !{!"FuncMDMap[0]", ptr @rotate_whole}
!10 = !{!"FuncMDValue[0]", !8}
!11 = !{!"FuncMDMap[1]", ptr @rotate_partial}
!12 = !{!"FuncMDValue[1]", !8}
!13 = !{!"FuncMDMap[2]", ptr @rotate_insert_chain}
!14 = !{!"FuncMDValue[2]", !8}
!15 = !{!"FuncMDMap[3]", ptr @tie_keeps_order}
!16 = !{!"FuncMDValue[3]", !8}
!17 = !{!"FuncMDMap[4]", ptr @dynamic_index}
!18 = !{!"FuncMDValue[4]", !8}
!19 = !{!"FuncMDMap[5]", ptr @rotate_intrinsic}
!20 = !{!"FuncMDValue[5]", !8}
!21 = !{!"FuncMD", !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20}
!22 = !{!"ModuleMD", !21}
!IGCMetadata = !{!22}
