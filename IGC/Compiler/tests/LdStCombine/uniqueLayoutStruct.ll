;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This is to test ldstcombine's layout struct creation. LdStCombine has an
; optimization to make sure no duplicate named structs will be created within
; a module.


; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S -igc-ldstcombine -regkey=EnableLdStCombine=1 \
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
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %arrayidx, align 8
  %add = add nuw nsw i64 %conv.i.i.i, 1
  %arrayidx1 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %s, i64 %add
  %1 = load <2 x i32>, <2 x i32> addrspace(1)* %arrayidx1, align 8
  %arrayidx2 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %conv.i.i.i
  store <2 x i32> %0, <2 x i32> addrspace(1)* %arrayidx2, align 8
  %arrayidx4 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %add
  store <2 x i32> %1, <2 x i32> addrspace(1)* %arrayidx4, align 8
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
  %0 = load <2 x i32>, <2 x i32> addrspace(1)* %arrayidx, align 8
  %add = add nuw nsw i64 %conv.i.i.i, 1
  %arrayidx1 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %s, i64 %add
  %1 = load <2 x i32>, <2 x i32> addrspace(1)* %arrayidx1, align 8
  %arrayidx2 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %conv.i.i.i
  store <2 x i32> %0, <2 x i32> addrspace(1)* %arrayidx2, align 8
  %arrayidx4 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %add
  store <2 x i32> %1, <2 x i32> addrspace(1)* %arrayidx4, align 8
  ret void
}

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" "null-pointer-is-valid"="true" }

!igc.functions = !{!330, !340}
!330 = !{void (<2 x i32> addrspace(1)*, <2 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16)* @test_st, !331}
!331 = !{!332}
!332 = !{!"function_type", i32 0}
!340 = !{void (<2 x i32> addrspace(1)*, <2 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16)* @test_st1, !331}
!341 = !{!"argId", i32 0}
!342 = !{!"implicitArgInfoListVec[0]", !341}
!343 = !{!"argId", i32 1}
!344 = !{!"implicitArgInfoListVec[1]", !343}
!345 = !{!"argId", i32 7}
!346 = !{!"implicitArgInfoListVec[2]", !345}
!347 = !{!"argId", i32 8}
!348 = !{!"implicitArgInfoListVec[3]", !347}
!349 = !{!"argId", i32 9}
!350 = !{!"implicitArgInfoListVec[4]", !349}
!351 = !{!"argId", i32 10}
!352 = !{!"implicitArgInfoListVec[5]", !351}
!353 = !{!"implicitArgInfoList", !342, !344, !346, !348, !350, !352}
!354 = !{!"argId", i32 0}
!355 = !{!"implicitArgInfoListVec[0]", !354}
!356 = !{!"argId", i32 1}
!357 = !{!"implicitArgInfoListVec[1]", !356}
!358 = !{!"argId", i32 7}
!359 = !{!"implicitArgInfoListVec[2]", !358}
!360 = !{!"argId", i32 8}
!361 = !{!"implicitArgInfoListVec[3]", !360}
!362 = !{!"argId", i32 9}
!363 = !{!"implicitArgInfoListVec[4]", !362}
!364 = !{!"argId", i32 10}
!365 = !{!"implicitArgInfoListVec[5]", !364}
!366 = !{!"implicitArgInfoList", !355, !357, !359, !361, !363, !365}
!367 = !{!"FuncMDMap[0]", void (<2 x i32> addrspace(1)*, <2 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16)* @test_st}
!368 = !{!"FuncMDValue[0]", !353}
!369 = !{!"FuncMDMap[1]", void (<2 x i32> addrspace(1)*, <2 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16)* @test_st1}
!370 = !{!"FuncMDValue[1]", !366}
!371 = !{!"FuncMD", !367, !368, !369, !370}
!372 = !{!"ModuleMD", !371}
!IGCMetadata = !{!372}
