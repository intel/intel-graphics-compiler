;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey PrintToConsole=1 -CoalescingEngine -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; CoalescingEngine
; ------------------------------------------------

; Test checks that CCTuple is created, and registered within list.

; CHECK: CC Tuple list:
; CHECK: CC Tuple 0 : 2 : 1 : CC Tuple list end.

define spir_kernel void @test_coal( i8* %a, i32 %b) {
entry:
  %0 = call i32 @llvm.genx.GenISA.DCL.input.i32(i32 1, i32 1)
  %1 = call <16 x i32> @llvm.genx.GenISA.ldptr.16i32(i32 %0, i32 1, i32 3, i32 4, i8* %a, i8* %a, i32 0, i32 1, i32 2)
  %2 = call <16 x i32> @llvm.genx.GenISA.ldptr.16i32(i32 1, i32 %0, i32 %0, i32 %0, i8* %a, i8* %a, i32 0, i32 1, i32 2)
  ret void
}

declare <16 x i32> @llvm.genx.GenISA.ldptr.16i32(i32, i32, i32, i32, i8*, i8*, i32, i32, i32)
declare i32 @llvm.genx.GenISA.DCL.input.i32(i32, i32)

!igc.functions = !{!0}
!0 = !{void (i8*, i32)* @test_coal, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
