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

; This test verifies stack-related variables initilization in case a kernel
; uses VLA, but it doesn't use stack calls. In such scenario, stack-related
; variables can be initilized in a very limited scope, meaning that only stack
; pointer and frame pointer need to be initilized. Additionally, stack and
; frame pointer don't need to be initilized as predefined %sp and %fp VISA
; variables, they can just be a regular VISA variables.

; CHECK-NOT: .decl SP{{.*}} alias=<%sp, 0>
; CHECK-NOT: .decl FP{{.*}} alias=<%fp, 0>
; CHECK-NOT: .decl ARGV{{.*}} alias=<%arg, 0>
; CHECK-NOT: .decl RETV{{.*}} alias=<%retval, 0>

; CHECK: add (M1_NM, 1) SP(0,0)<1> privateBase(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) FP(0,0)<1> SP(0,0)<0;1,0>

define spir_kernel void @test(ptr addrspace(1) %in, ptr addrspace(1) %out, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase, i32 %bufferOffset, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %vlaSize = load i32, ptr addrspace(1) %in, align 4
  %vlaStackAlloca = call ptr @llvm.genx.GenISA.VLAStackAlloca(i32 0, i32 %vlaSize)
  %lidX = zext i16 %localIdX to i32
  store i32 %lidX, ptr %vlaStackAlloca
  ; ... some other basic blocks here ...
  %val = load i32, ptr %vlaStackAlloca
  store i32 %val, ptr addrspace(1) %out
  ret void
}

declare ptr @llvm.genx.GenISA.VLAStackAlloca(i32, i32)

attributes #0 = { "hasVLA" }

!IGCMetadata = !{!0}
!igc.functions = !{!22}

!0 = !{!"ModuleMD", !1, !21}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test}
!3 = !{!"FuncMDValue[0]", !4, !17}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11, !14, !15, !16, !31, !32}
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
!21 = !{!"isHDCFastClearShader", i1 false}
!22 = !{ptr @test, !23}
!23 = !{!24, !25}
!24 = !{!"function_type", i32 0}
!25 = !{!"implicit_arg_desc", !26, !27, !28, !29}
!26 = !{i32 0}
!27 = !{i32 1}
!28 = !{i32 13}
!29 = !{i32 15, !29}
!30 = !{!"explicit_arg_num", i32 2}
!31 = !{!"argAllocMDListVec[6]", !7, !8, !9}
!32 = !{!"argAllocMDListVec[7]", !7, !8, !9}
