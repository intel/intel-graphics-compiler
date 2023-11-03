;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -finalizer-opts="-dumpcommonisa -isaasmToConsole" -mcpu=Gen9 < %s | FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; Gen9 VISA check
; CHECK-DAG: .decl [[V1:V[0-9]+]] v_type=G type=d num_elts=8 align=GRF
; CHECK-DAG: .decl [[V2:V[0-9]+]] v_type=G type=d num_elts=8 align=GRF
; CHECK-DAG: .decl [[V2ALIAS:V[0-9]+]] v_type=G type=d num_elts=8 alias=<[[V2]], 0>
; CHECK-DAG: .decl [[V1ALIAS:V[0-9]+]] v_type=G type=d num_elts=8 alias=<[[V1]], 0>

; CHECK: oword_ld (2) T{{[0-9]}} 0x0:ud [[V1]].0
; CHECK-NEXT: add3 (M1, 8) [[V2ALIAS]](0,0)<1> [[V1ALIAS]](0,0)<1;1,0> [[V1ALIAS]](0,0)<1;1,0> [[V1ALIAS]](0,0)<1;1,0>
; CHECK-NEXT: oword_st (2) T{{[0-9]}} 0x0:ud [[V2]].0

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

declare <8 x i32> @llvm.genx.oword.ld.v8i32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>) #0

declare <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @the_test(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec = tail call <8 x i32> @llvm.genx.oword.ld.v8i32(i32 0, i32 %0, i32 0) #2
  %res = call <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32> %vec, <8 x i32> %vec, <8 x i32> %vec) #2
  tail call void @llvm.genx.oword.st.v8i32(i32 %1, i32 0, <8 x i32> %res) #2
  ret void
}

attributes #0 = { "target-cpu"="Gen9" }
attributes #1 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="Gen9" }
attributes #2 = { nounwind }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i32, i32)* @the_test, !"the_test", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @the_test, null, null, null, null}
