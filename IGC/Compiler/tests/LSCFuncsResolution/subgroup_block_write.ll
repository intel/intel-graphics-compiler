;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -enable-debugify --igc-lsc-funcs-translation -platformpvc -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LSCFuncsResolution
; ------------------------------------------------

; Test checks that lsc builtins are lowered

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_lsc(i64 %base_dst, <2 x i32> %coord, <8 x i32> %val_to_store) {
; CHECK-LABEL: @test_lsc(

; CHECK: [[XOFF0:%.*]] = extractelement <2 x i32> %coord, i32 0
; CHECK: [[YOFF0:%.*]] = extractelement <2 x i32> %coord, i32 1
; CHECK: call void  @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %base_dst, i32 127, i32 7, i32 127, i32 [[XOFF0]], i32 [[YOFF0]], i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %val_to_store)
  call void @__builtin_IB_subgroup_block_write_flat_u32_wi8_m8k16v1(i64 %base_dst, i32 127, i32 7, i32 127, <2 x i32> %coord, <8 x i32> %val_to_store)

; CHECK: [[XOFF1:%.*]] = extractelement <2 x i32> %coord, i32 0
; CHECK: [[YOFF1:%.*]] = extractelement <2 x i32> %coord, i32 1
; CHECK: call void  @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %base_dst, i32 127, i32 7, i32 127, i32 [[XOFF1]], i32 [[YOFF1]], i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 2, <8 x i32> %val_to_store)
  call void @__builtin_IB_subgroup_block_write_flat_cacheopts_u32_wi8_m8k16v1(i64 %base_dst, i32 127, i32 7, i32 127, <2 x i32> %coord, <8 x i32> %val_to_store, i32 2)

; CHECK:    ret void
  ret void;
}

declare void @__builtin_IB_subgroup_block_write_flat_u32_wi8_m8k16v1(i64, i32, i32, i32, <2 x i32>, <8 x i32>)
declare void @__builtin_IB_subgroup_block_write_flat_cacheopts_u32_wi8_m8k16v1(i64, i32, i32, i32, <2 x i32>, <8 x i32>, i32)

!igc.functions = !{!0}
!0 = !{void (i64, <2 x i32>, <8 x i32>)* @test_lsc, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
