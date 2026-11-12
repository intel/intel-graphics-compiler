;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -simd-mode 32 -platformbmg -igc-emit-visa -regkey DumpVISAASMToConsole,DisableUniformAnalysis < %s | FileCheck %s --check-prefix=SIMD32
; RUN: igc_opt --opaque-pointers -simd-mode 16 -platformbmg -igc-emit-visa -regkey DumpVISAASMToConsole,DisableUniformAnalysis < %s | FileCheck %s --check-prefix=SIMD16

; simdShuffleXor with bit 4 of the xor value set swaps the SIMD16 halves of a
; SIMD32 variable. In SIMD16 that bit selects lanes past the end of the
; subgroup, which wrap around, so only bit 0 is emitted.

; SIMD32: mov (M1_NM, 16) [[TMP:[A-z0-9]*]](0,0)<2> ld(0,1)<2;1,0>
; SIMD32: mov (M1_NM, 16) [[TMP]](0,1)<2> ld(0,0)<2;1,0>
; SIMD32: mov (M1_NM, 16) [[TMPA:[A-z0-9_]*]](0,0)<1> [[TMP]](1,0)<1;1,0>
; SIMD32: mov (M1_NM, 16) [[TMPA]](1,0)<1> [[TMP]](0,0)<1;1,0>
; SIMD32: mov (M1, 32) sd(0,0)<1> [[TMPA]](0,0)<1;1,0>

; SIMD16: mov (M1_NM, 8) [[TMP:[A-z0-9]*]](0,0)<2> ld(0,1)<2;1,0>
; SIMD16: mov (M1_NM, 8) [[TMP]](0,1)<2> ld(0,0)<2;1,0>
; SIMD16-NOT: simdShuffleXorTmp
; SIMD16: mov (M1, 16) sd(0,0)<1> [[TMP]](0,0)<1;1,0>

define spir_kernel void @simdintr(ptr addrspace(1) %res) {
entry:
  %ld = load i32, ptr addrspace(1) %res
  %sd = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %ld, i32 17)
  store i32 %sd, ptr addrspace(1) %res
  ret void
}

; "anyint" - "result"
; "int" - "value"
; "int" - "xor value"
declare i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @simdintr}
!3 = !{!"FuncMDValue[0]", !4, !17}
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
!21 = !{ptr @simdintr, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}
