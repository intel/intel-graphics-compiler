;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers %s -S -o - -igc-stateless-to-stateful-resolution --target-addressing-mode bindless | FileCheck %s
; ------------------------------------------------
; PromoteStatelessToBindless : Test promotion of block read and write to bindless addressing
; ------------------------------------------------

; Source:
; kernel void test_promote(global int* src, global int* dst, int runtimeOffset)
; {
;     uint v = intel_sub_group_block_read(&src[runtimeOffset]);
;     intel_sub_group_block_write(&dst[runtimeOffset], v);
; }

; CHECK: target datalayout = "p2490368:32:32:32"

; CHECK-LABEL: @test_promote
; CHECK: [[RUNTIME_OFFSET:%.*]] = sext i32 %runtimeOffset to i64
; CHECK: [[TMP0:%.*]] = trunc i64 [[RUNTIME_OFFSET]] to i32
; CHECK: [[TMP1:%.*]] = mul i32 [[TMP0]], 4
; CHECK: [[BYTE_OFFSET0:%.*]] = add i32 %bufferOffset, [[TMP1]]
; CHECK: [[BASEPTR_SRC:%.*]] = inttoptr i32 %bindlessOffset to i32 addrspace(2490368)*
; CHECK: [[LOADED_VALUE:%.*]] = call i32 @llvm.genx.GenISA.simdBlockReadBindless.i32.p2490368i32.i32(i32 addrspace(2490368)* [[BASEPTR_SRC]], i32 [[BYTE_OFFSET0]])
; CHECK: [[TMP2:%.*]] = trunc i64 [[RUNTIME_OFFSET]] to i32
; CHECK: [[TMP3:%.*]] = mul i32 [[TMP2]], 4
; CHECK: [[BYTE_OFFSET1:%.*]] = add i32 %bufferOffset1, [[TMP3]]
; CHECK: [[BASEPTR_DST:%.*]] = inttoptr i32 %bindlessOffset2 to i32 addrspace(2490368)*
; CHECK: call void @llvm.genx.GenISA.simdBlockWriteBindless.p2490368i32.i32.i32(i32 addrspace(2490368)* [[BASEPTR_DST]], i32 [[LOADED_VALUE]], i32 [[BYTE_OFFSET1]])

define spir_kernel void @test_promote(i32 addrspace(1)* %src, i32 addrspace(1)* %dst, i32 %runtimeOffset, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i32 %bufferOffset1, i32 %bindlessOffset, i32 %bindlessOffset2) #0 {
entry:
  %idxprom = sext i32 %runtimeOffset to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %idxprom
  %0 = call i32 @llvm.genx.GenISA.simdBlockRead.i32.p1i32(i32 addrspace(1)* %arrayidx)
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %idxprom
  call void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)* %arrayidx2, i32 %0)
  ret void
}

; Function Attrs: nounwind readonly
declare i32 @llvm.genx.GenISA.simdBlockRead.i32.p1i32(i32 addrspace(1)*) #2

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)*, i32) #3

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { nounwind readonly }
attributes #3 = { nounwind }

!igc.functions = !{!3}
!IGCMetadata = !{!15}

!3 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i32, i32, i32, i32)* @test_promote, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !11, !13, !14}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 15, !10}
!10 = !{!"explicit_arg_num", i32 0}
!11 = !{i32 15, !12}
!12 = !{!"explicit_arg_num", i32 1}
!13 = !{i32 59, !10}
!14 = !{i32 59, !12}
!15 = !{!"ModuleMD", !17, !90}
!17 = !{!"compOpt", !48, !49, !57, !58}
!48 = !{!"HasBufferOffsetArg", i1 true}
!49 = !{!"BufferOffsetArgOptional", i1 true}
!57 = !{!"UseBindlessMode", i1 true}
!58 = !{!"UseLegacyBindlessMode", i1 false}
!90 = !{!"FuncMD", !91, !92}
!91 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i32, i32, i32, i32)* @test_promote}
!92 = !{!"FuncMDValue[0]",!99, !121}
!99 = !{!"functionType", !"KernelFunction"}
!121 = !{!"resAllocMD", !122, !123, !124, !125, !141}
!122 = !{!"uavsNumType", i32 4}
!123 = !{!"srvsNumType", i32 0}
!124 = !{!"samplersNumType", i32 0}
!125 = !{!"argAllocMDList", !126, !130, !132, !135, !136, !137, !138, !139, !140}
!126 = !{!"argAllocMDListVec[0]", !127, !128, !129}
!127 = !{!"type", i32 1}
!128 = !{!"extensionType", i32 -1}
!129 = !{!"indexType", i32 2}
!130 = !{!"argAllocMDListVec[1]", !127, !128, !131}
!131 = !{!"indexType", i32 3}
!132 = !{!"argAllocMDListVec[2]", !133, !128, !134}
!133 = !{!"type", i32 0}
!134 = !{!"indexType", i32 -1}
!135 = !{!"argAllocMDListVec[3]", !133, !128, !134}
!136 = !{!"argAllocMDListVec[4]", !133, !128, !134}
!137 = !{!"argAllocMDListVec[5]", !133, !128, !134}
!138 = !{!"argAllocMDListVec[6]", !133, !128, !134}
!139 = !{!"argAllocMDListVec[7]", !133, !128, !134}
!140 = !{!"argAllocMDListVec[8]", !133, !128, !134}
!141 = !{!"inlineSamplersMD"}
