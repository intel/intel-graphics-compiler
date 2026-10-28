;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that ResourceLoopDecodeSink honours the ALU distance of the shader type: a ray-tracing shader allows 2
; independent instructions between two sends of a run, a compute shader 4. The same IR is therefore grouped differently.
;
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformbmg -simd-mode 16 -inputrt -regkey MaxFuseResourceLoopSize=3 --resource-loop-decode-sink -S %s 2>&1 | FileCheck %s --check-prefix=RT
; RUN: igc_opt --typed-pointers -platformbmg -simd-mode 16 -inputcs -regkey MaxFuseResourceLoopSize=3 --resource-loop-decode-sink -S %s 2>&1 | FileCheck %s --check-prefix=CS

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Z = constant i32 1
@ThreadGroupSize_Y = constant i32 1

define spir_kernel void @test_decode_sink_rt(i32 %src1, i32 %val, i32 addrspace(1)* %dst) {
entry:
  %svn = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %nonuniform = zext i16 %svn to i32
  %res = inttoptr i32 %nonuniform to <4 x float> addrspace(2621440)*

; COM: the decode of the first load moves below the second one; the two instructions computing its address stay
; RT: [[A:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32{{.*}}!igc.resource.loop.prepared
; RT-NEXT: %off1 = add i32 %nonuniform, 4
; RT-NEXT: %off1b = add i32 %off1, %nonuniform
; RT-NEXT: [[B:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32{{.*}}!igc.resource.loop.prepared
; COM: three independent instructions are more than a ray-tracing shader allows, so the run ends at the second send
; RT-NEXT: %a.dec = and i32
; RT-NEXT: %b.dec = shl i32
; RT-NEXT: %o2 = add i32 %nonuniform, 8
; RT-NEXT: %o2b = add i32 %o2, %nonuniform
; RT-NEXT: %o2c = xor i32 %o2b, %nonuniform
; RT-NEXT: [[C:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32
; RT-NOT: !igc.resource.loop.prepared
; COM: a compute shader allows four, so all three loads are adjacent, both decodes move below the last one, and all
; COM: three sends carry the tag
; CS: [[A:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32{{.*}}!igc.resource.loop.prepared
; CS-NEXT: %off1 = add i32 %nonuniform, 4
; CS-NEXT: %off1b = add i32 %off1, %nonuniform
; CS-NEXT: [[B:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32{{.*}}!igc.resource.loop.prepared
; CS-NEXT: %o2 = add i32 %nonuniform, 8
; CS-NEXT: %o2b = add i32 %o2, %nonuniform
; CS-NEXT: %o2c = xor i32 %o2b, %nonuniform
; CS-NEXT: [[C:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32{{.*}}!igc.resource.loop.prepared
; CS-NEXT: %a.dec = and i32
; CS-NEXT: %b.dec = shl i32
  %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %src1, i32 4, i1 false)
  %a.dec = and i32 %a, 65535
  %off1 = add i32 %nonuniform, 4
  %off1b = add i32 %off1, %nonuniform
  %b = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %off1b, i32 4, i1 false)
  %b.dec = shl i32 %b, 2
  %o2 = add i32 %nonuniform, 8
  %o2b = add i32 %o2, %nonuniform
  %o2c = xor i32 %o2b, %nonuniform
  %c = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %o2c, i32 4, i1 false)
  %s0 = add i32 %a.dec, %b.dec
  %s1 = add i32 %s0, %c
  store i32 %s1, i32 addrspace(1)* %dst, align 4
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
!2 = !{!"FuncMDMap[0]", void (i32, i32, i32 addrspace(1)*)* @test_decode_sink_rt}
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
!21 = !{void (i32, i32, i32 addrspace(1)*)* @test_decode_sink_rt, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}

