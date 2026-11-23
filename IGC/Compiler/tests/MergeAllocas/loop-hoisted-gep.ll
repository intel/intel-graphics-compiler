;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:32:32-v96:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

; RUN: igc_opt --opaque-pointers -S --platformbmg --igc-ocl-merge-allocas %s | FileCheck %s

; Two arrays live in disjoint parts of a loop body, but their loop-invariant
; addresses were hoisted into the preheader. Address computations do not touch
; the memory, so they must not keep the arrays alive over the whole loop.

define void @test(i32 %i) #0 {
; CHECK: alloca [16 x i32]
; CHECK-NOT: alloca
  %alloca1 = alloca [16 x i32]
  %alloca2 = alloca [16 x i32]
  br label %looppreheader

looppreheader:
  %gep1 = getelementptr i8, ptr %alloca1, i32 -4
  %gep2 = getelementptr i8, ptr %alloca2, i32 -4
  br label %loopheader

loopheader:
  br label %loopbody1

loopbody1:
  %p1 = getelementptr i32, ptr %gep1, i32 %i
  store i32 9, ptr %p1
  %a = load i32, ptr %p1
  br label %loopbody2

loopbody2:
  %p2 = getelementptr i32, ptr %gep2, i32 %i
  store i32 10, ptr %p2
  %b = load i32, ptr %p2
  br i1 false, label %loopheader, label %loopexit

loopexit:
  ret void
}

attributes #0 = { nounwind readnone }
