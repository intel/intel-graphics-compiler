;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrologEpilogInsertion -mattr=+ocl_runtime -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S -vc-arg-reg-size=32 \
; RUN: -vc-ret-reg-size=12 < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <8 x float> @llvm.genx.oword.ld.unaligned.v8f32(i32, i32, i32)
declare void @llvm.genx.oword.st.v8f32(i32, i32, <8 x float>)

define dllexport void @test(i32 %arg, <8 x i1> %mask, i32 %x1, i32 %x2, i32 %x3, i32 %x4, i32 %x5, i32 %x6, i64 %x7, i64 %x8) local_unnamed_addr #0 {
entry:
  %a0 = tail call <8 x float> @llvm.genx.oword.ld.unaligned.v8f32(i32 0, i32 1, i32 %arg)
  %a1 = tail call <8 x float> @llvm.genx.oword.ld.unaligned.v8f32(i32 0, i32 2, i32 %arg)
  %a2 = tail call <8 x float> @llvm.genx.oword.ld.unaligned.v8f32(i32 0, i32 3, i32 %arg)
  %a3 = tail call <8 x float> @llvm.genx.oword.ld.unaligned.v8f32(i32 0, i32 4, i32 %arg)
  %a4 = tail call <8 x float> @llvm.genx.oword.ld.unaligned.v8f32(i32 0, i32 5, i32 %arg)

; call is below: foo(mask, a0, a1, a2, a3, a4).
; check that the mask as well as predicate (since there is no differences between them for stackcalls) is passed correctly.
; CHECK-LABEL: test
; CHECK: %[[MASK_CAST:[^ ]+]] = bitcast <8 x i1> %mask to <1 x i8>
; CHECK: call <1024 x i8> @llvm.genx.wrregioni.v1024i8.v1i8.i16.i1(<1024 x i8> %{{[^ ]+}}, <1 x i8> %[[MASK_CAST]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call <256 x float> @llvm.genx.wrregionf.v256f32.v8f32.i16.i1(<256 x float> %{{[^ ]+}}, <8 x float> %a0
; CHECK: call <256 x float> @llvm.genx.wrregionf.v256f32.v8f32.i16.i1(<256 x float> %{{[^ ]+}}, <8 x float> %a1
; CHECK: call <256 x float> @llvm.genx.wrregionf.v256f32.v8f32.i16.i1(<256 x float> %{{[^ ]+}}, <8 x float> %a2
; CHECK: call <256 x float> @llvm.genx.wrregionf.v256f32.v8f32.i16.i1(<256 x float> %{{[^ ]+}}, <8 x float> %a3
; CHECK: call <256 x float> @llvm.genx.wrregionf.v256f32.v8f32.i16.i1(<256 x float> %{{[^ ]+}}, <8 x float> %a4

  %ret = tail call spir_func <8 x float> @foo(<8 x i1> %mask, <8 x float> %a0, <8 x float> %a1, <8 x float> %a2, <8 x float> %a3, <8 x float> %a4)
  tail call void @llvm.genx.oword.st.v8f32(i32 5, i32 %arg, <8 x float> %ret)
  ret void
}

define internal spir_func <8 x float> @foo(<8 x i1> %0, <8 x float> %1, <8 x float> %2, <8 x float> %3, <8 x float> %4, <8 x float> %5) #1 {
entry:
  %add.i = fadd <8 x float> %1, %2
  %add.i7 = fadd <8 x float> %add.i, %3
  %add.i13 = fadd <8 x float> %add.i7, %4
  %add.i19 = fadd <8 x float> %add.i13, %5
  ret <8 x float> %add.i19
}

attributes #0 = { nounwind "CMGenxMain" "oclrt"="1" }
attributes #1 = { noinline norecurse nounwind readnone "CMStackCall" }

;!spirv.Source = !{!0}
;!opencl.spir.version = !{!1}
;!opencl.ocl.version = !{!2}
;!opencl.used.extensions = !{!3}
;!opencl.used.optional.core.features = !{!3}
;!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!genx.kernel.internal = !{!15}

!0 = !{i32 0, i32 100000}
!1 = !{i32 1, i32 2}
!2 = !{i32 1, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (i32, <8 x i1>, i32, i32, i32, i32, i32, i32, i64, i64)* @test, !"test", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 112, i32 2, i32 2, i32 2, i32 2, i32 2, i32 24, i32 8, i32 96, i32 104}
!7 = !{i32 -1, i32 160, i32 168, i32 176, i32 184, i32 192, i32 64, i32 128, i32 144, i32 152}
!8 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!9 = !{!"svmptr_t", !"buffer_t", !"buffer_t", !"buffer_t", !"buffer_t", !"buffer_t"}
!15 = !{void (i32, <8 x i1>, i32, i32, i32, i32, i32, i32, i64, i64)* @test, !16, !17, !18, !22}
!16 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!17 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 0}
!18 = !{!19}
!19 = !{i32 0, !20}
!20 = !{!21}
!21 = !{i32 9, i32 0}
!22 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9}
