;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that ResourceLoopDecodeSink does not build a run that ResourceLoopAnalysis would split: a uniform instruction or
; an i1 computation between two sends ends the run, so nothing moves and nothing is tagged.
;
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformbmg -simd-mode 16 -inputrt -regkey MaxFuseResourceLoopSize=3 --resource-loop-decode-sink -S %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Z = constant i32 1
@ThreadGroupSize_Y = constant i32 1

define spir_kernel void @test_decode_sink_barrier(i32 %src1, i32 %val, i32 addrspace(1)* %dst) {
entry:
  %svn = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %nonuniform = zext i16 %svn to i32
  %res = inttoptr i32 %nonuniform to <4 x float> addrspace(2621440)*
  %n2 = add i32 %nonuniform, 64
  %res2 = inttoptr i32 %n2 to <4 x float> addrspace(2621441)*

; COM: a uniform address computation between the sends ends the run: the decode of the first load stays where it is
; CHECK: %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32
; CHECK-NOT: !igc.resource.loop.prepared
; CHECK-NEXT: %a.dec = and i32
; CHECK-NEXT: %u = add i32 %src1, 4
; CHECK-NEXT: %b = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32
; CHECK-NOT: !igc.resource.loop.prepared
; COM: an i1 computation between the sends ends the run as well
; CHECK: %c = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32
; CHECK-NOT: !igc.resource.loop.prepared
; CHECK-NEXT: %c.dec = and i32
; CHECK-NEXT: %flag = icmp eq i32 %nonuniform, 3
; CHECK-NEXT: %fz = zext i1 %flag to i32
; CHECK-NEXT: %off2 = add i32 %fz, %nonuniform
; CHECK-NEXT: %d = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32
; CHECK-NOT: !igc.resource.loop.prepared
; CHECK: ret void
  %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %nonuniform, i32 4, i1 false)
  %a.dec = and i32 %a, 65535
  %u = add i32 %src1, 4
  %b = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %u, i32 4, i1 false)
  %c = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32(<4 x float> addrspace(2621441)* %res2, i32 %nonuniform, i32 4, i1 false)
  %c.dec = and i32 %c, 65535
  %flag = icmp eq i32 %nonuniform, 3
  %fz = zext i1 %flag to i32
  %off2 = add i32 %fz, %nonuniform
  %d = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32(<4 x float> addrspace(2621441)* %res2, i32 %off2, i32 4, i1 false)
  %s0 = add i32 %a.dec, %b
  %s1 = add i32 %s0, %c.dec
  %s2 = add i32 %s1, %d
  store i32 %s2, i32 addrspace(1)* %dst, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #4
declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32(<4 x float> addrspace(2621441)*, i32, i32, i1) #4
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #1

attributes #1 = { nounwind readnone }
attributes #4 = { argmemonly nounwind readonly }

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32, i32, i32 addrspace(1)*)* @test_decode_sink_barrier}
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
!21 = !{void (i32, i32, i32 addrspace(1)*)* @test_decode_sink_barrier, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}

