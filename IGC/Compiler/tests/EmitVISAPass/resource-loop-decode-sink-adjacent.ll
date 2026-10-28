;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that ResourceLoopDecodeSink leaves a run of sends alone when there is no decode code to move. Such a run keeps
; going through ResourceLoopAnalysis with FuseResourceLoopMinSize as tuned, so it must not get the marker that exempts
; a run from that threshold.
;
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformbmg -simd-mode 16 -inputrt -regkey MaxFuseResourceLoopSize=3 --resource-loop-decode-sink -S %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Z = constant i32 1
@ThreadGroupSize_Y = constant i32 1

define spir_kernel void @test_decode_sink_adjacent(i32 %src1, i32 %val, i32 addrspace(1)* %dst) {
entry:
  %svn = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %nonuniform = zext i16 %svn to i32
  %res = inttoptr i32 %nonuniform to <4 x float> addrspace(2621440)*

; COM: two adjacent sends, only an address computation between them: nothing moves and nothing is marked
; CHECK: %x = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32
; CHECK-NOT: !igc.resource.loop.prepared
; CHECK-NEXT: %off = add i32 %nonuniform, 4
; CHECK-NEXT: %y = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32
; CHECK-NOT: !igc.resource.loop.prepared
; CHECK: ret void
  %x = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %src1, i32 4, i1 false)
  %off = add i32 %nonuniform, 4
  %y = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %off, i32 4, i1 false)
  %s = add i32 %x, %y
  store i32 %s, i32 addrspace(1)* %dst, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #4
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #1

attributes #1 = { nounwind readnone }
attributes #4 = { argmemonly nounwind readonly }

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32, i32, i32 addrspace(1)*)* @test_decode_sink_adjacent}
!3 = !{!"FuncMDValue[0]", !4, !17}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11, !14, !15, !16}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !12, !8, !13}
!12 = !{!"type", i32 1}
!13 = !{!"indexType", i32 0}
!14 = !{!"argAllocMDListVec[3]", !7, !8, !9}
!15 = !{!"argAllocMDListVec[4]", !7, !8, !9}
!16 = !{!"argAllocMDListVec[5]", !7, !8, !9}
!17 = !{!"m_OpenCLArgTypeQualifiers", !18, !19, !20}
!18 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!19 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!20 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!21 = !{void (i32, i32, i32 addrspace(1)*)* @test_decode_sink_adjacent, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}

