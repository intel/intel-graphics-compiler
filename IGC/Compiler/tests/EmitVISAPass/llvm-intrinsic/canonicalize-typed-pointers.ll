;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; RUN: igc_opt -platformdg2 -simd-mode 8 -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks canonicalize intrinsic emit
; CHECK: add (M1_NM, 1) [[A:[A-z0-9]*]](0,0)<1> [[SRC1:[A-z0-9]*]](0,0)<0;1,0> 0x80000000:f
; CHECK: lsc_store.ugm (M1_NM, 1)  flat[{{.*}}]:a64  [[A]]:d32t

define spir_kernel void @test(float %src1, float addrspace(1)* %res, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) {
entry:
  %a = call float @llvm.canonicalize.f32(float %src1)
  store float %a, float addrspace(1)* %res, align 4
  ret void
}

declare float @llvm.canonicalize.f32(float)

!IGCMetadata = !{!0}
!igc.functions = !{!311}

!0 = !{!"ModuleMD", !67,!2}
!2 = !{!"compOpt", !10, !11}
!67 = !{!"FuncMD", !68, !69}
!68 = !{!"FuncMDMap[0]", void (float, float addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @test}
!69 = !{!"FuncMDValue[0]", !101, !149, !326}
!101 = !{!"resAllocMD", !105}
!105 = !{!"argAllocMDList", !106, !110, !111, !114, !115, !116}
!106 = !{!"argAllocMDListVec[0]", !107, !108, !109}
!107 = !{!"type", i32 0}
!108 = !{!"extensionType", i32 -1}
!109 = !{!"indexType", i32 -1}
!110 = !{!"argAllocMDListVec[1]", !107, !108, !109}
!111 = !{!"argAllocMDListVec[2]", !112, !108, !113}
!112 = !{!"type", i32 1}
!113 = !{!"indexType", i32 0}
!114 = !{!"argAllocMDListVec[3]", !107, !108, !109}
!115 = !{!"argAllocMDListVec[4]", !107, !108, !109}
!116 = !{!"argAllocMDListVec[5]", !107, !108, !109}
!149 = !{!"m_OpenCLArgTypeQualifiers", !150, !151, !152}
!150 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!151 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!152 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!311 = !{void (float, float addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @test, !312}
!312 = !{!313}
!313 = !{!"function_type", i32 0}
!10 = !{!"FloatRoundingMode", i32 1}
!11 = !{!"FloatCvtIntRoundingMode", i32 3}
!319 = !{!"argId", i32 0}
!320 = !{!"implicitArgInfoListVec[0]", !319}
!321 = !{!"argId", i32 1}
!322 = !{!"implicitArgInfoListVec[1]", !321}
!323 = !{!"argId", i32 15}
!324 = !{!"explicitArgNum", i32 2}
!325 = !{!"implicitArgInfoListVec[2]", !323, !324}
!326 = !{!"implicitArgInfoList", !320, !322, !325}
