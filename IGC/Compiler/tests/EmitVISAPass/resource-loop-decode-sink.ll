;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that ResourceLoopDecodeSink moves the decode code of a lane-varying send below the last send of a run of
; same-resource sends, so that the sends become adjacent and can be fused into one resource loop.
;
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformPtl -simd-mode 16 -inputcs -regkey MaxFuseResourceLoopSize=3 --resource-loop-decode-sink -S %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

define spir_kernel void @test_decode_sink(<8 x i32> %r0, i32 %runtime_value_0, i32 %runtime_value_1, <4 x float> addrspace(2)* %runtime_value_2, <4 x float> addrspace(2)* %runtime_value_3, i32 %runtime_value_4, i32 %runtime_value_5) #0 {
entry:
  %0 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
  %GroupID_X = bitcast float %0 to i32
  %1 = shl i32 %GroupID_X, 6
  %2 = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %3 = zext i16 %2 to i32
  %ThreadID_X = add i32 %1, %3
  %4 = ptrtoint <4 x float> addrspace(2)* %runtime_value_3 to i64
  %5 = add i64 %4, 272
  %6 = inttoptr i64 %5 to <4 x i32> addrspace(2)*
  %7 = load <4 x i32>, <4 x i32> addrspace(2)* %6, align 4, !invariant.load !114
  %8 = extractelement <4 x i32> %7, i32 3
  %9 = add i32 %runtime_value_4, %ThreadID_X
  %res = inttoptr i32 %9 to <4 x float> addrspace(2621440)*
  %o = add i32 %runtime_value_5, %ThreadID_X
  %other = inttoptr i32 %o to <4 x float> addrspace(2621441)*

; COM: the decode of the first load moves below the last load of the run: A, B and C are adjacent
; COM: (only the address computation, which feeds the next load, stays between them)
; CHECK: [[A:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32
; CHECK-NEXT: [[OFF1:%.*]] = or i32
; CHECK-NEXT: [[B:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32
; CHECK-NEXT: [[OFF2:%.*]] = or i32
; CHECK-NEXT: [[C:%.*]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32
; CHECK-NEXT: %a.dec = and i32 [[A]], 1073741823
; CHECK-NEXT: %a.cvt = uitofp i32 %a.dec to float
; CHECK-NEXT: %b.dec = shl i32 [[B]], 2
  %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %ThreadID_X, i32 4, i1 false)
  %a.dec = and i32 %a, 1073741823
  %a.cvt = uitofp i32 %a.dec to float
  %off1 = or i32 %ThreadID_X, 4
  %b = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %off1, i32 4, i1 false)
  %b.dec = shl i32 %b, 2
  %off2 = or i32 %ThreadID_X, 8
  %c = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %off2, i32 4, i1 false)

; COM: a send of another resource ends the run: nothing moves across it
; CHECK: %d = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32
; CHECK-NEXT: %d.dec = and i32 %d, 7
; CHECK-NEXT: %e = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32
  %d = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32(<4 x float> addrspace(2621441)* %other, i32 %ThreadID_X, i32 4, i1 false)
  %d.dec = and i32 %d, 7
  %e = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %ThreadID_X, i32 4, i1 false)

  %a.bc = bitcast float %a.cvt to i32
  %s0 = add i32 %a.bc, %b.dec
  %s1 = add i32 %s0, %c
  %s2 = add i32 %s1, %d.dec
  %s3 = add i32 %s2, %e
  %sink_ptr = inttoptr i32 %s3 to i32*
  store i32 %s3, i32* %sink_ptr
  ret void
}

declare float @llvm.genx.GenISA.DCL.SystemValue.f32(i32) #1
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #1

declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #2
declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621441v4f32(<4 x float> addrspace(2621441)*, i32, i32, i1) #2

attributes #0 = { convergent nounwind "null-pointer-is-valid"="true" }
attributes #1 = { argmemonly nounwind readonly willreturn }
attributes #2 = { argmemonly nounwind readonly }

!IGCMetadata = !{!0}
!igc.functions = !{!100}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (<8 x i32>, i32, i32, <4 x float> addrspace(2)*, <4 x float> addrspace(2)*, i32, i32)* @test_decode_sink}
!3 = !{!"FuncMDValue[0]", !4, !5, !123}
!4 = !{!"functionType", !"KernelFunction"}
!5 = !{!"resAllocMD", !6, !7, !8, !9, !20}
!6 = !{!"uavsNumType", i32 0}
!7 = !{!"srvsNumType", i32 0}
!8 = !{!"samplersNumType", i32 0}
!9 = !{!"argAllocMDList", !10, !12, !13, !14, !15, !16, !17}
!10 = !{!"argAllocMDListVec[0]", !30, !31, !32}
!12 = !{!"argAllocMDListVec[1]", !30, !31, !32}
!13 = !{!"argAllocMDListVec[2]", !30, !31, !32}
!14 = !{!"argAllocMDListVec[3]", !30, !31, !32}
!15 = !{!"argAllocMDListVec[4]", !30, !31, !32}
!16 = !{!"argAllocMDListVec[5]", !30, !31, !32}
!17 = !{!"argAllocMDListVec[6]", !30, !31, !32}
!20 = !{!"inlineSamplersMD"}
!30 = !{!"type", i32 0}
!31 = !{!"extensionType", i32 -1}
!32 = !{!"indexType", i32 -1}

!100 = !{void (<8 x i32>, i32, i32, <4 x float> addrspace(2)*, <4 x float> addrspace(2)*, i32, i32)* @test_decode_sink, !101}
!101 = !{!102}
!102 = !{!"function_type", i32 0}

!114 = !{null}
!115 = !{!"argId", i32 0}
!116 = !{!"implicitArgInfoListVec[0]", !115}
!117 = !{!"argId", i32 1}
!118 = !{!"implicitArgInfoListVec[1]", !117}
!119 = !{!"argId", i32 8}
!120 = !{!"implicitArgInfoListVec[2]", !119}
!121 = !{!"argId", i32 9}
!122 = !{!"implicitArgInfoListVec[3]", !121}
!123 = !{!"implicitArgInfoList", !116, !118, !120, !122}
