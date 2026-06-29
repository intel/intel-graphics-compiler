;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; RUN: igc_opt -platformdg2 -igc-emit-visa -simd-mode 16 -regkey DumpVISAASMToConsole,ForceAddingStackcallKernelPrerequisites < %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks that %sp is saved to temp variable and then restored.

; CHECK: .decl [[SP:[A-z0-9]*]] v_type=G type=ud num_elts=1 align=hword alias=<%sp, 0>
; CHECK: .decl [[FP:[A-z0-9]*]] v_type=G type=ud num_elts=1 align=hword alias=<%fp, 0>
; COM: move %sp to %fp
; CHECK: mov (M1_NM, 1) [[FP]](0,0)<1> [[SP]](0,0)<0;1,0>
; COM: move %sp to tmp_var
; CHECK: mov (M1_NM, 1) [[TMP_VAR:[A-z0-9]*]](0,0)<1> [[SP]](0,0)<0;1,0>
; CHECK: lsc_store.ugm (M1_NM, 1)
; COM: restore %sp from tmp_var
; CHECK: mov (M1_NM, 1) [[SP]](0,0)<1> [[TMP_VAR]](0,0)<0;1,0>

define spir_kernel void @fcall(<16 x i32> %s1, <16 x i32> addrspace(2490368)* %res, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) {
entry:
  %stack = call i8* @llvm.stacksave()
  store <16 x i32> %s1, <16 x i32> addrspace(2490368)* %res, align 8
  call void @llvm.stackrestore(i8* %stack)
  ret void
}

declare i8* @llvm.stacksave()
declare void @llvm.stackrestore(i8*)

!IGCMetadata = !{!0}
!igc.functions = !{!24}

!0 = !{!"ModuleMD", !1, !21}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (<16 x i32>, <16 x i32> addrspace(2490368)*, <8 x i32>, <8 x i32>, i32)* @fcall}
!3 = !{!"FuncMDValue[0]", !4, !17, !42}
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
!22 = !{!"PrivateMemoryPerFGMap[0]", void (<16 x i32>, <16 x i32> addrspace(2490368)*, <8 x i32>, <8 x i32>, i32)* @fcall}
!23 = !{!"PrivateMemoryPerFGValue[0]", i32 0}
!24 = !{void (<16 x i32>, <16 x i32> addrspace(2490368)*, <8 x i32>, <8 x i32>, i32)* @fcall, !25}
!25 = !{!26}
!26 = !{!"function_type", i32 0}
!33 = !{!"argId", i32 0}
!34 = !{!"implicitArgInfoListVec[0]", !33}
!35 = !{!"argId", i32 1}
!36 = !{!"implicitArgInfoListVec[1]", !35}
!37 = !{!"argId", i32 13}
!38 = !{!"implicitArgInfoListVec[2]", !37}
!39 = !{!"argId", i32 15}
!40 = !{!"explicitArgNum", i32 2}
!41 = !{!"implicitArgInfoListVec[3]", !39, !40}
!42 = !{!"implicitArgInfoList", !34, !36, !38, !41}
