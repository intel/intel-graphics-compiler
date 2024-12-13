;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe2 -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe2 -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s


target triple = "genx64-unknown-unknown"


declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>) #0
declare void @llvm.genx.oword.st.v8i8(i32, i32, <8 x i8>) #0
declare { <8 x i1>, <8 x i32> } @llvm.genx.add3c.v8i1.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)

declare !internal_intrinsic_id !12 { <8 x double>, <8 x i1> } @llvm.vc.internal.invm.v8f64.v8i1(<8 x double>, <8 x double>) #3
declare !internal_intrinsic_id !13 { <8 x double>, <8 x i1> } @llvm.vc.internal.rsqrtm.v8f64.v8i1(<8 x double>) #3

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @the_test(i32 %0, i32 %1) local_unnamed_addr #1 {

; CHECK: .function "the_test_BB_0"
; CHECK: the_test_BB_0:
  %vec = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0) #2
  %vecdouble = bitcast <8 x i64>  %vec to <8 x double>
  %cmcl.builtin.InvM.i = tail call { <8 x double>, <8 x i1> } @llvm.vc.internal.invm.v8f64.v8i1(<8 x double> %vecdouble, <8 x double> %vecdouble) #2
; CHECK: invm (M1, 8) [[VR1:.*]](0,0)<1> [[P2:.*]] [[VR2:.*]](0,0)<1;1,0> [[VR2]](0,0)<1;1,0>
  %.InvM.res.i = extractvalue { <8 x double>, <8 x i1> } %cmcl.builtin.InvM.i, 0
  %.InvM.mask.i = extractvalue { <8 x double>, <8 x i1> } %cmcl.builtin.InvM.i, 1
  %cmcl.builtin.MRsqrt.i = tail call { <8 x double>, <8 x i1> } @llvm.vc.internal.rsqrtm.v8f64.v8i1(<8 x double> %.InvM.res.i) #2
; CHECK: rsqtm (M1, 8) [[VR4:.*]](0,0)<1> [[P1:.*]] [[VR3:.*]](0,0)<1;1,0>
  %.MRsqrt.res.i = extractvalue { <8 x double>, <8 x i1> } %cmcl.builtin.MRsqrt.i, 0
  %.MRsqrt.mask.i = extractvalue { <8 x double>, <8 x i1> } %cmcl.builtin.MRsqrt.i, 1
  %add.i = fadd <8 x double> %.InvM.res.i, %.MRsqrt.res.i
  %or.i30 = or <8 x i1> %.MRsqrt.mask.i, %.InvM.mask.i
; CHECK: or (M1, 8) [[P3:.*]] [[P1]] [[P2]]
  %or.i = zext <8 x i1> %or.i30 to <8 x i8>
  %veclong = bitcast <8 x double> %add.i to <8 x i64>
  tail call void @llvm.genx.oword.st.v8i64(i32 %1, i32 0, <8 x i64> %veclong) #2
  tail call void @llvm.genx.oword.st.v8i8(i32 %1, i32 0, <8 x i8> %or.i) #2
  ret void
}

attributes #0 = { nofree nosync nounwind }
attributes #1 = { mustprogress nofree noinline nosync nounwind willreturn "CMGenxMain" "oclrt"="1" }
attributes #2 = { nounwind }
attributes #3 = { nofree nosync nounwind readnone }

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
!12 = !{i32 11204}
!13 = !{i32 11271}