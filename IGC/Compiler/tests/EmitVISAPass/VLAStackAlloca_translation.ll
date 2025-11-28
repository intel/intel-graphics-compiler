;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; This test checks stack memory allocation for uniform and non-uniform
; vlaStackAlloca calls

; CHECK-LABEL: .kernel "test_uniform"
; CHECK: add (M1_NM, 1) SP(0,0)<1> privateBase(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) FP(0,0)<1> SP(0,0)<0;1,0>
; CHECK: mul (M1_NM, 1) vlaSize(0,0)<1> array_size(0,0)<0;1,0> 0x4:w
; CHECK: add (M1, 32) vlaStackAlloca(0,0)<1> SP(0,0)<0;1,0> 0x0:d
; CHECK: add (M1_NM, 1) SP(0,0)<1> SP(0,0)<0;1,0> vlaSize(0,0)<0;1,0>

define spir_kernel void @test_uniform(i32 %array_size, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase, i32 %bufferOffset, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %vlaSize = mul i32 %array_size, 4
  %vlaStackAlloca = call ptr @llvm.genx.GenISA.VLAStackAlloca(i32 0, i32 %vlaSize)
  store i32 %array_size, ptr %vlaStackAlloca, align 4
  ret void
}

; CHECK-LABEL: .kernel "test_non_uniform"
; CHECK: add (M1_NM, 1) SP(0,0)<1> privateBase(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) FP(0,0)<1> SP(0,0)<0;1,0>
; CHECK: mov (M1_NM, 8) simdLaneId(0,0)<1> 0x76543210:v
; CHECK: add (M1_NM, 8) simdLaneId(0,8)<1> simdLaneId(0,0)<1;1,0> 0x8:w
; CHECK: add (M1_NM, 16) simdLaneId(0,16)<1> simdLaneId(0,0)<1;1,0> 0x10:w
; CHECK: mov (M1, 32) simdLaneIdExt(0,0)<1> simdLaneId_0v(0,0)<1;1,0>
; CHECK: mul (M1_NM, 1) vlaSize(0,0)<1> array_size(0,0)<0;1,0> 0x4:w
; CHECK: mul (M1, 32) vlaOffset(0,0)<1> vlaSize(0,0)<0;1,0> simdLaneIdExt(0,0)<1;1,0>
; CHECK: add (M1, 32) vlaStackAlloca(0,0)<1> SP(0,0)<0;1,0> vlaOffset(0,0)<1;1,0>
; CHECK: mul (M1_NM, 1) vlaSize(0,0)<1> vlaSize(0,0)<0;1,0> 0x20:uw
; CHECK: add (M1_NM, 1) SP(0,0)<1> SP(0,0)<0;1,0> vlaSize(0,0)<0;1,0>

define spir_kernel void @test_non_uniform(i32 %array_size, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase, i32 %bufferOffset, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %simdLaneIdExt = zext i16 %simdLaneId to i32
  %vlaSize = mul i32 %array_size, 4
  %vlaOffset = mul i32 %vlaSize, %simdLaneIdExt
  %vlaStackAlloca = call ptr @llvm.genx.GenISA.VLAStackAlloca(i32 %vlaOffset, i32 %vlaSize)
  store i32 %array_size, ptr %vlaStackAlloca, align 4
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()

declare ptr @llvm.genx.GenISA.VLAStackAlloca(i32, i32)

attributes #0 = { "hasVLA" }

!IGCMetadata = !{!0}
!igc.functions = !{!26, !34}

!0 = !{!"ModuleMD", !1, !25}
!1 = !{!"FuncMD", !2, !3, !23, !24}
!2 = !{!"FuncMDMap[0]", ptr @test_uniform}
!3 = !{!"FuncMDValue[0]", !4, !19}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11, !14, !15, !16, !17, !18}
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
!17 = !{!"argAllocMDListVec[6]", !7, !8, !9}
!18 = !{!"argAllocMDListVec[7]", !7, !8, !9}
!19 = !{!"m_OpenCLArgTypeQualifiers", !20, !21, !22}
!20 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!21 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!22 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!23 = !{!"FuncMDMap[1]", ptr @test_non_uniform}
!24 = !{!"FuncMDValue[1]", !4, !19}
!25 = !{!"isHDCFastClearShader", i1 false}
!26 = !{ptr @test_uniform, !27}
!27 = !{!28, !29}
!28 = !{!"function_type", i32 0}
!29 = !{!"implicit_arg_desc", !30, !31, !32, !33}
!30 = !{i32 0}
!31 = !{i32 1}
!32 = !{i32 13}
!33 = distinct !{i32 15, !33}
!34 = !{ptr @test_non_uniform, !27}
