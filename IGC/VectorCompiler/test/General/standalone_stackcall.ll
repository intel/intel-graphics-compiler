;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 -save-stack-call-linkage \
; RUN: -finalizer-opts='-dumpcommonisa -isaasmToConsole' \
; RUN: -vc-function-control=stackcall -o /dev/null | FileCheck %s

; CHECK: .global_function "_Z11SIMD_CALLEEPfDv16_fi"
; CHECK: fret

; ModuleID = 'Deserialized LLVM Module'
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readonly
declare <16 x float> @llvm.genx.svm.block.ld.unaligned.v16f32.i64(i64) #0

; Function Attrs: alwaysinline nounwind readonly
define spir_func <16 x float> @_Z11SIMD_CALLEEPfDv16_fi(float addrspace(4)* %A, <16 x float> %b, i32 %i) local_unnamed_addr #1 !FuncArgSize !5 !FuncRetSize !6 {
entry:
  %idx.ext.i = sext i32 %i to i64
  %add.ptr.i = getelementptr inbounds float, float addrspace(4)* %A, i64 %idx.ext.i
  %0 = ptrtoint float addrspace(4)* %add.ptr.i to i64
  %call.i.i.esimd = tail call <16 x float> @llvm.genx.svm.block.ld.unaligned.v16f32.i64(i64 %0)
  %add.i.i = fadd <16 x float> %call.i.i.esimd, %b
  ret <16 x float> %add.i.i
}

attributes #0 = { nounwind readonly }
attributes #1 = { alwaysinline nounwind readonly "CMStackCall" }

!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}

!0 = !{i32 0, i32 100000}
!1 = !{i32 1, i32 2}
!2 = !{i32 1, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{i32 4}
!6 = !{i32 1}