;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --debugify -igc-code-sinking -inputcs -S < %s | FileCheck %s
; ------------------------------------------------
; CodeSinking
; ------------------------------------------------

; Check that debug intrinsics don't affect instruction count for
; CodeSinking pass
;
; Checker below checks that nothing was sinked.
; Function has 31 + GenISA.CatchAllDebugLine() instructions,
; which are less than CODE_SINKING_MIN_SIZE


define spir_kernel void @test_custom(float %a, float* %b) {
; CHECK-LABEL: @test_custom(
; CHECK:  entry:
; CHECK:    call void @llvm.genx.GenISA.CatchAllDebugLine()
; CHECK:    [[TMP0:%.*]] = load float, float* [[B:%.*]], align 4
; CHECK:    [[TMP1:%.*]] = fcmp ueq float [[A:%.*]], [[TMP0]]
; CHECK:    [[TMP2:%.*]] = fdiv float [[A]], [[TMP0]]
; CHECK:    [[TMP3:%.*]] = fdiv float [[TMP2]], [[TMP0]]
; CHECK:    [[TMP4:%.*]] = fdiv float [[TMP3]], [[TMP3]]
; CHECK:    [[TMP5:%.*]] = fdiv float [[TMP4]], [[TMP3]]
; CHECK:    [[TMP6:%.*]] = fdiv float [[TMP5]], [[TMP5]]
; CHECK:    [[TMP7:%.*]] = fadd float [[TMP6]], [[TMP0]]
; CHECK:    [[TMP8:%.*]] = fdiv float [[TMP7]], [[TMP0]]
; CHECK:    [[TMP9:%.*]] = fadd float [[TMP8]], [[TMP0]]
; CHECK:    [[TMP10:%.*]] = fadd float [[TMP9]], [[TMP0]]
; CHECK:    [[TMP11:%.*]] = fadd float [[TMP10]], [[TMP0]]
; CHECK:    [[TMP12:%.*]] = fadd float [[TMP11]], [[TMP0]]
; CHECK:    [[TMP13:%.*]] = fadd float [[TMP12]], [[TMP0]]
; CHECK:    [[TMP14:%.*]] = fadd float [[TMP13]], [[TMP0]]
; CHECK:    [[TMP15:%.*]] = fadd float [[TMP13]], [[TMP0]]
; CHECK:    [[TMP16:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP17:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP18:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP19:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP20:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP21:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP22:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP23:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP24:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP25:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP26:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[TMP27:%.*]] = fadd float [[TMP14]], [[TMP0]]
; CHECK:    [[SEL:%.*]] = select i1 [[TMP1]], float [[TMP27]], float [[TMP0]]
; CHECK:    store float [[SEL]], float* [[B]], align 4
; CHECK:    ret void
;
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine()
  %0 = load float, float* %b, align 4
  %1 = fcmp ueq float %a, %0
  %2 = fdiv float %a, %0
  %3 = fdiv float %2, %0
  %4 = fdiv float %3, %3
  %5 = fdiv float %4, %3
  %6 = fdiv float %5, %5
  %7 = fadd float %6, %0
  %8 = fdiv float %7, %0
  %9 = fadd float %8, %0
  %10 = fadd float %9, %0
  %11 = fadd float %10, %0
  %12 = fadd float %11, %0
  %13 = fadd float %12, %0
  %14 = fadd float %13, %0
  %15 = fadd float %13, %0
  %16 = fadd float %14, %0
  %17 = fadd float %14, %0
  %18 = fadd float %14, %0
  %19 = fadd float %14, %0
  %20 = fadd float %14, %0
  %21 = fadd float %14, %0
  %22 = fadd float %14, %0
  %23 = fadd float %14, %0
  %24 = fadd float %14, %0
  %25 = fadd float %14, %0
  %26 = fadd float %14, %0
  %27 = fadd float %14, %0
  %sel = select i1 %1, float %27, float %0
  store float %sel, float* %b, align 4
  ret void
}

declare void @llvm.genx.GenISA.CatchAllDebugLine()
