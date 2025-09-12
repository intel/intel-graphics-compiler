;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: opaque-ptr-fix, regkeys, llvm-14-plus
;
; RUN: igc_opt --typed-pointers -simd-mode 8 -platformdg2 -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s -check-prefixes=CHECK-BOTH,CHECK-RGBA
; RUN: igc_opt --typed-pointers -simd-mode 8 -platformdg2 -igc-emit-visa -regkey HandlePhiNodeInChannelPrune=1 -regkey DumpVISAASMToConsole < %s | FileCheck %s -check-prefixes=CHECK-BOTH,CHECK-ONLYR
; RUN: igc_opt --typed-pointers -simd-mode 8 -platformdg2 -igc-emit-visa -regkey HandlePhiNodeInChannelPrune=1 -regkey EnableDeSSA=0 -regkey DumpVISAASMToConsole < %s | FileCheck %s -check-prefixes=CHECK-BOTH,CHECK-NOCOALESCE_ONLYR
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------


; Test0 checks "sampleptr" intrinsic when only one channel is used (no phi node).

; CHECK-BOTH-LABEL: .kernel "test0"
; CHECK-BOTH: sample_3d.R (M1, 8)


define spir_kernel void @test0(i32 addrspace(1)* %res32, <4 x float> addrspace(1)* %resf) {
entry0:
  %sample0 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32(float 1.0, float 2.0, float 3.0, float 4.0, float 5.0, float 6.0, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 0, i32 0, i32 0)
  %eeSample0 = extractelement <4 x float> %sample0, i32 0
  %v00 = insertelement <4 x float> undef, float %eeSample0, i32 0
  %v01 = insertelement <4 x float> %v00, float 0.0, i32 1
  %v02 = insertelement <4 x float> %v01, float 0.0, i32 2
  %v03 = insertelement <4 x float> %v02, float 0.0, i32 3
  store <4 x float> %v03, <4 x float> addrspace(1)* %resf
  ret void
}

; Test1 checks whether the "sampleptr" intrinsic will emit only R instead of RGBA when used in a phi node.
; In this test, the sample is not coalesced because it's used both in the phi node and separately in extractelement.

; CHECK-BOTH-LABEL: .kernel "test1"
; CHECK-RGBA: sample_3d.RGBA (M1, 8)
; CHECK-RGBA: sample_3d.RGBA (M1, 8)
; CHECK-ONLYR: sample_3d.R (M1, 8)
; CHECK-ONLYR: sample_3d.R (M1, 8)
; CHECK-NOCOALESCE_ONLYR: sample_3d.R (M1, 8)
; CHECK-NOCOALESCE_ONLYR: sample_3d.R (M1, 8)


define spir_kernel void @test1(i32 addrspace(1)* %res32, <4 x float> addrspace(1)* %resf) {
entry:
  %res32_val = load i32, i32 addrspace(1)* %res32
  %cmp2 = icmp eq i32 %res32_val, 0
  %sample1 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32(float 1.0, float 2.0, float 3.0, float 4.0, float 5.0, float 6.0, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 0, i32 0, i32 0)
  br i1 %cmp2, label %BB0, label %BB1

BB0:
  %sample2 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32(float 5.0, float 6.0, float 7.0, float 8.0, float 9.0, float 10.0, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 0, i32 0, i32 0)
  br label %BB1

BB1:
  %phi = phi <4 x float> [ %sample1, %entry ], [ %sample2, %BB0 ]
  %eeSample1 = extractelement <4 x float> %sample1, i32 0
  %eePhi = extractelement <4 x float> %phi, i32 0
  %maxEE = call float @llvm.maxnum.f32(float %eeSample1, float %eePhi)
  %v0 = insertelement <4 x float> undef, float %maxEE, i32 0
  %v1 = insertelement <4 x float> %v0, float 0.0, i32 1
  %v2 = insertelement <4 x float> %v1, float 0.0, i32 2
  %v3 = insertelement <4 x float> %v2, float 0.0, i32 3

  store <4 x float> %v3, <4 x float> addrspace(1)* %resf
  ret void
}

; Test2 checks whether the "sampleptr" intrinsic will emit only R when the sample is used only in a phi node (not separately in extractelement).
; In this test, the sample is coalesced because it's only used in the phi node.
;
; Note: Due to DeSSA coalescing it does not work even with handlePhi flag, as we don't run channel pruning for coalesced variables.

; CHECK-BOTH-LABEL: .kernel "test2"
; CHECK-RGBA: sample_3d.RGBA (M1, 8)
; CHECK-RGBA: sample_3d.RGBA (M1, 8)
; CHECK-ONLYR-NOT: sample_3d.R (M1, 8)
; CHECK-ONLYR-NOT: sample_3d.R (M1, 8)
; CHECK-NOCOALESCE_ONLYR-NOT: sample_3d.R (M1, 8)
; CHECK-NOCOALESCE_ONLYR: sample_3d.R (M1, 8)


define spir_kernel void @test2(i32 addrspace(1)* %res32, <4 x float> addrspace(1)* %resf) {
entry2:
  %res32_val2 = load i32, i32 addrspace(1)* %res32
  %cmp22 = icmp eq i32 %res32_val2, 0
  %sample3 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32(float 1.0, float 2.0, float 3.0, float 4.0, float 5.0, float 6.0, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 0, i32 0, i32 0)
  br i1 %cmp22, label %BB02, label %BB12

BB02:
  %sample4 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32(float 5.0, float 6.0, float 7.0, float 8.0, float 9.0, float 10.0, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 0, i32 0, i32 0)
  br label %BB12

BB12:
  %phi2 = phi <4 x float> [ %sample3, %entry2 ], [ %sample4, %BB02 ]
  %eePhi2 = extractelement <4 x float> %phi2, i32 0
  %v02 = insertelement <4 x float> undef, float %eePhi2, i32 0
  %v12 = insertelement <4 x float> %v02, float 0.0, i32 1
  %v22 = insertelement <4 x float> %v12, float 0.0, i32 2
  %v32 = insertelement <4 x float> %v22, float 0.0, i32 3

  store <4 x float> %v32, <4 x float> addrspace(1)* %resf
  ret void
}

; Test_cycle checks whether "sampleptr" intrinsic will emit only R when the sample is used in a phi node inside a loop forming cycle.
;
; Note: Due to DeSSA coalescing it does not work completely even with handlePhi flag, as we don't run channel pruning for coalesced variables.
;
; CHECK-BOTH-LABEL: .kernel "test_cycle"
; CHECK-RGBA: sample_3d.RGBA (M1, 8)
; CHECK-RGBA: sample_3d.RGBA (M1, 8)
; CHECK-ONLYR: sample_3d.R (M1, 8)
; CHECK-ONLYR-NOT: sample_3d.R (M1, 8)
; CHECK-NOCOALESCE_ONLYR: sample_3d.R (M1, 8)
; CHECK-NOCOALESCE_ONLYR: sample_3d.R (M1, 8)

define spir_kernel void @test_cycle(i32 addrspace(1)* %res32, <4 x float> addrspace(1)* %resf) {
entry:
  %sampleA = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32(    float 1.0, float 2.0, float 3.0, float 4.0, float 5.0, float 6.0,    i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32,    i32 0, i32 0, i32 0)
  br label %loop.start

loop.start:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop.if.merge ]
  %phiC = phi <4 x float> [ %sampleA, %entry ], [ %phiC_next, %loop.if.merge ] ; cycle
  %tmp = and i32 %i, 1
  %cond = icmp eq i32 %tmp, 0
  br i1 %cond, label %loop.if.then, label %loop.if.merge

loop.if.then:
  %i_float = sitofp i32 %i to float
  %sampleB = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32( float %i_float, float 8.0, float 9.0, float 10.0, float 11.0, float 12.0,    i32 addrspace(1)* %res32, i32 addrspace(1)* %res32, i32 addrspace(1)* %res32,    i32 0, i32 0, i32 0)
  br label %loop.if.merge

loop.if.merge:
  %phiC_next = phi <4 x float> [ %phiC, %loop.start ], [ %sampleB, %loop.if.then ] ; cycle
  %eeSamplerA0 = extractelement <4 x float> %sampleA, i32 0
  %eePhiC_next = extractelement <4 x float> %phiC_next, i32 0
  %hardUse3 = fmul float %eePhiC_next, 2.0
  %tmp3 = call float @llvm.maxnum.f32(float %eeSamplerA0, float %hardUse3)
  %i.next = add i32 %i, 1
  %cond2 = icmp slt i32 %i.next, 4
  br i1 %cond2, label %loop.start, label %exit

exit:
  %v0 = insertelement <4 x float> undef, float %tmp3, i32 0
  %v1 = insertelement <4 x float> %v0, float 0.0, i32 1
  %v2 = insertelement <4 x float> %v1, float 0.0, i32 2
  %v3 = insertelement <4 x float> %v2, float 0.0, i32 3
  store <4 x float> %v3, <4 x float> addrspace(1)* %resf
  ret void
}



declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32(float, float, float, float, float, float, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32, i32, i32)
declare float @llvm.maxnum.f32(float, float)
!IGCMetadata = !{!0}
!igc.functions = !{!21, !30, !37, !44}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !24, !25, !33, !34, !40, !41}
!2 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <4 x float> addrspace(1)*)* @test1}
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

!24 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*, <4 x float> addrspace(1)*)* @test2}
!25 = !{!"FuncMDValue[1]", !26, !17}
!26 = !{!"resAllocMD", !27}
!27 = !{!"argAllocMDList", !28, !29}
!28 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!29 = !{!"argAllocMDListVec[1]", !12, !8, !13}

!33 = !{!"FuncMDMap[2]", void (i32 addrspace(1)*, <4 x float> addrspace(1)*)* @test0}
!34 = !{!"FuncMDValue[2]", !35, !17}
!35 = !{!"resAllocMD", !36}
!36 = !{!"argAllocMDList", !28, !29}

!40 = !{!"FuncMDMap[3]", void (i32 addrspace(1)*, <4 x float> addrspace(1)*)* @test_cycle}
!41 = !{!"FuncMDValue[3]", !42, !17}
!42 = !{!"resAllocMD", !43}
!43 = !{!"argAllocMDList", !28, !29}

!21 = !{void (i32 addrspace(1)*, <4 x float> addrspace(1)*)* @test1, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}
!30 = !{void (i32 addrspace(1)*, <4 x float> addrspace(1)*)* @test2, !31}
!31 = !{!32}
!32 = !{!"function_type", i32 0}
!37 = !{void (i32 addrspace(1)*, <4 x float> addrspace(1)*)* @test0, !38}
!38 = !{!39}
!39 = !{!"function_type", i32 0}
!44 = !{void (i32 addrspace(1)*, <4 x float> addrspace(1)*)* @test_cycle, !45}
!45 = !{!46}
!46 = !{!"function_type", i32 0}
