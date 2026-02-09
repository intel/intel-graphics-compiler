;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe3P -S < %s | FileCheck %s --check-prefix=Xe3P
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe3P -S < %s | FileCheck %s --check-prefix=Xe3P

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe3PLPG -S < %s | FileCheck %s --check-prefix=Xe3PLPG
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe3PLPG -S < %s | FileCheck %s --check-prefix=Xe3PLPG



;; Test legalization of <32 x i64> constant that it is not split for SIMD32 programming model

declare <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64>, i32, i32, i32, i16, i32) #0
declare <64 x i64> @llvm.genx.wrregioni.v64i64.v32i64.i16.i1(<64 x i64>, <32 x i64>, i32, i32, i32, i16, i32, i1) #0
declare <64 x i64> @llvm.vc.internal.lsc.load.ugm.v64i64.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <64 x i64>) #2
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v64i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <64 x i64>) #3

define dllexport spir_kernel void @test(i8 addrspace(1)* nocapture readonly %arg, i8 addrspace(1)* nocapture writeonly %arg1, i64 %impl.arg.private.base) local_unnamed_addr #1 {
  ; Xe3P: [[CONST_SPLIT0:%[^ ]+]] = call <32 x i64> @llvm.genx.wrregioni.v32i64.v16i64.i16.i1(<32 x i64> undef, <16 x i64> <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>, i32 16, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ; Xe3P: [[CONST_SPLIT16:%[^ ]+]] = call <32 x i64> @llvm.genx.wrregioni.v32i64.v16i64.i16.i1(<32 x i64> [[CONST_SPLIT0]], <16 x i64> <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>, i32 16, i32 16, i32 1, i16 128, i32 undef, i1 true)
  ; Xe3P: add <32 x i64> %{{[^ ]+}}, [[CONST_SPLIT16]]
  ; Xe3PLPG: add <32 x i64> %{{[^ ]+}}, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  %ptrtoint = ptrtoint i8 addrspace(1)* %arg to i64
  %ugm = call <64 x i64> @llvm.vc.internal.lsc.load.ugm.v64i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 8, <2 x i8> zeroinitializer, i64 0, i64 %ptrtoint, i16 1, i32 0, <64 x i64> undef)
  %1 = tail call <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64> %ugm, i32 0, i32 32, i32 1, i16 0, i32 undef)
  %2 = add <32 x i64> %1, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  %wrregioni = tail call <64 x i64> @llvm.genx.wrregioni.v64i64.v32i64.i16.i1(<64 x i64> %ugm, <32 x i64> %2, i32 0, i32 32, i32 1, i16 256, i32 undef, i1 true)
  %ptrtoint2 = ptrtoint i8 addrspace(1)* %arg1 to i64
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v64i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 8, <2 x i8> zeroinitializer, i64 0, i64 %ptrtoint2, i16 1, i32 0, <64 x i64> %wrregioni)
  ret void
}

attributes #0 = { nofree nosync nounwind readnone }
attributes #1 = { mustprogress nofree noinline nosync nounwind willreturn "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #2 = { nounwind readonly }
attributes #3 = { nounwind writeonly }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.ocl.version = !{!1, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}
!genx.kernels = !{!6}
!genx.kernel.internal = !{!10}
!llvm.ident = !{!14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14, !14}
!llvm.module.flags = !{!15}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{i16 6, i16 14}
!6 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i64)* @test, !"test", !7, i32 0, !8, !1, !9, i32 0}
!7 = !{i32 0, i32 0, i32 96}
!8 = !{i32 136, i32 144, i32 128}
!9 = !{!"svmptr_t", !"svmptr_t"}
!10 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i64)* @test, !11, !12, !4, !13}
!11 = !{i32 0, i32 0, i32 0}
!12 = !{i32 0, i32 1, i32 2}
!13 = !{i32 255, i32 255, i32 255}
!14 = !{!"clang version 14.0.5"}
!15 = !{i32 1, !"wchar_size", i32 4}
