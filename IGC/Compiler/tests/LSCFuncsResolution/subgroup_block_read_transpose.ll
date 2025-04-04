;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-lsc-funcs-translation -platformpvc -S %s 2>&1  | FileCheck %s
; ------------------------------------------------
; LSCFuncsResolution
; ------------------------------------------------

; Test checks that lsc builtins are lowered

define spir_kernel void @test_lsc_2dblock_read(i64 %base, <2 x i32> %cord) {
; CHECK-LABEL: @test_lsc_2dblock_read(

; CHECK:    [[T1:%.*]] = extractelement <2 x i32> %cord, i32 0
; CHECK:    [[T2:%.*]] = extractelement <2 x i32> %cord, i32 1
; CHECK:    call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %base, i32 512, i32 128, i32 512, i32 [[T1]], i32 [[T2]], i32 32, i32 4, i32 8, i32 1, i1 true, i1 false, i32 0)
  %1 = call <2 x i32> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k4(i64 %base, i32 512, i32 128, i32 512, <2 x i32> %cord, i32 0)

; CHECK:    [[T3:%.*]] = extractelement <2 x i32> %cord, i32 0
; CHECK:    [[T4:%.*]] = extractelement <2 x i32> %cord, i32 1
; CHECK:    call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %base, i32 512, i32 128, i32 512, i32 [[T3]], i32 [[T4]], i32 32, i32 8, i32 8, i32 1, i1 true, i1 false, i32 0)
  %2 = call <4 x i32> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k8(i64 %base, i32 512, i32 128, i32 512, <2 x i32> %cord, i32 0)

; CHECK:    call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %base, i32 512, i32 128, i32 512, i32 {{.*}}, i32 {{.*}}, i32 8, i32 4, i32 32, i32 1, i1 true, i1 false, i32 0)
  %3 = call <4 x i16> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k4(i64 %base, i32 512, i32 128, i32 512, <2 x i32> %cord, i32 0)

; CHECK:    call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 512, i32 128, i32 512, i32 {{.*}}, i32 {{.*}}, i32 8, i32 8, i32 32, i32 1, i1 true, i1 false, i32 0)
  %4 = call <8 x i16> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k8(i64 %base, i32 512, i32 128, i32 512, <2 x i32> %cord, i32 0)

; CHECK:    call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %base, i32 512, i32 128, i32 512, i32 {{.*}}, i32 {{.*}}, i32 16, i32 4, i32 16, i32 1, i1 true, i1 false, i32 0)
  %5 = call <4 x i16> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k4(i64 %base, i32 512, i32 128, i32 512, <2 x i32> %cord, i32 0)

; CHECK:    call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 512, i32 128, i32 512, i32 {{.*}}, i32 {{.*}}, i32 16, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %6 = call <8 x i16> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k8(i64 %base, i32 512, i32 128, i32 512, <2 x i32> %cord, i32 0)

; CHECK:    ret void
  ret void
}

declare <2 x i32> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k4(i64, i32, i32, i32, <2 x i32>, i32)
declare <4 x i32> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k8(i64, i32, i32, i32, <2 x i32>, i32)

; emulated transpose
declare <4 x i16> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k4(i64, i32, i32, i32, <2 x i32>, i32)
declare <8 x i16> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k8(i64, i32, i32, i32, <2 x i32>, i32)
declare <4 x i16> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k4(i64, i32, i32, i32, <2 x i32>, i32)
declare <8 x i16> @__builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k8(i64, i32, i32, i32, <2 x i32>, i32)

!igc.functions = !{!0}

!0 = !{void (i64, <2 x i32>)* @test_lsc_2dblock_read, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
