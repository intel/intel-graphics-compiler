;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s

; The speculation allow-list admits a curated set of pure, non-convergent GenISA
; intrinsics that exist at this pass's pipeline stage (it runs before Emu64 / FP
; legalization): RuntimeValue (a push constant / uniform kernel-arg load -- the
; original || -> select case), simdLaneId / simdSize (lane & dispatch identity,
; independent of the active mask), and bfrev (single-op bit reverse). Each is
; hoisted, so its triangle linearizes to a select.

; RuntimeValue is hoisted; the merge PHI becomes a select.
define i32 @test_speculate_runtimevalue(i1 %c, i32 %a) {
; CHECK-LABEL: define i32 @test_speculate_runtimevalue(
; CHECK:         call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 8)
; CHECK:         select i1 %c
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %rv = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 8)
  %v = add i32 %rv, %a
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %v, %arm ], [ %a, %entry ]
  ret i32 %r
}

; simdLaneId (lane identity, i16) is hoisted; the zext of its result is cheap too.
define i32 @test_speculate_simdlaneid(i1 %c, i32 %a) {
; CHECK-LABEL: define i32 @test_speculate_simdlaneid(
; CHECK:         call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:         select i1 %c
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %l = call i16 @llvm.genx.GenISA.simdLaneId()
  %z = zext i16 %l to i32
  %v = add i32 %z, %a
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %v, %arm ], [ %a, %entry ]
  ret i32 %r
}

; simdSize (dispatch width, uniform) is hoisted.
define i32 @test_speculate_simdsize(i1 %c, i32 %a) {
; CHECK-LABEL: define i32 @test_speculate_simdsize(
; CHECK:         call i32 @llvm.genx.GenISA.simdSize()
; CHECK:         select i1 %c
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %s = call i32 @llvm.genx.GenISA.simdSize()
  %v = add i32 %s, %a
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %v, %arm ], [ %a, %entry ]
  ret i32 %r
}

; bfrev (single-op bit reverse) is hoisted.
define i32 @test_speculate_bfrev(i1 %c, i32 %a) {
; CHECK-LABEL: define i32 @test_speculate_bfrev(
; CHECK:         call i32 @llvm.genx.GenISA.bfrev
; CHECK:         select i1 %c
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %b = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %a)
  %v = add i32 %b, %a
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %v, %arm ], [ %a, %entry ]
  ret i32 %r
}

; Negative: an extended-math intrinsic (native tanh) is readnone/nounwind/willreturn
; but NOT on the allow-list -- it is typically emitted behind a deliberate input
; guard (here isnan(x) ? NaN : tanh(x)). It is not speculated: the branch, the
; guarded call, and the PHI survive and no select is formed.
define float @test_no_speculate_math(float %x) {
; CHECK-LABEL: define float @test_no_speculate_math(
; CHECK:         br i1 %c
; CHECK:         call float @llvm.genx.GenISA.tanh.f32(float %x)
; CHECK:         phi float
; CHECK-NOT:     select
entry:
  %c = fcmp oeq float %x, %x
  br i1 %c, label %call, label %merge

call:                                             ; preds = %entry
  %t = call float @llvm.genx.GenISA.tanh.f32(float %x)
  br label %merge

merge:                                            ; preds = %entry, %call
  %r = phi float [ %t, %call ], [ 0.000000e+00, %entry ]
  ret float %r
}

; Negative: a convergent wave op inside a speculated arm must NOT be hoisted.
; classifySuccessor has a dedicated convergent backstop that rejects the arm
; regardless of the cost allow-list, because hoisting the op out of the branch can
; change the lane mask it executes under and silently alter its result. So the
; branch, the wave op, and the PHI all survive and no select is formed.
; (WaveClustered is not on the allow-list today; this pins the safety property so
; adding it later cannot silently start flattening.)
define i32 @test_no_speculate_convergent_arm(i1 %c, i32 %a, i32 %b) {
; CHECK-LABEL: define i32 @test_no_speculate_convergent_arm(
; CHECK:         br i1 %c
; CHECK:         call i32 @llvm.genx.GenISA.WaveClustered.i32(
; CHECK:         phi i32
; CHECK-NOT:     select
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %w = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %a, i8 6, i32 8, i32 0)
  %v = add i32 %w, %b
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %v, %arm ], [ %a, %entry ]
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #0
declare i16 @llvm.genx.GenISA.simdLaneId() #0
declare i32 @llvm.genx.GenISA.simdSize() #0
declare i32 @llvm.genx.GenISA.bfrev.i32(i32) #0
declare float @llvm.genx.GenISA.tanh.f32(float) #0
declare i32 @llvm.genx.GenISA.WaveClustered.i32(i32, i8, i32, i32) #1

attributes #0 = { nounwind readnone willreturn }
attributes #1 = { convergent inaccessiblememonly nounwind }
