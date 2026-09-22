;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #0

define void @convergent_shfl_skip(i1 %c, i32 %val, i32 %lane, ptr addrspace(1) %out) {
; CHECK-LABEL: define void @convergent_shfl_skip(
; CHECK:       entry:
; CHECK:       br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK:       call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32
; CHECK:       br label %merge0
; CHECK:       merge0:
; CHECK:       %p0 = phi i32 [ %s0, %body0 ], [ 0, %entry ]
; CHECK:       br i1 %c, label %body1, label %merge1
; CHECK:       body1:
; CHECK:       call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32
; CHECK:       br label %merge1
; CHECK:       merge1:
; CHECK:       %p1 = phi i32 [ %s1, %body1 ], [ %p0, %merge0 ]
; CHECK:       store i32 %p1, ptr addrspace(1) %out, align 4
; CHECK:       ret void
; CHECK-NOT:   .dcm

entry:
  br i1 %c, label %body0, label %merge0

body0:
  %s0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %val, i32 %lane, i32 0)
  br label %merge0

merge0:
  %p0 = phi i32 [ %s0, %body0 ], [ 0, %entry ]
  br i1 %c, label %body1, label %merge1

body1:
  %s1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %val, i32 %lane, i32 0)
  br label %merge1

merge1:
  %p1 = phi i32 [ %s1, %body1 ], [ %p0, %merge0 ]
  store i32 %p1, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent }
