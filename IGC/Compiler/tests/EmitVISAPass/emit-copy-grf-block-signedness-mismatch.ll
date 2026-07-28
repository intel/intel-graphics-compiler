;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; @stack_caller passes a stack-call argument that is 'zext i8 %ld to i32'. On this path
; the source CVariable ends up with an unsigned type (ud) while the formal
; argument type is signed (d) -- same width, different signedness. emitCopyGRFBlock
; must relax its type check to a width check and bitcast the source to the formal
; type before writing it to the ARGV block.
;
; Check that the argument is bitcast to the signed formal type (a 'd' alias of the
; 'ud' source) and that alias is the value written into the ARGV block.


; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -GenXCodeGenModule -platformCri -inputocl \
; RUN:         -igc-emit-visa -simd-mode 16 -regkey DumpVISAASMToConsole %s | FileCheck %s

; CHECK: .global_function "stack_caller"
; CHECK: .decl [[SRC:[A-Za-z0-9_]+]] v_type=G type=ud num_elts=1 align=wordx32{{$}}
; CHECK: .decl [[BC:[A-Za-z0-9_]+]] v_type=G type=d num_elts=1 align=wordx32 alias=<[[SRC]], 0>
; CHECK: mov (M1, 16) {{.*}}(2,0)<1> [[BC]](0,0)<0;1,0>

define spir_kernel void @test(i32 %s1, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) {
entry:
  call spir_func void @stack_caller()
  ret void
}

define internal spir_func void @stack_caller() "visaStackCall" {
  %1 = addrspacecast ptr addrspace(4) null to ptr addrspace(1)
  %2 = load i8, ptr addrspace(1) %1, align 1
  %3 = zext i8 %2 to i32
  call spir_func void null(ptr addrspace(4) null, i32 %3)
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!24, !28}
!0 = !{!"ModuleMD", !1, !21}
!1 = !{!"FuncMD", !2, !3, !50, !51}
!2 = !{!"FuncMDMap[0]", ptr @test}
!3 = !{!"FuncMDValue[0]", !4, !17, !46}
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
!21 = !{!"PrivateMemoryPerFG", !22, !23}
!22 = !{!"PrivateMemoryPerFGMap[0]", ptr @test}
!23 = !{!"PrivateMemoryPerFGValue[0]", i32 0}
!24 = !{ptr @test, !25}
!25 = !{!26}
!26 = !{!"function_type", i32 0}
!28 = !{ptr @stack_caller, !29}
!29 = !{!30}
!30 = !{!"function_type", i32 2}
!46 = !{!"implicitArgInfoList", !38, !40, !42, !45}
!38 = !{!"implicitArgInfoListVec[0]", !37}
!37 = !{!"argId", i32 0}
!40 = !{!"implicitArgInfoListVec[1]", !39}
!39 = !{!"argId", i32 1}
!42 = !{!"implicitArgInfoListVec[2]", !41}
!41 = !{!"argId", i32 13}
!45 = !{!"implicitArgInfoListVec[3]", !43, !44}
!43 = !{!"argId", i32 15}
!44 = !{!"explicitArgNum", i32 2}
!50 = !{!"FuncMDMap[1]", ptr @stack_caller}
!51 = !{!"FuncMDValue[1]", !54}
!54 = !{!"functionType", !"UserFunction"}
