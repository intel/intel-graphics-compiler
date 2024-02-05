;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=XeHPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s

target triple = "genx64-unknown-unknown"

; CHECK: .decl [[VR1:.*]] v_type=G type=ud num_elts=8
; CHECK: .decl [[VR2:.*]] v_type=G type=ud num_elts=8
; CHECK: .decl [[P1:.*]] v_type=P num_elts=8
; CHECK: .function "the_test_BB_0"
; CHECK: the_test_BB_0:
; CHECK: ([[P1]]) add3.o (M1, 8) [[VR1]](0,0)<1> [[VR2]](0,0)<1;1,0> [[VR2]](0,0)<1;1,0> [[VR2]](0,0)<1;1,0>

declare <8 x i32> @llvm.genx.oword.ld.v8i32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>) #0
declare void @llvm.genx.oword.st.v8i1(i32, i32, <8 x i1>) #0

declare { <8 x i1>, <8 x i32> } @llvm.genx.add3c.v8i1.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @the_test(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec = tail call <8 x i32> @llvm.genx.oword.ld.v8i32(i32 0, i32 %0, i32 0) #2
  %res = call { <8 x i1>, <8 x i32> } @llvm.genx.add3c.v8i1.v8i32(<8 x i32> %vec, <8 x i32> %vec, <8 x i32> %vec)
  %resx = extractvalue { <8 x i1>, <8 x i32> } %res, 1
  %resc = extractvalue { <8 x i1>, <8 x i32> } %res, 0
  tail call void @llvm.genx.oword.st.v8i32(i32 %1, i32 0, <8 x i32> %resx) #2
  tail call void @llvm.genx.oword.st.v8i1(i32 %1, i32 0, <8 x i1> %resc) #2
  ret void
}

attributes #0 = { nofree nosync nounwind readnone }
attributes #1 = { mustprogress nofree noinline nosync nounwind willreturn "CMGenxMain" "oclrt"="1" }
attributes #2 = { nounwind }

!spirv.MemoryModel = !{!9}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}
!llvm.ident = !{!10, !10, !10}
!llvm.module.flags = !{!11}


!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i32, i32)* @the_test, !"the_test", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @the_test, null, null, null, null}
!9 = !{i32 2, i32 2}
!10 = !{!"Ubuntu clang version 14.0.6"}
!11 = !{i32 1, !"wchar_size", i32 4}
