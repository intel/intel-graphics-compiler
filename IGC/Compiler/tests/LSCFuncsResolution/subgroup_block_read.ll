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
; CHECK:    call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %base, i32 0, i32 7, i32 0, i32 [[TMP1]], i32 [[TMP2]], i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  %1 = call <32 x i16> @__builtin_IB_subgroup_block_read_flat_u16_m16k16v2(i64 %base, i32 0, i32 7, i32 0, <2 x i32> %cord)

; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> %cord, i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <2 x i32> %cord, i32 1
; CHECK:    call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %base, i32 0, i32 7, i32 0, i32 [[TMP3]], i32 [[TMP4]], i32 16, i32 16, i32 32, i32 1, i1 false, i1 false, i32 0)
  %2 = call <32 x i16> @__builtin_IB_subgroup_block_read_flat_u16_m32k16v1(i64 %base, i32 0, i32 7, i32 0, <2 x i32> %cord)

; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> %cord, i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> %cord, i32 1
; CHECK:    call <64 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v64i16(i64 %base, i32 0, i32 7, i32 0, i32 [[TMP5]], i32 [[TMP6]], i32 16, i32 16, i32 32, i32 2, i1 false, i1 false, i32 0)
  %3 = call <64 x i16> @__builtin_IB_subgroup_block_read_flat_u16_m32k16v2(i64 %base, i32 0, i32 7, i32 0, <2 x i32> %cord)

; CHECK:    [[TMP7:%.*]] = extractelement <2 x i32> %cord, i32 0
; CHECK:    [[TMP8:%.*]] = extractelement <2 x i32> %cord, i32 1
; CHECK:    call <4 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v4i32(i64 %base, i32 0, i32 7, i32 0, i32 [[TMP7]], i32 [[TMP8]], i32 32, i32 8, i32 8, i32 1, i1 false, i1 false, i32 0)
  %4 = call <4 x i32> @__builtin_IB_subgroup_block_read_flat_u32_wi4_m8k8v1(i64 %base, i32 0, i32 7, i32 0, <2 x i32> %cord)

; CHECK:    [[TMP9:%.*]] = extractelement <2 x i32> %cord, i32 0
; CHECK:    [[TMP10:%.*]] = extractelement <2 x i32> %cord, i32 1
; CHECK:    call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 0, i32 7, i32 0, i32 [[TMP9]], i32 [[TMP10]], i32 16, i32 16, i32 16, i32 2, i1 false, i1 true, i32 0)
  %5 = call <16 x i16> @__builtin_IB_subgroup_block_read_flat_transform_u16_k16v2(i64 %base, i32 0, i32 7, i32 0, <2 x i32> %cord)

; CHECK:    [[TMP11:%.*]] = extractelement <2 x i32> %cord, i32 0
; CHECK:    [[TMP12:%.*]] = extractelement <2 x i32> %cord, i32 1
; CHECK:    call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 0, i32 7, i32 0, i32 [[TMP11]], i32 [[TMP12]], i32 16, i32 16, i32 32, i32 1, i1 false, i1 true, i32 0)
  %6 = call <16 x i16> @__builtin_IB_subgroup_block_read_flat_transform_u16_k32(i64 %base, i32 0, i32 7, i32 0, <2 x i32> %cord)

; CHECK:    [[TMP13:%.*]] = extractelement <2 x i32> %cord, i32 0
; CHECK:    [[TMP14:%.*]] = extractelement <2 x i32> %cord, i32 1
; CHECK:    call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %base, i32 0, i32 7, i32 0, i32 [[TMP13]], i32 [[TMP14]], i32 16, i32 16, i32 32, i32 2, i1 false, i1 true, i32 0)
  %7 = call <32 x i16> @__builtin_IB_subgroup_block_read_flat_transform_u16_k32v2(i64 %base, i32 0, i32 7, i32 0, <2 x i32> %cord)

; CHECK:    ret void
  ret void
}

declare <32 x i16> @__builtin_IB_subgroup_block_read_flat_u16_m16k16v2(i64, i32, i32, i32, <2 x i32>)
declare <32 x i16> @__builtin_IB_subgroup_block_read_flat_u16_m32k16v1(i64, i32, i32, i32, <2 x i32>)
declare <64 x i16> @__builtin_IB_subgroup_block_read_flat_u16_m32k16v2(i64, i32, i32, i32, <2 x i32>)

declare <4 x i32>  @__builtin_IB_subgroup_block_read_flat_u32_wi4_m8k8v1(i64, i32, i32, i32, <2 x i32>)

declare <16 x i16> @__builtin_IB_subgroup_block_read_flat_transform_u16_k16v2(i64, i32, i32, i32, <2 x i32>)
declare <16 x i16> @__builtin_IB_subgroup_block_read_flat_transform_u16_k32(i64, i32, i32, i32, <2 x i32>)
declare <32 x i16> @__builtin_IB_subgroup_block_read_flat_transform_u16_k32v2(i64, i32, i32, i32, <2 x i32>)

!igc.functions = !{!0}

!0 = !{void (i64, <2 x i32>)* @test_lsc, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
