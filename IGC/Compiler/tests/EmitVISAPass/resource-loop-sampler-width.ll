;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that a resource loop reads a non-uniform sampler from the lane it picked, when the sampler is wider than the
; texture. The lane offset is computed once, for the texture (4-byte elements), and has to be rescaled for the 8-byte
; sampler: without it, every lane but the first one reads the sampler of another lane and the loop never ends.
;
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformbmg -igc-emit-visa -simd-mode 16 -inputrt -regkey FuseResourceLoop=0,DumpVISAASMToConsole -S < %s | FileCheck %s

; CHECK: _opt_resource_loop:
; CHECK: fbl (M1_NM, 1) [[LANE:V[0-9]+]](0,0)<1> {{.*}}
; COM: the offset of the 32-bit texture: lane * 4
; CHECK: shl (M1_NM, 1) [[OFF:V[0-9]+]](0,0)<1> [[LANE]](0,0)<0;1,0> 0x2:w
; CHECK: addr_add (M1_NM, 1) A0(0)<1> &texture [[UW:V[0-9]+]](0,0)<0;1,0>
; COM: the offset of the 64-bit sampler: lane * 8, the (word) offset of the texture shifted once more
; CHECK: shl (M1_NM, 1) [[SOFF:V[0-9]+]](0,0)<1> [[UW]](0,0)<0;1,0> 0x1:w
; CHECK: addr_add (M1_NM, 1) A1(0)<1> &sel_ [[SOFF]](0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) {{.*}} r[A1(0),0]<0;1,0>:q

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

%__2D_DIM_Resource = type opaque

define spir_kernel void @test1(<64 x i32> %src, float addrspace(1)* %dst) {
entry:

  %svn0 = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %sampler = zext i16 %svn0 to i32
  %sampler64 = zext i32 %sampler to i64
  %sampler64b = add i64 %sampler64, 16
  %cnd = icmp ult i32 %sampler, 8
  %sel = select i1 %cnd, i64 %sampler64, i64 %sampler64b
  %NonUniformSampler = inttoptr i64 %sel to <4 x float> addrspace(2752518)*

  %svn1 = extractelement <64 x i32> %src, i32 40
  %texture = add i32 %sampler, 1280
  %NonUniformTexture = inttoptr i32 %texture to %__2D_DIM_Resource addrspace(2621450)*

  %call = tail call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621443__2D_DIM_Resource.p2621443__2D_DIM_Resource.p2752518v4f32(float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621450)* undef, %__2D_DIM_Resource addrspace(2621450)* %NonUniformTexture, <4 x float> addrspace(2752518)* %NonUniformSampler, i32 0, i32 0, i32 0)

  %out = extractelement <4 x float> %call, i32 0
  store float %out, float addrspace(1)* %dst, align 4
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621443__2D_DIM_Resource.p2621443__2D_DIM_Resource.p2752518v4f32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621450)*, %__2D_DIM_Resource addrspace(2621450)*, <4 x float> addrspace(2752518)*, i32, i32, i32) #4

declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #1
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #6

declare i32 @llvm.genx.GenISA.WaveBallot(i1, i32)
declare i32 @llvm.genx.GenISA.firstbitLo(i32)

attributes #4 = { argmemonly nounwind readonly }


!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (<64 x i32>, float addrspace(1)*)* @test1}
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
!21 = !{void (<64 x i32>, float addrspace(1)*)* @test1, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}
