;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerEnableVirtualSeeds=1 -dce --platformbmg < %s 2>&1 | FileCheck %s

; Virtual seed of 2 lanes from an <8 x float> source is rejected the same way
; as a natural one, no insertelement/extractelement round trip is left.

; CHECK-LABEL: @k(
; CHECK:         %bc0 = bitcast float %e0 to i32
; CHECK:         %bc1 = bitcast float %e1 to i32
; CHECK-NOT:     insertelement <2 x i32>
; CHECK:         call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g0, i32 %bc0, i64 4, i1 %pred)
; CHECK:         call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g1, i32 %bc1, i64 4, i1 %pred)

@g0 = external addrspace(1) global i32
@g1 = external addrspace(1) global i32

define spir_kernel void @k(i1 %pred, float %x, float %y, <8 x float> %wide, ptr addrspace(1) %dst) {
entry:
  ; natural seed of width 2 -> PreferredVectorSize = 2
  %n0 = insertelement <2 x float> zeroinitializer, float %x, i64 0
  %n1 = insertelement <2 x float> %n0, float %y, i64 1
  store <2 x float> %n1, ptr addrspace(1) %dst, align 8

  %src = fadd <8 x float> %wide, %wide
  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %bc0 = bitcast float %e0 to i32
  %bc1 = bitcast float %e1 to i32
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g0, i32 %bc0, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g1, i32 %bc1, i64 4, i1 %pred)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1), i32, i64, i1)

!igc.functions = !{!0}
!0 = !{ptr @k, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @k}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
