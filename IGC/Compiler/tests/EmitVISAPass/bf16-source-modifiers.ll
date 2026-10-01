;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys

; RUN: igc_opt -platformdg2 -opaque-pointers -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks if source modifiers on bfloats (abs, neg, -abs) are properly
; emulated on platforms without native bfloat support

declare float @llvm.fabs.f32(float)
declare bfloat @llvm.fabs.bf16(bfloat)

; fabs: mov with (abs) modifier for f32, and with 0x7fff for bf16
define spir_kernel void @test_fabs(float %a, bfloat %b, ptr %ptr1, ptr %ptr2) {
; CHECK: mov (M1_NM, 1) [[REG1:[A-z0-9]*]](0,0)<1> (abs)[[A:[A-z0-9]*]](0,0)<0;1,0>
  %1 = call float @llvm.fabs.f32(float %a)
; CHECK: and (M1_NM, 1) [[REG2:[A-z0-9]*]](0,0)<1> [[B:[A-z0-9]*]](0,0)<0;1,0> 0x7fff:uw
  %2 = call bfloat @llvm.fabs.bf16(bfloat %b)
  store float %1, ptr %ptr1
  store bfloat %2, ptr %ptr2
  ret void
}

; fneg: mov with (-) modifier for f32, xor with 0x8000 for bf16
define spir_kernel void @test_fneg(float %a, bfloat %b, ptr %ptr1, ptr %ptr2) {
; CHECK: mov (M1_NM, 1) [[REG3:[A-z0-9]*]](0,0)<1> (-)[[C:[A-z0-9]*]](0,0)<0;1,0>
  %1 = fneg float %a
; CHECK: xor (M1_NM, 1) [[REG4:[A-z0-9]*]](0,0)<1> [[D:[A-z0-9]*]](0,0)<0;1,0> 0x8000:uw
  %2 = fneg bfloat %b
  store float %1, ptr %ptr1
  store bfloat %2, ptr %ptr2
  ret void
}

; fneg(fabs): mov with (-abs) modifier for f32, or with 0x8000 for bf16
define spir_kernel void @test_negabs(float %a, bfloat %b, ptr %ptr1, ptr %ptr2) {
; CHECK: mov (M1_NM, 1) [[REG5:[A-z0-9]*]](0,0)<1> (-abs)[[E:[A-z0-9]*]](0,0)<0;1,0>
  %abs_f32 = call float @llvm.fabs.f32(float %a)
  %negabs_f32 = fneg float %abs_f32
  store float %negabs_f32, ptr %ptr1
; CHECK: or (M1_NM, 1) [[REG6:[A-z0-9]*]](0,0)<1> [[F:[A-z0-9]*]](0,0)<0;1,0> 0x8000:uw
  %abs_bf16 = call bfloat @llvm.fabs.bf16(bfloat %b)
  %negabs_bf16 = fneg bfloat %abs_bf16
  store bfloat %negabs_bf16, ptr %ptr2
  ret void
}


!IGCMetadata = !{!0}
!igc.functions = !{!20, !21, !22}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !8, !9, !16, !17}
!2 = !{!"FuncMDMap[0]", void (float, bfloat, ptr, ptr)* @test_fabs}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6}
!6 = !{!"argAllocMDListVec[0]", !7, !13, !14}
!7 = !{!"type", i32 0}
!8 = !{!"FuncMDMap[1]", void (float, bfloat, ptr, ptr)* @test_fneg}
!9 = !{!"FuncMDValue[1]", !4}
!13 = !{!"extensionType", i32 -1}
!14 = !{!"indexType", i32 -1}
!16 = !{!"FuncMDMap[2]", void (float, bfloat, ptr, ptr)* @test_negabs}
!17 = !{!"FuncMDValue[2]", !4}
!20 = !{void (float, bfloat, ptr, ptr)* @test_fabs, !11}
!21 = !{void (float, bfloat, ptr, ptr)* @test_fneg, !11}
!22 = !{void (float, bfloat, ptr, ptr)* @test_negabs, !11}
!11 = !{!15}
!15 = !{!"function_type", i32 0}
