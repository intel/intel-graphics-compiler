;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; RUN: igc_opt -platformdg2 -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks that llvm.sqrt.f32 call is emited as sqrt

; CHECK: sqrt (M1_NM, 1) [[SQRT:[A-z0-9]*]](0,0)<1> [[A:[A-z0-9]*]](0,0)<0;1,0>
; CHECK: lsc_store.ugm (M1_NM, 1) {{.*}} [[SQRT]]:d32

define spir_kernel void @intrinsic_sqrt(float %a, float addrspace(1)* %res32, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) {
entry:
  %sqrt32 = call float @llvm.sqrt.f32(float %a)
  store float %sqrt32, float addrspace(1)* %res32
  ret void
}

declare float @llvm.sqrt.f32(float) #5
declare double @llvm.sqrt.f64(double) #5

attributes #5 = { readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (float, float addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @intrinsic_sqrt}
!3 = !{!"FuncMDValue[0]", !4, !17, !36}
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
!21 = !{void (float, float addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @intrinsic_sqrt, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}
!29 = !{!"argId", i32 0}
!30 = !{!"implicitArgInfoListVec[0]", !29}
!31 = !{!"argId", i32 1}
!32 = !{!"implicitArgInfoListVec[1]", !31}
!33 = !{!"argId", i32 15}
!34 = !{!"explicitArgNum", i32 2}
!35 = !{!"implicitArgInfoListVec[2]", !33, !34}
!36 = !{!"implicitArgInfoList", !30, !32, !35}
