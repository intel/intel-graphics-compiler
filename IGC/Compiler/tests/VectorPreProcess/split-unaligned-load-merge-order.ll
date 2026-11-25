;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers %s -S -o - -ocl -platformbmg -igc-vectorpreprocess | FileCheck %s
; REQUIRES: llvm-16-plus

; A non-uniform byte-aligned load of 64-bit elements is split into dword loads, which are merged back into
; the elements. A store that isn't split needs the whole vector, so it is rebuilt from the merged elements,
; which must be defined before it.

define spir_kernel void @test(ptr addrspace(1) %in, ptr addrspace(1) %out) {
; CHECK-LABEL: @test(
; CHECK:         [[LO:%.*]] = load i32, ptr addrspace(1) {{%.*}}, align 1
; CHECK:         [[HI:%.*]] = load i32, ptr addrspace(1) {{%.*}}, align 1
; CHECK:         [[V0:%.*]] = insertelement <2 x i32> undef, i32 [[LO]], i64 0
; CHECK:         [[V1:%.*]] = insertelement <2 x i32> [[V0]], i32 [[HI]], i64 1
; CHECK:         [[ELT:%.*]] = bitcast <2 x i32> [[V1]] to i64
; CHECK:         [[VEC:%.*]] = insertelement <1 x i64> undef, i64 [[ELT]], i32 0
; CHECK:         store <1 x i64> [[VEC]], ptr addrspace(1) %out, align 8
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %idx = zext i16 %lane to i64
  %src = getelementptr i8, ptr addrspace(1) %in, i64 %idx
  %v = load <1 x i64>, ptr addrspace(1) %src, align 1
  store <1 x i64> %v, ptr addrspace(1) %out, align 8
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
