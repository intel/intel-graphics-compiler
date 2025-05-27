;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This is to test ldstcombine's layout struct creation. LdStCombine has an
; optimization to make sure no duplicate named structs will be created within
; a module.


; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

; CHECK-LABEL: target datalayout
; CHECK:       %__StructSOALayout_ = type <{ <2 x i32>, <2 x i32> }>
; CHECK-NOT:   %__StructSOALayout_{{.*}} = type
; CHECK-NOT:   %__StructAOSLayout_{{.*}} = type
; CHECK    :   define spir_kernel void @test_st

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_st(<2 x i32> addrspace(1)* %d, <2 x i32> addrspace(1)* %s, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %r0.scalar17 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar17
  %localIdX2 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX2
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %s, i64 %conv.i.i.i
  %0 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx, i64 8, i1 true, <2 x i32> <i32 2, i32 3>)
  %add = add nuw nsw i64 %conv.i.i.i, 1
  %arrayidx1 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %s, i64 %add
  %1 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx1, i64 8, i1 true, <2 x i32> <i32 4, i32 5>)
  %arrayidx2 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %conv.i.i.i
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx2, <2 x i32> %0, i64 8, i1 true)
  %arrayidx4 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %add
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx4, <2 x i32> %1, i64 8, i1 true)
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_st1(<2 x i32> addrspace(1)* %d, <2 x i32> addrspace(1)* %s, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %r0.scalar17 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar17
  %localIdX2 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX2
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %s, i64 %conv.i.i.i
  %0 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx, i64 8, i1 true, <2 x i32> <i32 6, i32 7>)
  %add = add nuw nsw i64 %conv.i.i.i, 1
  %arrayidx1 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %s, i64 %add
  %1 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx1, i64 8, i1 true, <2 x i32> <i32 8, i32 9>)
  %arrayidx2 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %conv.i.i.i
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx2, <2 x i32> %0, i64 8, i1 true)
  %arrayidx4 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %add
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx4, <2 x i32> %1, i64 8, i1 true)
  ret void
}

; Function Attrs: nounwind readonly
declare <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)*, i64, i1, <2 x i32>) #1

declare void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)*, <2 x i32>, i64, i1)

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" "null-pointer-is-valid"="true" }
attributes #1 = { nounwind readonly }

!igc.functions = !{!330, !340}
!330 = !{void (<2 x i32> addrspace(1)*, <2 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16)* @test_st, !331}
!331 = !{!332, !333}
!332 = !{!"function_type", i32 0}
!333 = !{!"implicit_arg_desc", !334, !335, !336, !337, !338, !339}
!334 = !{i32 0}
!335 = !{i32 1}
!336 = !{i32 7}
!337 = !{i32 8}
!338 = !{i32 9}
!339 = !{i32 10}
!340 = !{void (<2 x i32> addrspace(1)*, <2 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16)* @test_st1, !331}
