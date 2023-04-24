;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dump-module-to-visa-transform-info-path=%basename_t.structure \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: FileCheck %s --input-file=%basename_t.structure
; CHECK:      [0] result (K)
; CHECK-NEXT: [1] foo___vyfvyf (I)
; CHECK-NEXT: [2] f_f (K)
; CHECK-NEXT: [3] bar___vyfvyf (I)

; ModuleID = 'basic_with_indirect_calls.ll'
source_filename = "basic_with_indirect_calls.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

; Function Attrs: nounwind readonly
declare <16 x float> @llvm.genx.svm.block.ld.v16f32.i64(i64) #1

; Function Attrs: nounwind
declare void @llvm.genx.svm.block.st.i64.v16f32(i64, <16 x float>) #2

; Function Attrs: noinline nounwind readnone
define internal spir_func <16 x float> @foo___vyfvyf(<16 x float> %a, <16 x float> %b, <16 x i1> %__mask) #3 !FuncArgSize !156 !FuncRetSize !157 {
allocas:
  %add_a_load_b_load = fadd <16 x float> %a, %b
  ret <16 x float> %add_a_load_b_load
}

; Function Attrs: noinline nounwind
define internal spir_func <16 x float> @bar___vyfvyf(<16 x float> %a, <16 x float> %b, <16 x i1> %__mask) #4 !FuncArgSize !156 !FuncRetSize !157 {
allocas:
  %add_a_load_b_load = fsub <16 x float> %a, %b
  ret <16 x float> %add_a_load_b_load
}

; Function Attrs: nounwind readnone
declare i1 @llvm.genx.any.v16i1(<16 x i1>) #5

; Function Attrs: nounwind
define dllexport void @f_f(float* noalias %RET, float* noalias %aFOO, i64 %privBase) #6 {
  %svm_ld_ptrtoint = ptrtoint float* %aFOO to i64
  %src1 = call <16 x float> @llvm.genx.svm.block.ld.v16f32.i64(i64 %svm_ld_ptrtoint)
  %foo_raw = ptrtoint <16 x float> (<16 x float>, <16 x float>, <16 x i1>)* @bar___vyfvyf to i64
  %bar_raw = ptrtoint <16 x float> (<16 x float>, <16 x float>, <16 x i1>)* @foo___vyfvyf to i64
  %foo_raw_vect = insertelement <1 x i64> undef, i64 %foo_raw, i32 0
  %bar_raw_vect = insertelement <1 x i64> undef, i64 %bar_raw, i32 0
  %cmp = icmp eq i64 %privBase, 0
  %rawaddr_v = select i1 %cmp, <1 x i64> %foo_raw_vect, <1 x i64> %bar_raw_vect
  %rawaddr = extractelement <1 x i64> %rawaddr_v, i32 0
  %fptr = inttoptr i64 %rawaddr to <16 x float> (<16 x float>, <16 x float>, <16 x i1>)*
  %calltmp = call spir_func <16 x float> %fptr(<16 x float> %src1, <16 x float> %src1, <16 x i1> zeroinitializer) #7, !FuncArgSize !156, !FuncRetSize !157
  %svm_st_ptrtoint = ptrtoint float* %RET to i64
  call void @llvm.genx.svm.block.st.i64.v16f32(i64 %svm_st_ptrtoint, <16 x float> %calltmp)
  ret void
}

; Function Attrs: nounwind
define dllexport void @result(float* noalias %RET, i64 %privBase) #6 {
allocas:
  ret void
}

attributes #0 = { nounwind readnone speculatable willreturn "target-cpu"="Gen9" }
attributes #1 = { nounwind readonly "target-cpu"="Gen9" }
attributes #2 = { nounwind "target-cpu"="Gen9" }
attributes #3 = { noinline nounwind readnone "CMStackCall" "target-cpu"="Gen9" }
attributes #4 = { noinline nounwind "CMStackCall" "target-cpu"="Gen9" }
attributes #5 = { nounwind readnone "target-cpu"="Gen9" }
attributes #6 = { nounwind "CMGenxMain" "oclrt"="1" "target-cpu"="Gen9" }
attributes #7 = { nounwind }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4, !8}
!VC.Debug.Enable = !{}
!genx.kernel.internal = !{!13, !16}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (float*, float*, i64)* @f_f, !"f_f", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 0, i32 0, i32 96}
!6 = !{i32 72, i32 80, i32 64}
!7 = !{!"", !""}
!8 = !{void (float*, i64)* @result, !"result", !9, i32 0, !10, !11, !12, i32 0}
!9 = !{i32 0, i32 96}
!10 = !{i32 72, i32 64}
!11 = !{i32 0}
!12 = !{!""}
!13 = !{void (float*, float*, i64)* @f_f, !14, !15, !2, null}
!14 = !{i32 0, i32 0, i32 0}
!15 = !{i32 0, i32 1, i32 2}
!16 = !{void (float*, i64)* @result, !0, !17, !2, null}
!17 = !{i32 0, i32 1}
!156 = !{i32 5}
!157 = !{i32 2}
