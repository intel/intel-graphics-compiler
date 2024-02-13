;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-lsc-funcs-translation -platformpvc -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LSCFuncsResolution
; ------------------------------------------------

; Test checks that lsc builtins are lowered

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_lsc(i64 %base, <2 x i32> %cord) {
; CHECK-LABEL: @test_lsc(
; CHECK:    [[TMP1:%.*]] = extractelement <2 x i32> %cord, i32 0
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> %cord, i32 1
; CHECK:    call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %base, i32 0, i32 7, i32 0, i32 [[TMP1]], i32 [[TMP2]], i32 32, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:    ret void
;
  %1 = call <4 x i32> @__builtin_IB_subgroup_block_read_flat_u32_wi4_m8k8v1(i64 %base, i32 0, i32 7, i32 0, <2 x i32> %cord)
  ret void
}

declare <4 x i32> @__builtin_IB_subgroup_block_read_flat_u32_wi4_m8k8v1(i64, i32, i32, i32, <2 x i32>)

!igc.functions = !{!0}

!0 = !{void (i64, <2 x i32>)* @test_lsc, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
