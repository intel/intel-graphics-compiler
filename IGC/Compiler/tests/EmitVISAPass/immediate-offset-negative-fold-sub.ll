;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformNvl -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; RUN: igc_opt --opaque-pointers -platformNvl -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey LscImmOffsMatch=0 | FileCheck %s --check-prefix=NOFOLD
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; This test checks that CodeGenPatternMatch::MatchImmOffsetLSC negates the folded
; immediate offset when the address expression is a sub. Folding "var - imm" as
; [var + imm] when imm was negative would place the access 2*imm bytes past the intended address.
;
; The add case is covered alongside it to show the pre-existing "var + imm" path
; is unchanged, i.e. only the sub sign was affected by the fix.

define spir_kernel void @test(ptr addrspace(1) %out0, ptr addrspace(1) %out1, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %0 = zext i16 %localIdX to i32

  ; "var - imm" must fold to a NEGATIVE immediate offset; a dropped negation
  ; would emit +0x10 instead of -0x10.
  %1 = add nuw nsw i32 %0, 256
  %2 = sub nuw nsw i32 %1, 16
  %3 = inttoptr i32 %2 to ptr addrspace(3)
; CHECK: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{.*}}-0x10]:a32
  %4 = load i32, ptr addrspace(3) %3, align 4
  store i32 %4, ptr addrspace(1) %out0

  ; "var + imm" folds to a POSITIVE immediate offset (pre-existing path).
  %5 = add nuw nsw i32 %0, 16
  %6 = inttoptr i32 %5 to ptr addrspace(3)
; CHECK: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{.*}}+0x10]:a32
  %7 = load i32, ptr addrspace(3) %6, align 4
  store i32 %7, ptr addrspace(1) %out1

  ; With matching disabled nothing is folded, so both addresses are materialized
  ; and no immediate offset appears on either load.
; NOFOLD: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{V[0-9]+}}]:a32
; NOFOLD: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{V[0-9]+}}]:a32
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!22}

!0 = !{!"ModuleMD", !1, !21}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test}
!3 = !{!"FuncMDValue[0]", !4, !17, !37}
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
!21 = !{!"isHDCFastClearShader", i1 false}
!22 = !{ptr @test, !23}
!23 = !{!24}
!24 = !{!"function_type", i32 0}
!30 = !{!"argId", i32 0}
!31 = !{!"implicitArgInfoListVec[0]", !30}
!32 = !{!"argId", i32 1}
!33 = !{!"implicitArgInfoListVec[1]", !32}
!34 = !{!"argId", i32 15}
!35 = !{!"explicitArgNum", i32 2}
!36 = !{!"implicitArgInfoListVec[2]", !34, !35}
!37 = !{!"implicitArgInfoList", !31, !33, !36}
