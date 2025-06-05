;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers -igc-advmemopt -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; AdvMemOpt - check that only uniform load will be hoisted
; ------------------------------------------------

; CHECK-LABEL: @test_uniformness(
; CHECK:       bb:
; CHECK:         call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* [[B:%[A-z0-9]*]], i64 4, i1 true, i32 42){{.*}}
; CHECK-NEXT:    call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* [[B]], i64 4, i1 true, i32 43){{.*}}
; CHECK-NEXT:    br label %bb1
; CHECK:       bb1:
; CHECK:         [[ARRAYIDX1:%[A-z0-9]*]] = getelementptr inbounds i32, i32* [[B]], i64
; CHECK-NEXT:    call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* [[ARRAYIDX1]], i64 4, i1 true, i32 44){{.*}}
; CHECK:         ret void

define spir_kernel void @test_uniformness(i32* %b, i32 %a, float addrspace(1)* %sf, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %conv.i.i = zext i16 %localIdX to i64
  %0 = icmp slt i32 %a, 13
  br i1 %0, label %bb, label %end

bb:                                             ; preds = %bb1, %entry
  %1 = phi i32 [ 0, %entry ], [ %3, %bb1 ]
  %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %b, i64 4, i1 true, i32 42)
  br label %bb1

bb1:                                            ; preds = %bb
  %arrayidx1 = getelementptr inbounds i32, i32* %b, i64 %conv.i.i
  %cnu = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx1, i64 4, i1 true, i32 44)
  %c = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %b, i64 4, i1 true, i32 43)
  %3 = add i32 %1, %2
  %4 = add i32 %c, %3
  %5 = icmp slt i32 %3, %a
  br i1 %5, label %bb, label %end

end:                                              ; preds = %bb1, %entry
  %6 = phi i32 [ %a, %entry ], [ %4, %bb1 ]
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %b, i32 %6, i64 4, i1 true)
  ret void
}

; Function Attrs: nounwind readonly
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32*, i64, i1, i32) #0

declare void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32*, i32, i64, i1)

attributes #0 = { nounwind readonly }

!IGCMetadata = !{!0}
!igc.functions = !{!347}

!0 = !{!"ModuleMD", !78}
!78 = !{!"FuncMD", !79, !80}
!79 = !{!"FuncMDMap[0]", void (i32*, i32, float addrspace(1)*, i16, i16, i16)* @test_uniformness}
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
!347 = !{void (i32*, i32, float addrspace(1)*, i16, i16, i16)* @test_uniformness, !348}
!348 = !{!349, !350}
!349 = !{!"function_type", i32 0}
!350 = !{!"implicit_arg_desc", !351, !352, !353, !354, !355}
!351 = !{i32 0}
!352 = !{i32 1}
!353 = !{i32 8}
!354 = !{i32 9}
!355 = !{i32 10}
