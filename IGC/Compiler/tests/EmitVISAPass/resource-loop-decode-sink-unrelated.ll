;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that ResourceLoopDecodeSink moves the code between two sends that the later sends do not need, not only the code
; that consumes a send. Only what a later send needs, transitively, has to stay between the sends.
;
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformbmg -simd-mode 16 -inputrt -regkey MaxFuseResourceLoopSize=3 --resource-loop-decode-sink -S %s 2>&1 | FileCheck %s
; RUN: igc_opt --typed-pointers -platformbmg -simd-mode 16 -inputcs -regkey MaxFuseResourceLoopSize=3 --resource-loop-decode-sink -S %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Z = constant i32 1
@ThreadGroupSize_Y = constant i32 1

define spir_kernel void @test_decode_sink_unrelated(i32 %src1, i32 %val, i32 addrspace(1)* %dst) {
entry:
  %svn = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %nonuniform = zext i16 %svn to i32
  %res = inttoptr i32 %nonuniform to <4 x float> addrspace(2621440)*
  %n2 = add i32 %nonuniform, 64
  %res2 = inttoptr i32 %n2 to <4 x float> addrspace(2621441)*
  %n3 = add i32 %nonuniform, 128
  %res3 = inttoptr i32 %n3 to <4 x float> addrspace(2621442)*

; COM: an unrelated lane-varying instruction between the sends moves below the run together with the decode, and only
; COM: the address of the second load stays between them
; CHECK: %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32{{.*}}!igc.resource.loop.prepared
; CHECK-NEXT: %off = add i32 %nonuniform, 4
; CHECK-NEXT: %b = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32{{.*}}!igc.resource.loop.prepared
; CHECK-NEXT: %a.dec = and i32
; CHECK-NEXT: %unrelated = mul i32 %nonuniform, 77
; CHECK-NEXT: %b.dec = shl i32
  %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %nonuniform, i32 4, i1 false)
  %a.dec = and i32 %a, 65535
  %unrelated = mul i32 %nonuniform, 77
  %off = add i32 %nonuniform, 4
  %b = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %off, i32 4, i1 false)
  %b.dec = shl i32 %b, 2

; COM: an unrelated uniform instruction and an unrelated i1 computation would end a resource loop if they stayed
; COM: between the sends, so they move as well
; CHECK: %c = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32{{.*}}!igc.resource.loop.prepared
; CHECK-NEXT: %off2 = add i32 %nonuniform, 8
; CHECK-NEXT: %d = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32{{.*}}!igc.resource.loop.prepared
; CHECK-NEXT: %c.dec = and i32
; CHECK-NEXT: %uni = add i32 %src1, 9
; CHECK-NEXT: %flag = icmp eq i32 %nonuniform, 3
; CHECK-NEXT: %d.dec = shl i32
  %c = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32(<4 x float> addrspace(2621441)* %res2, i32 %nonuniform, i32 4, i1 false)
  %c.dec = and i32 %c, 255
  %uni = add i32 %src1, 9
  %flag = icmp eq i32 %nonuniform, 3
  %off2 = add i32 %nonuniform, 8
  %d = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32(<4 x float> addrspace(2621441)* %res2, i32 %off2, i32 4, i1 false)
  %d.dec = shl i32 %d, 1

; COM: %k is needed by the third send, not by the second one. It cannot stay between the first two sends (ResourceLoopAnalysis
; COM: would end the loop there), so the run is the first two sends and %k moves below them
; CHECK: %e = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32{{.*}}!igc.resource.loop.prepared
; CHECK-NEXT: %f = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32{{.*}}!igc.resource.loop.prepared
; CHECK-NEXT: %k = add i32 %nonuniform, 5
; CHECK-NEXT: %g = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32
; CHECK-NOT: !igc.resource.loop.prepared
; CHECK: ret void
  %e = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)* %res3, i32 %nonuniform, i32 4, i1 false)
  %k = add i32 %nonuniform, 5
  %f = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)* %res3, i32 %nonuniform, i32 8, i1 false)
  %g = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)* %res3, i32 %k, i32 4, i1 false)

  %s0 = add i32 %a.dec, %b.dec
  %s1 = add i32 %s0, %unrelated
  %s2 = add i32 %s1, %c.dec
  %s3 = add i32 %s2, %d.dec
  %s4 = select i1 %flag, i32 %s3, i32 %uni
  %s5 = add i32 %s4, %e
  %s6 = add i32 %s5, %f
  %s7 = add i32 %s6, %g
  store i32 %s7, i32 addrspace(1)* %dst, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #4
declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32(<4 x float> addrspace(2621441)*, i32, i32, i1) #4
declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)*, i32, i32, i1) #4
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #1

attributes #1 = { nounwind readnone }
attributes #4 = { argmemonly nounwind readonly }

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32, i32, i32 addrspace(1)*)* @test_decode_sink_unrelated}
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
!21 = !{void (i32, i32, i32 addrspace(1)*)* @test_decode_sink_unrelated, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}

