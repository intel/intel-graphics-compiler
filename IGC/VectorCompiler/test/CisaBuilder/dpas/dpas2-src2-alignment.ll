;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s

; Regression test for dpas Src2 alignment: the required base-decl alignment
; of Src2 depends on the actual Src1Precision/Src2Precision/SystolicDepth of
; each individual dpas call (see verifyInstructionDpas's src2Align formula in
; IsaVerification.cpp), and must never be a single blanket value (in
; particular it must not be forced to GRFALIGNED, which is both stricter
; than required in most cases and was found to sometimes insert spurious
; mov instructions -- Src2 alignment should be exactly one of 8, 16 or 32
; bytes depending on the precision pair):
;   - s8 x s8   (8-bit x 8-bit)  -> SystolicDepth(8) * OpsPerChan(4) *
;                                   Src2Bits(8) / 8   = 32 bytes
;   - u2 x u2   (2-bit x 2-bit)  -> SystolicDepth(8) * OpsPerChan(8) *
;                                   Src2Bits(2) / 8   = 16 bytes
;   - u8 x u2   (8-bit x 2-bit)  -> SystolicDepth(8) * OpsPerChan(4) *
;                                   Src2Bits(2) / 8   =  8 bytes
; Each variant uses its own, separately allocated Src2 storage so that
; register coalescing with an unrelated GRF-aligned value cannot mask an
; incorrect (too small) alignment computation.
;
; CHECK-DAG: .decl [[SRC2_S8:V[0-9]+]] v_type=G type=d num_elts=8 align=hword
; CHECK-DAG: .decl [[SRC2_U2:V[0-9]+]] v_type=G type=d num_elts=2 align=oword
; CHECK-DAG: .decl [[SRC2_U8U2:V[0-9]+]] v_type=G type=d num_elts=2 align=qword

; CHECK: dpas.s8.s8.8.1 (M1, 16) {{V[0-9]+}}.0 {{V[0-9]+}}.0 {{V[0-9]+}}.0 [[SRC2_S8]](0,0)
; CHECK: dpas.u2.u2.8.1 (M1, 16) {{V[0-9]+}}.0 {{V[0-9]+}}.0 {{V[0-9]+}}.0 [[SRC2_U2]](0,0)
; CHECK: dpas.u8.u2.8.1 (M1, 16) {{V[0-9]+}}.0 {{V[0-9]+}}.0 {{V[0-9]+}}.0 [[SRC2_U8U2]](0,0)

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@Acc1 = internal global <16 x i32> undef, align 512, !spirv.Decorations !0 #0
@Src1a = internal global <128 x i32> undef, align 512, !spirv.Decorations !0 #0
@Src2a = internal global <8 x i32> undef, align 256, !spirv.Decorations !5 #0

@Acc2 = internal global <16 x i32> undef, align 512, !spirv.Decorations !0 #0
@Src1b = internal global <128 x i32> undef, align 512, !spirv.Decorations !0 #0
@Src2b = internal global <2 x i32> undef, align 256, !spirv.Decorations !5 #0

@Acc3 = internal global <16 x i32> undef, align 512, !spirv.Decorations !0 #0
@Src1c = internal global <128 x i32> undef, align 512, !spirv.Decorations !0 #0
@Src2c = internal global <2 x i32> undef, align 256, !spirv.Decorations !5 #0

declare <16 x i32> @llvm.genx.dpas2.v16i32.v16i32.v128i32.v8i32(<16 x i32>, <128 x i32>, <8 x i32>, i32, i32, i32, i32, i32, i32) #1
declare <16 x i32> @llvm.genx.dpas2.v16i32.v16i32.v128i32.v2i32(<16 x i32>, <128 x i32>, <2 x i32>, i32, i32, i32, i32, i32, i32) #1

declare void @llvm.genx.vstore.v16i32.p0v16i32(<16 x i32>, <16 x i32>*) #2
declare <16 x i32> @llvm.genx.vload.v16i32.p0v16i32(<16 x i32>*) #2
declare <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>*) #2
declare <8 x i32> @llvm.genx.vload.v8i32.p0v8i32(<8 x i32>*) #2
declare <2 x i32> @llvm.genx.vload.v2i32.p0v2i32(<2 x i32>*) #2

define dllexport spir_kernel void @kernel(i64 %impl.arg.private.base) local_unnamed_addr #3 {
entry:
  ; Src1Precision=8 (S8), Src2Precision=8 (S8), SystolicDepth=8 -> 32-byte
  ; Src2 alignment.
  %acc1 = tail call <16 x i32> @llvm.genx.vload.v16i32.p0v16i32(<16 x i32>* nonnull @Acc1)
  %src1a = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1a)
  %src2a = tail call <8 x i32> @llvm.genx.vload.v8i32.p0v8i32(<8 x i32>* nonnull @Src2a)
  %res1 = tail call <16 x i32> @llvm.genx.dpas2.v16i32.v16i32.v128i32.v8i32(<16 x i32> %acc1, <128 x i32> %src1a, <8 x i32> %src2a, i32 8, i32 8, i32 8, i32 1, i32 1, i32 1)
  tail call void @llvm.genx.vstore.v16i32.p0v16i32(<16 x i32> %res1, <16 x i32>* nonnull @Acc1)

  ; Src1Precision=3 (U2), Src2Precision=3 (U2), SystolicDepth=8 -> 16-byte
  ; Src2 alignment.
  %acc2 = tail call <16 x i32> @llvm.genx.vload.v16i32.p0v16i32(<16 x i32>* nonnull @Acc2)
  %src1b = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1b)
  %src2b = tail call <2 x i32> @llvm.genx.vload.v2i32.p0v2i32(<2 x i32>* nonnull @Src2b)
  %res2 = tail call <16 x i32> @llvm.genx.dpas2.v16i32.v16i32.v128i32.v2i32(<16 x i32> %acc2, <128 x i32> %src1b, <2 x i32> %src2b, i32 3, i32 3, i32 8, i32 1, i32 1, i32 1)
  tail call void @llvm.genx.vstore.v16i32.p0v16i32(<16 x i32> %res2, <16 x i32>* nonnull @Acc2)

  ; Src1Precision=7 (U8), Src2Precision=3 (U2), SystolicDepth=8 -> 8-byte
  ; Src2 alignment.
  %acc3 = tail call <16 x i32> @llvm.genx.vload.v16i32.p0v16i32(<16 x i32>* nonnull @Acc3)
  %src1c = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1c)
  %src2c = tail call <2 x i32> @llvm.genx.vload.v2i32.p0v2i32(<2 x i32>* nonnull @Src2c)
  %res3 = tail call <16 x i32> @llvm.genx.dpas2.v16i32.v16i32.v128i32.v2i32(<16 x i32> %acc3, <128 x i32> %src1c, <2 x i32> %src2c, i32 7, i32 3, i32 8, i32 1, i32 1, i32 1)
  tail call void @llvm.genx.vstore.v16i32.p0v16i32(<16 x i32> %res3, <16 x i32>* nonnull @Acc3)

  ret void
}

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { nofree nosync nounwind readnone }
attributes #2 = { nounwind }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!spirv.MemoryModel = !{!9}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!10}
!opencl.spir.version = !{!11}
!opencl.ocl.version = !{!10}
!opencl.used.extensions = !{!12}
!opencl.used.optional.core.features = !{!12}
!spirv.Generator = !{!13}
!genx.kernels = !{!14}
!genx.kernel.internal = !{!17}

!0 = !{!1, !2, !3, !4}
!1 = !{i32 21}
!2 = !{i32 44, i32 512}
!3 = !{i32 5624}
!4 = !{i32 5628, i32 0}
!5 = !{!1, !6, !3, !4}
!6 = !{i32 44, i32 256}
!9 = !{i32 2, i32 2}
!10 = !{i32 0, i32 0}
!11 = !{i32 1, i32 2}
!12 = !{}
!13 = !{i16 6, i16 14}
!14 = !{void (i64)* @kernel, !"kernel", !15, i32 0, !16, !12, !12, i32 0}
!15 = !{i32 96}
!16 = !{i32 128}
!17 = !{void (i64)* @kernel, !18, !18, !12, !19}
!18 = !{i32 0}
!19 = !{i32 255}
