;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
; *** IR Dump After*** (GenXLoadStoreLegalization#1)
; ModuleID = 'Deserialized LLVM Module'
target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

%intel.buffer_rw_t = type opaque

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @"lsc_test<unsigned int, 8>"(i8 addrspace(1)* %0, %intel.buffer_rw_t addrspace(1)* %1, i8 addrspace(1)* %2, i8 addrspace(1)* nocapture readnone %3, i64 %impl.arg.private.base, i64 %impl.arg.indirect.data.buffer, i64 %impl.arg.scratch.buffer) local_unnamed_addr #0 {
  %5 = ptrtoint i8 addrspace(1)* %0 to i64
  %6 = ptrtoint %intel.buffer_rw_t addrspace(1)* %1 to i64
  %7 = trunc i64 %6 to i32
  %8 = ptrtoint i8 addrspace(1)* %2 to i64
; COM: Check that the following sequence doesn't make the pass crash
  %9 = icmp ugt i64 %impl.arg.private.base, 0
  %10 = select i1 %9, i64 %5, i64 %6
; CHECK-TYPED-PTRS: [[SURF64_PTR:%[^ ]+]] = ptrtoint %intel.buffer_rw_t addrspace(1)* %1 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR:%[^ ]+]] = ptrtoint ptr addrspace(1) %1 to i64
; CHECK: call void @_Z3barIjLi8EL15ChannelMaskType1ELi0EEvy15cm_surfaceindexyy(i64 %5, i64 [[SURF64_PTR]], i64 %8)
; CHECK: call void @_Z3barIjLi8EL15ChannelMaskType3ELi1EEvy15cm_surfaceindexyy(i64 %5, i64 [[SURF64_PTR]], i64 %8)
; CHECK: call void @_Z3barIjLi8EL15ChannelMaskType7ELi2EEvy15cm_surfaceindexyy(i64 %5, i64 [[SURF64_PTR]], i64 %8)
; CHECK: call void @_Z3barIjLi8EL15ChannelMaskType15ELi3EEvy15cm_surfaceindexyy(i64 %5, i64 [[SURF64_PTR]], i64 %8)
  tail call spir_func void @_Z3barIjLi8EL15ChannelMaskType1ELi0EEvy15cm_surfaceindexyy(i64 %5, i32 %7, i64 %8) #1
  tail call spir_func void @_Z3barIjLi8EL15ChannelMaskType3ELi1EEvy15cm_surfaceindexyy(i64 %5, i32 %7, i64 %8) #1
  tail call spir_func void @_Z3barIjLi8EL15ChannelMaskType7ELi2EEvy15cm_surfaceindexyy(i64 %5, i32 %7, i64 %8) #1
  tail call spir_func void @_Z3barIjLi8EL15ChannelMaskType15ELi3EEvy15cm_surfaceindexyy(i64 %5, i32 %7, i64 %8) #1
  ret void
}

; CHECK: void @_Z3barIjLi8EL15ChannelMaskType1ELi0EEvy15cm_surfaceindexyy(i64 %0, i64 %1, i64 %2)
; CHECK: [[SURF32_PTR:%[^ ]+]] = trunc i64 %1 to i32
; CHECK: tail call void @llvm.vc.internal.lsc.store.quad.bti.v8i1.v2i8.v8i32.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 [[SURF32_PTR]], <8 x i32> %10, i16 1, i32 0, <8 x i32> %8)
; Function Attrs: noinline nounwind
define internal spir_func void @_Z3barIjLi8EL15ChannelMaskType1ELi0EEvy15cm_surfaceindexyy(i64 %0, i32 %1, i64 %2) unnamed_addr #1 {
  %4 = call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <3 x i8> zeroinitializer, i64 0, i64 %2, i16 1, i32 0, <4 x i64> undef)
  %5 = bitcast <4 x i64> %4 to <8 x i32>
  %6 = call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <3 x i8> zeroinitializer, i64 0, i64 %0, i16 1, i32 0, <4 x i64> undef)
  %7 = bitcast <4 x i64> %6 to <8 x i32>
  %8 = shl <8 x i32> %5, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %9 = add <8 x i32> %8, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  tail call void @llvm.vc.internal.lsc.store.quad.bti.v8i1.v2i8.v8i32.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 %1, <8 x i32> %9, i16 1, i32 0, <8 x i32> %7)
  ret void
}

; CHECK: void @_Z3barIjLi8EL15ChannelMaskType3ELi1EEvy15cm_surfaceindexyy(i64 %0, i64 %1, i64 %2)
; CHECK: [[SURF32_PTR:%[^ ]+]] = trunc i64 %1 to i32
; CHECK: call void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v32i32(<16 x i1> %13, i8 2, i8 3, i8 3, <2 x i8> <i8 1, i8 1>, i32 [[SURF32_PTR]], <16 x i32> %14, i16 1, i32 0, <32 x i32> %15)
; Function Attrs: noinline nounwind
define internal spir_func void @_Z3barIjLi8EL15ChannelMaskType3ELi1EEvy15cm_surfaceindexyy(i64 %0, i32 %1, i64 %2) unnamed_addr #1 {
  %4 = add i64 %2, 32
  %5 = call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <3 x i8> zeroinitializer, i64 0, i64 %4, i16 1, i32 0, <4 x i64> undef)
  %6 = bitcast <4 x i64> %5 to <8 x i32>
  %7 = add i64 %0, 128
  %8 = call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <3 x i8> zeroinitializer, i64 0, i64 %7, i16 1, i32 0, <8 x i64> undef)
  %9 = bitcast <8 x i64> %8 to <16 x i32>
  %10 = shl <8 x i32> %6, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %11 = add <8 x i32> %10, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %12 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> zeroinitializer, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0)
  %13 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %11, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %14 = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %9, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
  call void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v32i32(<16 x i1> %12, i8 2, i8 3, i8 3, <2 x i8> <i8 1, i8 1>, i32 %1, <16 x i32> %13, i16 1, i32 0, <32 x i32> %14)
  ret void
}

; CHECK: void @_Z3barIjLi8EL15ChannelMaskType7ELi2EEvy15cm_surfaceindexyy(i64 %0, i64 %1, i64 %2)
; CHECK: [[SURF32_PTR:%[^ ]+]] = trunc i64 %1 to i32
; CHECK: call void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v48i32(<16 x i1> %18, i8 2, i8 3, i8 7, <2 x i8> <i8 1, i8 3>, i32 [[SURF32_PTR]], <16 x i32> %19, i16 1, i32 0, <48 x i32> %20)
; Function Attrs: noinline nounwind
define internal spir_func void @_Z3barIjLi8EL15ChannelMaskType7ELi2EEvy15cm_surfaceindexyy(i64 %0, i32 %1, i64 %2) unnamed_addr #1 {
  %4 = add i64 %2, 64
  %5 = call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <3 x i8> zeroinitializer, i64 0, i64 %4, i16 1, i32 0, <4 x i64> undef)
  %6 = bitcast <4 x i64> %5 to <8 x i32>
  %7 = add i64 %0, 256
  %8 = call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <3 x i8> zeroinitializer, i64 0, i64 %7, i16 1, i32 0, <8 x i64> undef)
  %9 = bitcast <8 x i64> %8 to <16 x i32>
  %10 = call <24 x i32> @llvm.genx.wrregioni.v24i32.v16i32.i16.i1(<24 x i32> undef, <16 x i32> %9, i32 1, i32 1, i32 0, i16 0, i32 0, i1 true)
  %11 = add i64 %0, 320
  %12 = call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <3 x i8> zeroinitializer, i64 0, i64 %11, i16 1, i32 0, <4 x i64> undef)
  %13 = bitcast <4 x i64> %12 to <8 x i32>
  %14 = call <24 x i32> @llvm.genx.wrregioni.v24i32.v8i32.i16.i1(<24 x i32> %10, <8 x i32> %13, i32 1, i32 1, i32 0, i16 64, i32 0, i1 true)
  %15 = shl <8 x i32> %6, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %16 = add <8 x i32> %15, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %17 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> zeroinitializer, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0)
  %18 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %16, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %19 = call <48 x i32> @llvm.genx.wrregioni.v48i32.v24i32.i16.i1(<48 x i32> undef, <24 x i32> %14, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
  call void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v48i32(<16 x i1> %17, i8 2, i8 3, i8 7, <2 x i8> <i8 1, i8 3>, i32 %1, <16 x i32> %18, i16 1, i32 0, <48 x i32> %19)
  ret void
}

; CHECK: void @_Z3barIjLi8EL15ChannelMaskType15ELi3EEvy15cm_surfaceindexyy(i64 %0, i64 %1, i64 %2)
; CHECK: [[SURF32_PTR:%[^ ]+]] = trunc i64 %1 to i32
; CHECK: call void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v64i32(<16 x i1> %13, i8 2, i8 3, i8 15, <2 x i8> <i8 4, i8 1>, i32 [[SURF32_PTR]], <16 x i32> %14, i16 1, i32 0, <64 x i32> %15)
; Function Attrs: noinline nounwind
define internal spir_func void @_Z3barIjLi8EL15ChannelMaskType15ELi3EEvy15cm_surfaceindexyy(i64 %0, i32 %1, i64 %2) unnamed_addr #1 {
  %4 = add i64 %2, 96
  %5 = call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <3 x i8> zeroinitializer, i64 0, i64 %4, i16 1, i32 0, <4 x i64> undef)
  %6 = bitcast <4 x i64> %5 to <8 x i32>
  %7 = add i64 %0, 384
  %8 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 6, <3 x i8> zeroinitializer, i64 0, i64 %7, i16 1, i32 0, <16 x i64> undef)
  %9 = bitcast <16 x i64> %8 to <32 x i32>
  %10 = shl <8 x i32> %6, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %11 = add <8 x i32> %10, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %12 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> zeroinitializer, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0)
  %13 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %11, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %14 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.i1(<64 x i32> undef, <32 x i32> %9, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
  call void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v64i32(<16 x i1> %12, i8 2, i8 3, i8 15, <2 x i8> <i8 4, i8 1>, i32 %1, <16 x i32> %13, i16 1, i32 0, <64 x i32> %14)
  ret void
}

; Function Attrs: nounwind writeonly
declare !internal_intrinsic_id !18 void @llvm.vc.internal.lsc.store.quad.bti.v8i1.v2i8.v8i32.v8i32(<8 x i1>, i8, i8, i8, <2 x i8>, i32, <8 x i32>, i16, i32, <8 x i32>) #2

; Function Attrs: nounwind writeonly
declare !internal_intrinsic_id !18 void @llvm.vc.internal.lsc.store.quad.bti.v8i1.v2i8.v8i32.v32i32(<8 x i1>, i8, i8, i8, <2 x i8>, i32, <8 x i32>, i16, i32, <32 x i32>) #2

; Function Attrs: nounwind writeonly
declare !internal_intrinsic_id !18 void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v32i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <32 x i32>) #2

; Function Attrs: nounwind writeonly
declare !internal_intrinsic_id !18 void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v48i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <48 x i32>) #2

; Function Attrs: nounwind writeonly
declare !internal_intrinsic_id !18 void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v64i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <64 x i32>) #2

; Function Attrs: nounwind readonly
declare !internal_intrinsic_id !19 <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v3i8.i64(<1 x i1>, i8, i8, i8, <3 x i8>, i64, i64, i16, i32, <4 x i64>) #3

; Function Attrs: nounwind readonly
declare !internal_intrinsic_id !19 <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v1i1.v3i8.i64(<1 x i1>, i8, i8, i8, <3 x i8>, i64, i64, i16, i32, <8 x i64>) #3

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !20 <24 x i32> @llvm.genx.wrregioni.v24i32.v16i32.i16.i1(<24 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !20 <24 x i32> @llvm.genx.wrregioni.v24i32.v8i32.i16.i1(<24 x i32>, <8 x i32>, i32, i32, i32, i16, i32, i1) #4

; Function Attrs: nounwind readonly
declare !internal_intrinsic_id !19 <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v1i1.v3i8.i64(<1 x i1>, i8, i8, i8, <3 x i8>, i64, i64, i16, i32, <16 x i64>) #3

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !21 <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1>, <8 x i1>, i32) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !20 <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32>, <8 x i32>, i32, i32, i32, i16, i32, i1) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !20 <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !20 <48 x i32> @llvm.genx.wrregioni.v48i32.v24i32.i16.i1(<48 x i32>, <24 x i32>, i32, i32, i32, i16, i32, i1) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !20 <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.i1(<64 x i32>, <32 x i32>, i32, i32, i32, i16, i32, i1) #4

attributes #0 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #1 = { noinline nounwind }
attributes #2 = { nounwind writeonly }
attributes #3 = { nounwind readonly }
attributes #4 = { nounwind readnone }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!genx.kernel.internal = !{!12}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (i8 addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64, i64, i64)* @"lsc_test<unsigned int, 8>", !"lsc_test<unsigned int, 8>", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 0, i32 2, i32 0, i32 0, i32 96, i32 144, i32 152}
!7 = !{i32 200, i32 208, i32 216, i32 224, i32 192, i32 128, i32 136}
!8 = !{i32 0, i32 0, i32 0, i32 0}
!9 = !{!"svmptr_t", !"buffer_t read_write", !"svmptr_t", !"svmptr_t"}
!12 = !{void (i8 addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64, i64, i64)* @"lsc_test<unsigned int, 8>", !13, !14, !3, !15}
!13 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!14 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6}
!15 = !{i32 255, i32 1, i32 255, i32 255, i32 255, i32 255, i32 255}
!18 = !{i32 11250}
!19 = !{i32 11230}
!20 = !{i32 11192}
!21 = !{i32 11190}
