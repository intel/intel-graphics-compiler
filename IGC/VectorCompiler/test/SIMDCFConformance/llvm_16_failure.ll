;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_16_or_greater
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s

; CHECK-NOT: Error
%intel.buffer_rw_t.1 = type opaque

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(none)
declare <64 x i8> @llvm.genx.rdregioni.v64i8.v192i8.i16(<192 x i8>, i32, i32, i32, i16, i32) #0

; Function Attrs: mustprogress nounwind willreturn
declare void @llvm.genx.media.st.v64i8(i32, i32, i32, i32, i32, i32, <64 x i8>) #1

; Function Attrs: mustprogress noinline nounwind willreturn
define internal spir_func { <192 x i8>, <32 x i1> } @foo() unnamed_addr #2 {
entry:
  %goto_foo = tail call { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v16i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <16 x i1> zeroinitializer, <16 x i1> zeroinitializer)
  %em_foo = extractvalue { <32 x i1>, <16 x i1>, i1 } %goto_foo, 0
  %rm_foo = extractvalue { <32 x i1>, <16 x i1>, i1 } %goto_foo, 1
  %join_foo = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v16i1(<32 x i1> %em_foo, <16 x i1> %rm_foo)
  %join_em_foo = extractvalue { <32 x i1>, i1 } %join_foo, 0
  %foo_res = insertvalue { <192 x i8>, <32 x i1> } { <192 x i8> zeroinitializer, <32 x i1> undef }, <32 x i1> %join_em_foo, 1
  ret { <192 x i8>, <32 x i1> } %foo_res
}

; Function Attrs: mustprogress noinline nounwind willreturn
define internal spir_func void @bar(<192 x i8> %bar_in, <32 x i1> %EM.in) unnamed_addr #2 {
entry:
  %goto_bar = tail call { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v16i1(<32 x i1> %EM.in, <16 x i1> zeroinitializer, <16 x i1> undef)
  %em_bar = extractvalue { <32 x i1>, <16 x i1>, i1 } %goto_bar, 0
  %rm_bar = extractvalue { <32 x i1>, <16 x i1>, i1 } %goto_bar, 1
  %join_bar = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v16i1(<32 x i1> %em_bar, <16 x i1> %rm_bar)
  %bar_out = tail call <64 x i8> @llvm.genx.rdregioni.v64i8.v192i8.i16(<192 x i8> %bar_in, i32 8, i32 8, i32 1, i16 0, i32 8)
  tail call void @llvm.genx.media.st.v64i8(i32 0, i32 0, i32 0, i32 8, i32 1, i32 2, <64 x i8> %bar_out)
  ret void
}

; Function Attrs: mustprogress noinline nounwind willreturn
define dllexport spir_kernel void @Gexp(%intel.buffer_rw_t.1 addrspace(1)* nocapture readnone %A, %intel.buffer_rw_t.1 addrspace(1)* nocapture readnone %B, %intel.buffer_rw_t.1 addrspace(1)* nocapture readnone %C, %intel.buffer_rw_t.1 addrspace(1)* nocapture readnone %D, %intel.buffer_rw_t.1 addrspace(1)* nocapture readnone %E, %intel.buffer_rw_t.1 addrspace(1)* nocapture readnone %F, i64 %impl.arg.private.base) local_unnamed_addr #3 {
; CHECK-LABEL: Gexp
; CHECK: call {{.*}} foo
; CHECK: call {{.*}} bar
entry:
  %foo_res = tail call spir_func { <192 x i8>, <32 x i1> } @foo() #4
  %data_foo = extractvalue { <192 x i8>, <32 x i1> } %foo_res, 0
  %em_foo = extractvalue { <192 x i8>, <32 x i1> } %foo_res, 1
  tail call spir_func void @bar(<192 x i8> %data_foo, <32 x i1> %em_foo) #4
  ret void
}

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(none)
declare { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v16i1(<32 x i1>, <16 x i1>, <16 x i1>) #0

; Function Attrs: mustprogress nounwind willreturn
declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v16i1(<32 x i1>, <16 x i1>) #1

attributes #0 = { mustprogress nofree nosync nounwind willreturn memory(none) }
attributes #1 = { mustprogress nounwind willreturn }
attributes #2 = { mustprogress noinline nounwind willreturn "VC.SimdCFArg" }
attributes #3 = { mustprogress noinline nounwind willreturn "CMGenxMain" "oclrt"="1" }
attributes #4 = { noinline nounwind }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!genx.kernel.internal = !{!10}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (%intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, i64)* @Gexp, !"Gexp", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 96}
!7 = !{i32 72, i32 80, i32 88, i32 96, i32 104, i32 112, i32 64}
!8 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!9 = !{!"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write"}
!10 = !{void (%intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, %intel.buffer_rw_t.1 addrspace(1)*, i64)* @Gexp, !11, !12, !3, !13, i32 0}
!11 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!12 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6}
!13 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 255}
