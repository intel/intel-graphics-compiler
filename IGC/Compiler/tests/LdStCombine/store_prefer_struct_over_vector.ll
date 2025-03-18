;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; LdStCombine can use either vector and struct as the final stored value.
; This test is test that a struct is prefered over a vector to avoid additional
; bitcast instructions.

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

; CHECK-LABEL: target datalayout
; CHECK:       %__StructSOALayout_ = type <{ float, i32 }>
; CHECK-LABEL: define spir_kernel void @test_st
; CHECK:       %[[TMP:.*]] = call <2 x i32> @llvm.genx.GenISA.bitcastfromstruct.v2i32.__StructSOALayout_
; CHECK:       store <2 x i32> %[[TMP]]
; CHECK:       ret

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.dw1_t = type { float, i32 }

; Function Attrs: convergent nounwind
define spir_kernel void @test_st(i8 addrspace(1)* %d, float addrspace(1)* %sf, i32 addrspace(1)* %si, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %conv.i.i = zext i16 %localIdX to i64
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %sf, i64 %conv.i.i
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %add = fadd float %0, 1.000000e+00
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %conv.i.i
  %1 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  %add2 = add nsw i32 %1, 1
  %conv = sitofp i32 %add2 to float
  %conv5 = fptosi float %conv to i32
  %2 = bitcast i8 addrspace(1)* %d to %struct.dw1_t addrspace(1)*
  %x.sroa.0.0..sroa_idx = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %2, i64 %conv.i.i, i32 0
  store float %add, float addrspace(1)* %x.sroa.0.0..sroa_idx, align 4
  %x.sroa.4.0..sroa_idx2 = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %2, i64 %conv.i.i, i32 1
  store i32 %conv5, i32 addrspace(1)* %x.sroa.4.0..sroa_idx2, align 4
  ret void
}

attributes #0 = { convergent nounwind "null-pointer-is-valid"="true" }

!igc.functions = !{!334}
!334 = !{void (i8 addrspace(1)*, float addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16)* @test_st, !335}
!335 = !{!336, !337}
!336 = !{!"function_type", i32 0}
!337 = !{!"implicit_arg_desc", !338, !339, !340, !341, !342}
!338 = !{i32 0}
!339 = !{i32 1}
!340 = !{i32 8}
!341 = !{i32 9}
!342 = !{i32 10}
