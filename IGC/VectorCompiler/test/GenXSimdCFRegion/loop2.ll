;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -simdcf-region -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------
; This test checks that GenXSimdCFRegion generate
; correct if-then simd llvm-ir (based on ispc test)

; Function Attrs: nounwind readnone
declare i1 @llvm.genx.any.v16i1(<16 x i1>) #0

; Function Attrs: nounwind
; CHECK: f_f
; CHECK: [[GOTO:%[A-z0-9.]*]] = tail call {{.*}} @llvm.genx.simdcf.goto
; CHECK: {{.*}} = extractvalue {{.*}} [[GOTO]]

; CHECK: [[JOIN_CALL:%[A-z0-9.]*]] = tail call {{.*}} @llvm.genx.simdcf.join
; CHECK: %{{.*}} = extractvalue {{.*}} [[JOIN_CALL]]


define dllexport spir_kernel void @f_f(i8 addrspace(1)* %0, i8 addrspace(1)* %1, i64 %impl.arg.private.base) #1 {
  %3 = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %impl.arg.private.base, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  %4 = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
  %5 = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %4, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %6 = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %5, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  %7 = ptrtoint i8 addrspace(1)* %0 to i64
  %8 = ptrtoint i8 addrspace(1)* %1 to i64
  %9 = call <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v1i1.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, i8 0, i8 0, i64 0, i64 %8, i16 1, i32 0, <16 x float> undef)
  %10 = call <1 x float> @llvm.vc.internal.lsc.load.ugm.v1f32.v1i1.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, i64 %8, i16 1, i32 0, <1 x float> undef)
  %11 = extractelement <1 x float> %10, i64 0
  %12 = fadd float %11, -1.000000e+00
  %13 = insertelement <16 x float> undef, float %12, i64 0
  %14 = shufflevector <16 x float> %13, <16 x float> undef, <16 x i32> zeroinitializer
  br label %15

15:                                               ; preds = %19, %2
  %16 = phi <16 x float> [ undef, %2 ], [ %30, %19 ]
  %17 = phi <16 x i1> [ <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, %2 ], [ zeroinitializer, %19 ]
  %18 = call i1 @llvm.genx.any.v16i1(<16 x i1> %17)
  br i1 %18, label %19, label %31

19:                                               ; preds = %15
  %20 = call <96 x float> @llvm.genx.read.predef.reg.v96f32.v96f32(i32 8, <96 x float> undef)
  %21 = call <96 x float> @llvm.genx.wrregionf.v96f32.v16f32.i16.i1(<96 x float> %20, <16 x float> %9, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %22 = call <96 x float> @llvm.genx.read.predef.reg.v96f32.v96f32(i32 8, <96 x float> undef)
  %23 = call <96 x float> @llvm.genx.wrregionf.v96f32.v16f32.i16.i1(<96 x float> %22, <16 x float> %14, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %24 = bitcast <16 x i1> %17 to <2 x i8>
  %25 = call <384 x i8> @llvm.genx.read.predef.reg.v384i8.v384i8(i32 8, <384 x i8> undef)
  %26 = call <384 x i8> @llvm.genx.wrregioni.v384i8.v2i8.i16.i1(<384 x i8> %25, <2 x i8> %24, i32 0, i32 2, i32 1, i16 128, i32 undef, i1 true)
  %27 = call <64 x float> @llvm.genx.read.predef.reg.v64f32.v64f32(i32 9, <64 x float> undef)
  %28 = call <16 x float> @llvm.genx.rdregionf.v16f32.v64f32.i16(<64 x float> %27, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %29 = call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float> undef, <16 x float> %28, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %30 = select <16 x i1> %17, <16 x float> %29, <16 x float> %16
  br label %15

31:                                               ; preds = %15
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.i64.v16f32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, i8 0, i8 0, i64 0, i64 %7, i16 1, i32 0, <16 x float> %16)
  ret void
}

; Function Attrs: nounwind readonly
declare !genx_intrinsic_id !26 <96 x float> @llvm.genx.read.predef.reg.v96f32.v96f32(i32, <96 x float>) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !27 <96 x float> @llvm.genx.wrregionf.v96f32.v16f32.i16.i1(<96 x float>, <16 x float>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind writeonly
declare !genx_intrinsic_id !28 <16 x float> @llvm.genx.write.predef.reg.v16f32.v96f32(i32, <96 x float>) #5

; Function Attrs: nounwind readonly
declare !genx_intrinsic_id !26 <384 x i8> @llvm.genx.read.predef.reg.v384i8.v384i8(i32, <384 x i8>) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !29 <384 x i8> @llvm.genx.wrregioni.v384i8.v2i8.i16.i1(<384 x i8>, <2 x i8>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind writeonly
declare !genx_intrinsic_id !28 <2 x i8> @llvm.genx.write.predef.reg.v2i8.v384i8(i32, <384 x i8>) #5

; Function Attrs: nounwind readonly
declare !genx_intrinsic_id !26 <64 x float> @llvm.genx.read.predef.reg.v64f32.v64f32(i32, <64 x float>) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !30 <16 x float> @llvm.genx.rdregionf.v16f32.v64f32.i16(<64 x float>, i32, i32, i32, i16, i32) #0

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !27 <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float>, <16 x float>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !29 i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64, i64, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind writeonly
declare !genx_intrinsic_id !28 i64 @llvm.genx.write.predef.reg.i64.i64(i32, i64) #5

; Function Attrs: nounwind readonly
declare !genx_intrinsic_id !26 <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32, i64) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !32 i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32) #0

; Function Attrs: nounwind readonly
declare !internal_intrinsic_id !33 <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v1i1.i64(<1 x i1>, i8, i8, i8, i8, i8, i64, i64, i16, i32, <16 x float>) #4

; Function Attrs: nounwind readonly
declare !internal_intrinsic_id !33 <1 x float> @llvm.vc.internal.lsc.load.ugm.v1f32.v1i1.i64(<1 x i1>, i8, i8, i8, i8, i8, i64, i64, i16, i32, <1 x float>) #4

; Function Attrs: nounwind writeonly
declare !internal_intrinsic_id !34 void @llvm.vc.internal.lsc.store.ugm.v1i1.i64.v16f32(<1 x i1>, i8, i8, i8, i8, i8, i64, i64, i16, i32, <16 x float>) #5

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind "CMGenxMain" "oclrt"="1" }
attributes #2 = { nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #3 = { alwaysinline nounwind "CMStackCall" }
attributes #4 = { nounwind readonly }
attributes #5 = { nounwind writeonly }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2, !3, !3, !3}
!opencl.ocl.version = !{!1, !3, !3, !3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}
!genx.kernels = !{!6}
!llvm.ident = !{!15, !15, !15}
!llvm.module.flags = !{!16}
!genx.kernel.internal = !{!17}


!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{i16 6, i16 14}
!6 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i64)* @f_f, !"f_f", !7, i32 0, !8, !1, !9, i32 0}
!7 = !{i32 0, i32 0, i32 96}
!8 = !{i32 72, i32 80, i32 64}
!9 = !{!"svmptr_t", !"svmptr_t"}
!15 = !{!"clang version 14.0.5"}
!16 = !{i32 1, !"wchar_size", i32 4}
!17 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i64)* @f_f, !18, !19, !4, !20}
!18 = !{i32 0, i32 0, i32 0}
!19 = !{i32 0, i32 1, i32 2}
!20 = !{i32 255, i32 255, i32 255}
!26 = !{i32 10965}
!27 = !{i32 11168}
!28 = !{i32 11164}
!29 = !{i32 11169}
!30 = !{i32 10963}
!32 = !{i32 10964}
!33 = !{i32 11189}
!34 = !{i32 11199}
