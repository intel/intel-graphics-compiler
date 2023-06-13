;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXRegionCollapsing
; ------------------------------------------------
; This test checks that GenXRegionCollapsing pass doesn't collapse rdregion if this will
; result in clobbering of a global volatile value access.

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@global = internal global <4 x i32> undef, align 16, !spirv.Decorations !0 #0

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @kernel(i64 %idxSvmPtr, i64 %dataSvmPtr, i64 %outSvmPtr, i64 %impl.arg.private.base) local_unnamed_addr #4 {
entry:
  %.gload = load volatile <4 x i32>, <4 x i32>* @global, align 16
  %.gload1 = load volatile <4 x i32>, <4 x i32>* @global, align 16
  %call1.i.i = tail call <4 x i32> @llvm.genx.svm.block.ld.v4i32.i64(i64 %dataSvmPtr)
  %.wrr.gstore = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %.gload1, <4 x i32> %call1.i.i, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
  store volatile <4 x i32> %.wrr.gstore, <4 x i32>* @global, align 16
  %gload = load volatile <4 x i32>, <4 x i32>* @global, align 16
  %rdr = tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %gload, i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK:  %gload = load volatile <4 x i32>, <4 x i32>* @global, align 16
; CHECK-NEXT:  %rdr = tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %gload, i32 0, i32 1, i32 0, i16 0, i32 undef)
  %wrregion = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %gload, <1 x i32> <i32 100500>, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  store volatile <4 x i32> %wrregion, <4 x i32>* @global, align 16
; COM: %rdr must not get collapsed with %splat.splat since there is the potentially clobbering store to related global value preceding %splat.splat.
  %splat.splat = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> %rdr, i32 0, i32 4, i32 0, i16 0, i32 undef)
  tail call void @llvm.genx.svm.block.st.i64.v4i32(i64 %outSvmPtr, <4 x i32> %splat.splat)
  ret void
}

declare void @llvm.genx.svm.block.st.i64.v4i32(i64, <4 x i32>)
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.genx.svm.block.ld.v4i32.i64(i64)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { nofree nosync nounwind readnone }
attributes #2 = { nofree nounwind readonly }
attributes #3 = { nounwind }
attributes #4 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #5 = { convergent nounwind }
attributes #6 = { nounwind readnone }
attributes #7 = { nounwind writeonly }

!genx.kernels = !{!11}
!genx.kernel.internal = !{!18}

!0 = !{!1, !2, !3, !4}
!1 = !{i32 21}
!2 = !{i32 44, i32 16}
!3 = !{i32 5624}
!4 = !{i32 5628, i32 0}
!9 = !{}
!11 = !{void (i64, i64, i64, i64)* @kernel, !"kernel", !12, i32 0, !13, !14, !15, i32 0}
!12 = !{i32 0, i32 0, i32 0, i32 96}
!13 = !{i32 72, i32 80, i32 88, i32 64}
!14 = !{i32 0, i32 0, i32 0}
!15 = !{!"", !"", !""}
!18 = !{void (i64, i64, i64, i64)* @kernel, !19, !20, !9, !21}
!19 = !{i32 0, i32 0, i32 0, i32 0}
!20 = !{i32 0, i32 1, i32 2, i32 3}
!21 = !{i32 -1, i32 -1, i32 -1, i32 255}

