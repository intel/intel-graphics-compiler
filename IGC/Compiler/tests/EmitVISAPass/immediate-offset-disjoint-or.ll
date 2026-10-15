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

; This test checks that CodeGenPatternMatch::MatchImmOffsetLSC folds the
; immediate of an "or" into the LSC immediate offset when the variable operand
; has no bit of the immediate set. InstCombine produces such an "or" from an
; "add" of disjoint bit fields, for example when a lane-dependent index and a
; constant offset select different bits of an SLM address. The "or" then
; computes the same address as the "add". With LscImmOffsMatch=0 no access
; has an immediate offset.

define spir_kernel void @test(ptr addrspace(1) %out0, ptr addrspace(1) %out1, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %0 = zext i16 %localIdX to i32

  ; %1 uses bits 4..6 only, and 128 is bit 7: "%1 | 128" is "%1 + 128".
  %1 = and i32 %0, 112
  %2 = or i32 %1, 128
  %3 = inttoptr i32 %2 to ptr addrspace(3)
; CHECK: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{[A-Za-z0-9_]+}}+0x80]:a32
; NOFOLD: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{V[0-9]+}}]:a32
  %4 = load i32, ptr addrspace(3) %3, align 4

  ; The same with the immediate as the first operand.
  %5 = or i32 256, %1
  %6 = inttoptr i32 %5 to ptr addrspace(3)
; CHECK: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{[A-Za-z0-9_]+}}+0x100]:a32
; NOFOLD: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{V[0-9]+}}]:a32
  %7 = load i32, ptr addrspace(3) %6, align 4

  ; %8 can have bit 7 set, so "%8 | 128" is not "%8 + 128" and is not folded.
  %8 = and i32 %0, 240
  %9 = or i32 %8, 128
  %10 = inttoptr i32 %9 to ptr addrspace(3)
; CHECK: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{V[0-9]+}}]:a32
; NOFOLD: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{V[0-9]+}}]:a32
  %11 = load i32, ptr addrspace(3) %10, align 4

  ; A shift clears the low bits of the variable operand: "%12 | 16" is
  ; "%12 + 16". The store takes the same immediate offset path as the load.
  %12 = shl i32 %1, 3
  %13 = or i32 %12, 16
  %14 = inttoptr i32 %13 to ptr addrspace(3)
; CHECK: lsc_store.slm (M1_NM, 1)  flat[{{[A-Za-z0-9_]+}}+0x10]:a32  {{V[0-9]+}}:d32t
; NOFOLD: lsc_store.slm (M1_NM, 1)  flat[{{V[0-9]+}}]:a32  {{V[0-9]+}}:d32t
  store i32 %4, ptr addrspace(3) %14, align 4

  %15 = add i32 %7, %11
  store i32 %15, ptr addrspace(1) %out0
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
