;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXFloatControlWrapper -march=genx64 \
; RUN: -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @kernel
; CHECK-NEXT: [[VAR1:[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; CHECK-NEXT: [[VAR2:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> [[VAR1]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[VAR3:[^ ]+]] = and i32 [[VAR2]], -1265
; CHECK-NEXT: [[VAR4:[^ ]+]] = or i32 [[VAR3]], 0
; CHECK-NEXT: [[VAR5:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> [[VAR1]], i32 [[VAR4]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[VAR5]])
define dllexport spir_kernel void @kernel(i32 %arg, i64 %privBase) #0 {
  %a = call spir_func float @subroutine3(float 1.0) #3
  call spir_func float @stackcall(float %a) #4
  ret void
}

; CHECK-LABEL: @subroutine1
; CHECK-NOT:   predef
define internal spir_func float @subroutine1(float %a) #1 {
  %b = fadd float %a, 1.0
  ret float %b
}

; CHECK-LABEL: @subroutine2
; CHECK-NEXT: [[VAR1:[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; CHECK-NEXT: [[VAR2:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> [[VAR1]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[VAR3:[^ ]+]] = and i32 [[VAR2]], -1265
; CHECK-NEXT: [[VAR4:[^ ]+]] = or i32 [[VAR3]], 16
; CHECK-NEXT: [[VAR5:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> [[VAR1]], i32 [[VAR4]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[VAR5]])
; CHECK:      [[VAR6:[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; CHECK-NEXT: [[VAR7:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> [[VAR6]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[VAR8:[^ ]+]] = and i32 [[VAR7]], -1265
; CHECK-NEXT: [[VAR9:[^ ]+]] = and i32 [[VAR2]], 1264
; CHECK-NEXT: [[VAR10:[^ ]+]] = or i32 [[VAR8]], [[VAR9]]
; CHECK-NEXT: [[VAR11:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> [[VAR6]], i32 [[VAR10]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[VAR12:[^ ]+]] = call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[VAR11]])
; CHECK-NEXT: ret
define internal spir_func float @subroutine2(float %a) #2 {
  %b = call spir_func float @subroutine1(float %a) #1
  ret float %b
}

; CHECK-LABEL: @subroutine3
; CHECK-NEXT: [[VAR1:[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; CHECK-NEXT: [[VAR2:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> [[VAR1]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[VAR3:[^ ]+]] = and i32 [[VAR2]], -1265
; CHECK-NEXT: [[VAR4:[^ ]+]] = or i32 [[VAR3]], 32
; CHECK-NEXT: [[VAR5:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> [[VAR1]], i32 [[VAR4]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[VAR5]])
; CHECK:      [[VAR6:[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; CHECK-NEXT: [[VAR7:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> [[VAR6]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[VAR8:[^ ]+]] = and i32 [[VAR7]], -1265
; CHECK-NEXT: [[VAR9:[^ ]+]] = and i32 [[VAR2]], 1264
; CHECK-NEXT: [[VAR10:[^ ]+]] = or i32 [[VAR8]], [[VAR9]]
; CHECK-NEXT: [[VAR11:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> [[VAR6]], i32 [[VAR10]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[VAR12:[^ ]+]] = call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[VAR11]])
; CHECK-NEXT: ret
define internal spir_func float @subroutine3(float %a) #3 {
  %b = call spir_func float @subroutine2(float %a) #2
  ret float %b
}

; CHECK-LABEL: @stackcall
; CHECK-NEXT: [[VAR1:[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; CHECK-NEXT: [[VAR2:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> [[VAR1]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[VAR3:[^ ]+]] = and i32 [[VAR2]], -1265
; CHECK-NEXT: [[VAR4:[^ ]+]] = or i32 [[VAR3]], 16
; CHECK-NEXT: [[VAR5:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> [[VAR1]], i32 [[VAR4]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[VAR5]])
; CHECK:      [[VAR6:[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; CHECK-NEXT: [[VAR7:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> [[VAR6]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[VAR8:[^ ]+]] = and i32 [[VAR7]], -1265
; CHECK-NEXT: [[VAR9:[^ ]+]] = and i32 [[VAR2]], 1264
; CHECK-NEXT: [[VAR10:[^ ]+]] = or i32 [[VAR8]], [[VAR9]]
; CHECK-NEXT: [[VAR11:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> [[VAR6]], i32 [[VAR10]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[VAR12:[^ ]+]] = call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[VAR11]])
; CHECK-NEXT: ret
define internal spir_func float @stackcall(float %a) #4 {
  %c = call spir_func float @subroutine2(float %a) #0
  ret float %c
}

attributes #0 = { noinline nounwind "CMGenxMain" }
attributes #1 = { noinline nounwind }
attributes #2 = { noinline nounwind "CMFloatControl"="16" }
attributes #3 = { noinline nounwind "CMFloatControl"="32" }
attributes #4 = { noinline nounwind "CMStackCall" "CMFloatControl"="16" }

!genx.kernels = !{!2}
!genx.kernel.internal = !{!7}

!0 = !{i32 0, i32 0}
!1 = !{}
!2 = !{void (i32, i64)* @kernel, !"kernel", !3, i32 0, !4, !5, !6, i32 0}
!3 = !{i32 2, i32 96}
!4 = !{i32 72, i32 64}
!5 = !{i32 0}
!6 = !{!"buffer_t read_write"}
!7 = !{void (i32, i64)* @kernel, !0, !8, !1, !8}
!8 = !{i32 0, i32 1}
