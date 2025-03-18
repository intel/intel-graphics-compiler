;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Starting with Xe2, hardware is able to handle the following load:
;
; load [VAR+IMM]
;
; Formerly emitting an add instruction would be required:
;
; add tmp VAR IMM
; load [tmp]
;
; However, HW does an early bounds check on VAR. Thus, if VAR is negative,
; then the bounds check fails early even though the immediate offset would
; bring the final calculation to a positive number.
;
; This shows up when one indexes from the top of an SLM allocation:
;
; float slm[IMM];
; slm[IMM - var] = ...

; This test verifies whether immediate offset pattern match is ONLY applied
; when VAR is proven to be a positive value.

define spir_kernel void @test(i32 addrspace(1)* %out0, i32 addrspace(1)* %out1, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = zext i16 %localIdY to i32

  %2 = shl nuw nsw i32 %1, 6
  %3 = shl nuw nsw i32 %0, 2
  %4 = sub i32 0, %3
  %5 = sub i32 %4, %2
  %6 = add nsw i32 %5, 1020
  %7 = inttoptr i32 %6 to i32 addrspace(3)*
; COM: VAR (%5) is a negative value, so immediate global offset cannot be applied
; CHECK-NOT: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{V[0-9]+}}+0x3FC]:a32
; CHECK:     lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{V[0-9]+}}]:a32
  %8 = load i32, i32 addrspace(3)* %7, align 4
  store i32 %8, i32 addrspace(1)* %out0

  ; Test positive var offset
  %9 = add nuw nsw i32 %0, 256
  %10 = inttoptr i32 %9 to i32 addrspace(3)*
; COM: VAR (%0) is a positive value, so immediate global offset can be applied
; CHECK: lsc_load.slm (M1_NM, 1)  {{V[0-9]+}}:d32t  flat[{{.*}}+0x100]:a32
  %11 = load i32, i32 addrspace(3)* %10, align 4
  store i32 %11, i32 addrspace(1)* %out1
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!22}

!0 = !{!"ModuleMD", !1, !21}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i32, i16, i16, i16)* @test}
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
!21 = !{!"isHDCFastClearShader", i1 false}
!22 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i32, i16, i16, i16)* @test, !23}
!23 = !{!24, !25}
!24 = !{!"function_type", i32 0}
!25 = !{!"implicit_arg_desc", !26, !27, !28}
!26 = !{i32 0}
!27 = !{i32 1}
!28 = !{i32 15, !29}
!29 = !{!"explicit_arg_num", i32 2}
