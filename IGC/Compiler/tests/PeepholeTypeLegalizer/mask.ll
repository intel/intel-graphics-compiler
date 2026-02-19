;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-int-type-legalizer -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

; Tests for masks done via combination of trunc/ext instructions.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_i32_to_i32(i32 %input, i32 addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = and i32 %input, 7
; CHECK:         store i32 %0, i32 addrspace(1)* %dst
; CHECK:         ret void
  %0 = trunc i32 %input to i3
  %1 = zext i3 %0 to i32
  store i32 %1, i32 addrspace(1)* %dst
  ret void
}

define spir_kernel void @test_i32_to_i16(i32 %input, i16 addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = and i32 %input, 7
; CHECK:         %1 = trunc i32 %0 to i16
; CHECK:         store i16 %1, i16 addrspace(1)* %dst
; CHECK:         ret void
  %0 = trunc i32 %input to i3
  %1 = zext i3 %0 to i16
  store i16 %1, i16 addrspace(1)* %dst
  ret void
}

define spir_kernel void @test_i64_to_i64(i64 %input, i64 addrspace(1)* %dst) {
entry:
; CHECK-LABEL:       entry:
; CHECK:               [[CASTVEC:%.*]] = bitcast i64 %input to <2 x i32>
; CHECK:               [[IDX0_BEFORE:%.*]] = extractelement <2 x i32> [[CASTVEC]], i64 0
; COM: Mask & 0xFFFFFFFF might be optimized out
; CHECK-PRE-LLVM-14:   [[VEC0:%.*]] = insertelement <2 x i32> undef, i32 [[IDX0_BEFORE]], i64 0
; CHECK:  [[IDX0_AFTER:%.*]] = and i32 [[IDX0_BEFORE]], -1
; CHECK:  [[VEC0:%.*]] = insertelement <2 x i32> undef, i32 [[IDX0_AFTER]], i64 0
; CHECK:               [[IDX1_BEFORE:%.*]] = extractelement <2 x i32> [[CASTVEC]], i64 1
; CHECK:               [[IDX1_AFTER:%.*]] = and i32 [[IDX1_BEFORE]], 31
; CHECK:               [[VEC1:%.*]] = insertelement <2 x i32> [[VEC0]], i32 [[IDX1_AFTER]], i64 1
; CHECK:               [[RESULT:%.*]] = bitcast <2 x i32> [[VEC1]] to i64
; CHECK:               store i64 [[RESULT]], i64 addrspace(1)* %dst
; CHECK:               ret void
  %0 = trunc i64 %input to i37
  %1 = zext i37 %0 to i64
  store i64 %1, i64 addrspace(1)* %dst
  ret void
}

define spir_kernel void @test_i64_to_i8(i64 %input, i8 addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = bitcast i64 %input to <2 x i32>
; CHECK:         %1 = extractelement <2 x i32> %0, i32 0
; CHECK:         %2 = and i32 %1, 7
; CHECK:         %3 = trunc i32 %2 to i8
; CHECK:         store i8 %3, i8 addrspace(1)* %dst
; CHECK:         ret void
  %0 = trunc i64 %input to i3
  %1 = zext i3 %0 to i8
  store i8 %1, i8 addrspace(1)* %dst
  ret void
}

!igc.functions = !{!0, !1}

!0 = !{void (i32, i32 addrspace(1)*)* @test_i32_to_i32, !100}
!1 = !{void (i32, i16 addrspace(1)*)* @test_i32_to_i16, !100}
!2 = !{void (i64, i64 addrspace(1)*)* @test_i64_to_i64, !100}
!3 = !{void (i64, i8 addrspace(1)*)* @test_i64_to_i8, !100}
!100 = !{!101}
!101 = !{!"function_type", i32 0}
