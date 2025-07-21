;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey DisableCodeScheduling=0 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 --igc-code-scheduling \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 -S %s 2>&1 | FileCheck %s



; First check dumps
; CHECK: Function no_dpas
; CHECK-NOT: Scheduling basic block

; CHECK: Function multi_dpas
; CHECK: Scheduling basic block bb1
; CHECK: Commited the schedule
; CHECK: Scheduling basic block bb2
; CHECK: Commited the schedule



; The pass doesn't apply to kernels without dpas instructions

define spir_kernel void @no_dpas(ptr addrspace(1) %A) {
; CHECK-LABEL: @no_dpas(
; CHECK:       entry:
; CHECK:         [[A:%.*]] = add i32 1, 2
; CHECK:         [[B:%.*]] = add i32 3, 4
; CHECK:         [[C:%.*]] = add i32 [[B]], 5
; CHECK:         store i32 [[C]], ptr addrspace(1) [[A:%.*]], align 4
; CHECK:         ret void
;
entry:
  %a = add i32 1, 2
  %b = add i32 3, 4
  %c = add i32 %b, 5
  store i32 %c, ptr addrspace(1) %A
  ret void
}



; Schedule the instructions for every BB with a dpas instruction separately
; The arithmetic instructions that have dependent values are scheduled earlier to hide latency

define spir_kernel void @multi_dpas(ptr addrspace(1) %A) {
; CHECK-LABEL: @multi_dpas(
; CHECK:       entry:
; CHECK:         br label [[BB1:%.*]]
; CHECK:       bb1:
; CHECK:         [[DPAS1:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK:         [[B:%.*]] = add i32 3, 4
; CHECK:         [[C:%.*]] = add i32 [[B]], 5
; CHECK:         [[A:%.*]] = add i32 1, 2
; CHECK:         br label [[BB2:%.*]]
; CHECK:       bb2:
; CHECK:         [[DPAS2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK:         [[B1:%.*]] = add i32 8, 9
; CHECK:         [[C1:%.*]] = add i32 [[B1]], 10
; CHECK:         [[D:%.*]] = add i32 [[C]], [[C1]]
; CHECK:         [[A1:%.*]] = add i32 6, 7
; CHECK:         ret void
;
entry:
  br label %bb1

bb1:
  %a = add i32 1, 2
  %b = add i32 3, 4
  %c = add i32 %b, 5
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
  i32 1, i32 1, i32 1, i32 1, i1 false)
  br label %bb2

bb2:
  %a1 = add i32 6, 7
  %b1 = add i32 8, 9
  %c1 = add i32 %b1, 10
  %d = add i32 %c, %c1
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
  i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
