;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -platformdg2 -igc-emit-visa -simd-mode 32 %s -regkey DumpVISAASMToConsole | FileCheck %s

; This test verifies whether uniform bindless byte loads and stores with alignment greater than 1
; can properly be handled by EmitVISAPass. They should simply be treated as memory operations
; with alignment 1.

; The tested module has been generated from the following OpenCL C code:
; kernel void test(global char* in, global char* out)
; {
;     out[1] = in[1];
; }

; However, one small manual modification has been applied on beforeUnification while generating below
; LLVM module. An alignment for load and store instruction has manually been modified from 1 to 2.
; Byte memory instructions with alignment 2 are legal, but we cannot generate them directly from OpenCL C.
; However, SYCL FE Compiler can generate them, so IGC needs to be able to handle such cases properly.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test(i8 addrspace(1)* %in, i8 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i32 %bufferOffset1, i32 %bindlessOffset, i32 %bindlessOffset2) #0 {
entry:
  %0 = add i32 %bufferOffset, 1
  %1 = inttoptr i32 %bindlessOffset to i8 addrspace(2490368)*
; CHECK: movs (M1_NM, 1) %bss(0) bindlessOffset(0,0)<0;1,0>
; CHECK: gather_scaled.1 (M1_NM, 1) %bss 0x0:ud {{.*}} {{.*}}
  %2 = call i8 @llvm.genx.GenISA.ldraw.indexed.i8.p2490368i8(i8 addrspace(2490368)* %1, i32 %0, i32 2, i1 false)
  %3 = add i32 %bufferOffset1, 1
  %4 = inttoptr i32 %bindlessOffset2 to i8 addrspace(2490368)*
; CHECK: movs (M1_NM, 1) %bss(0) bindlessOffset{{.*}}(0,0)<0;1,0>
; CHECK: scatter_scaled.1 (M1, 16) %bss 0x0:ud {{.*}} {{.*}}
; CHECK: movs (M1_NM, 1) %bss(0) bindlessOffset{{.*}}(0,0)<0;1,0>
; CHECK: scatter_scaled.1 (M5, 16) %bss 0x0:ud {{.*}} {{.*}}
  call void @llvm.genx.GenISA.storeraw.indexed.p2490368i8.i8(i8 addrspace(2490368)* %4, i32 %3, i8 %2, i32 2, i1 false)
  ret void
}

declare i8 @llvm.genx.GenISA.ldraw.indexed.i8.p2490368i8(i8 addrspace(2490368)*, i32, i32, i1) #1
declare void @llvm.genx.GenISA.storeraw.indexed.p2490368i8.i8(i8 addrspace(2490368)*, i32, i8, i32, i1) #2

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #1 = { argmemonly nounwind readonly }
attributes #2 = { argmemonly nounwind writeonly }

!igc.functions = !{!3}
!IGCMetadata = !{!15}

!3 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32, i32, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !11, !13, !14}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 15, !10}
!10 = !{!"explicit_arg_num", i32 0}
!11 = !{i32 15, !12}
!12 = !{!"explicit_arg_num", i32 1}
!13 = !{i32 53, !10}
!14 = !{i32 53, !12}
!15 = !{!"ModuleMD", !17, !92}
!17 = !{!"compOpt", !57, !58}
!57 = !{!"UseBindlessMode", i1 true}
!58 = !{!"UseLegacyBindlessMode", i1 false}
!92 = !{!"FuncMD", !93, !94}
!93 = !{!"FuncMDMap[0]", void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32, i32, i32)* @test}
!94 = !{!"FuncMDValue[0]", !101,  !123, !171}
!101 = !{!"functionType", !"KernelFunction"}
!123 = !{!"resAllocMD", !124, !125, !126, !127, !142}
!124 = !{!"uavsNumType", i32 6}
!125 = !{!"srvsNumType", i32 0}
!126 = !{!"samplersNumType", i32 0}
!127 = !{!"argAllocMDList", !128, !132, !134, !137, !138, !139, !140, !141}
!128 = !{!"argAllocMDListVec[0]", !129, !130, !131}
!129 = !{!"type", i32 1}
!130 = !{!"extensionType", i32 -1}
!131 = !{!"indexType", i32 2}
!132 = !{!"argAllocMDListVec[1]", !129, !130, !133}
!133 = !{!"indexType", i32 3}
!134 = !{!"argAllocMDListVec[2]", !135, !130, !136}
!135 = !{!"type", i32 0}
!136 = !{!"indexType", i32 -1}
!137 = !{!"argAllocMDListVec[3]", !135, !130, !136}
!138 = !{!"argAllocMDListVec[4]", !135, !130, !136}
!139 = !{!"argAllocMDListVec[5]", !135, !130, !136}
!140 = !{!"argAllocMDListVec[6]", !135, !130, !136}
!141 = !{!"argAllocMDListVec[7]", !135, !130, !136}
!142 = !{!"inlineSamplersMD"}
!171 = !{!"m_OpenCLArgTypeQualifiers", !172, !173}
!172 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!173 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
