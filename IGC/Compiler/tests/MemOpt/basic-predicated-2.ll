;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This is a second basic test to improve test coverage after support predicated loads/stores in MemOpt and LdStCombine
; It verifies non-uniform cases with alignment < 4 and >=4.

; RUN: igc_opt --typed-pointers %s -S -o - --basic-aa -igc-memopt -instcombine | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define spir_kernel void @test_uniformness(i32 addrspace(1)* %d, i32 addrspace(1)* %ss, float addrspace(1)* %sf, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %conv.i.i = zext i16 %localIdX to i64
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %ss, i64 %conv.i.i
  %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32 addrspace(1)* %arrayidx1, i64 2, i1 true, i32 42)
  %conv.i.i.1 = add i64 %conv.i.i, 1
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %ss, i64 %conv.i.i.1
  %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32 addrspace(1)* %arrayidx2, i64 4, i1 true, i32 43)
  %conv.i.i.2 = add i64 %conv.i.i, 2
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %ss, i64 %conv.i.i.2
  %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32 addrspace(1)* %arrayidx3, i64 4, i1 true, i32 44)
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32 addrspace(1)* %d, i32 %0, i64 2, i1 true)
  %arrayidx4 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 1
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32 addrspace(1)* %arrayidx4, i32 %1, i64 4, i1 true)
  %arrayidx5 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 2
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32 addrspace(1)* %arrayidx5, i32 %2, i64 4, i1 true)
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_uniformness
; CHECK:   %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32 addrspace(1)* %arrayidx1, i64 2, i1 true, i32 42)
; CHECK:   %3 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* %2, i64 4, i1 true, <2 x i32> <i32 43, i32 44>)
; CHECK:   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32 addrspace(1)* %d, i32 %1, i64 2, i1 true)
; CHECK:   call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* %4, <2 x i32> %3, i64 4, i1 true)

; Function Attrs: nounwind readonly
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32 addrspace(1)*, i64, i1, i32) #0

declare void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32 addrspace(1)*, i32, i64, i1)

attributes #0 = { nounwind readonly }

!IGCMetadata = !{!0}
!igc.functions = !{!347}

!0 = !{!"ModuleMD", !78}
!78 = !{!"FuncMD", !79, !80}
!79 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32 addrspace(1)*, float addrspace(1)*, i16, i16, i16)* @test_uniformness}
!80 = !{!"FuncMDValue[0]", !87, !114, !166}
!87 = !{!"functionType", !"KernelFunction"}
!114 = !{!"resAllocMD", !115, !116, !117, !118, !134}
!115 = !{!"uavsNumType", i32 3}
!116 = !{!"srvsNumType", i32 0}
!117 = !{!"samplersNumType", i32 0}
!118 = !{!"argAllocMDList", !119, !123, !125, !127, !130, !131, !132, !133}
!119 = !{!"argAllocMDListVec[0]", !120, !121, !122}
!120 = !{!"type", i32 1}
!121 = !{!"extensionType", i32 -1}
!122 = !{!"indexType", i32 0}
!123 = !{!"argAllocMDListVec[1]", !120, !121, !124}
!124 = !{!"indexType", i32 1}
!125 = !{!"argAllocMDListVec[2]", !120, !121, !126}
!126 = !{!"indexType", i32 2}
!127 = !{!"argAllocMDListVec[3]", !128, !121, !129}
!128 = !{!"type", i32 0}
!129 = !{!"indexType", i32 -1}
!130 = !{!"argAllocMDListVec[4]", !128, !121, !129}
!131 = !{!"argAllocMDListVec[5]", !128, !121, !129}
!132 = !{!"argAllocMDListVec[6]", !128, !121, !129}
!133 = !{!"argAllocMDListVec[7]", !128, !121, !129}
!134 = !{!"inlineSamplersMD"}
!166 = !{!"m_OpenCLArgTypeQualifiers", !167, !168, !169}
!167 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!168 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!169 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!347 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, float addrspace(1)*, i16, i16, i16)* @test_uniformness, !348}
!348 = !{!349, !350}
!349 = !{!"function_type", i32 0}
!350 = !{!"implicit_arg_desc", !351, !352, !353, !354, !355}
!351 = !{i32 0}
!352 = !{i32 1}
!353 = !{i32 8}
!354 = !{i32 9}
!355 = !{i32 10}
