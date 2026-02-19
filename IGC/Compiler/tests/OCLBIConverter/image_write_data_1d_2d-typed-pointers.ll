;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-conv-ocl-to-common | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

%spirv.Image._void_0_0_0_0_0_0_1 = type opaque
%spirv.Image._void_0_0_1_0_0_0_1 = type opaque
%spirv.Image._void_1_0_0_0_0_0_1 = type opaque
%spirv.Image._void_1_0_1_0_0_0_1 = type opaque
%spirv.Image._void_2_0_0_0_0_0_1 = type opaque

define spir_kernel void @kernel_1d(%spirv.Image._void_0_0_0_0_0_0_1 addrspace(1)* %img_1d) {
  %img_as_int = ptrtoint %spirv.Image._void_0_0_0_0_0_0_1 addrspace(1)* %img_1d to i64
  %trunc = trunc i64 %img_as_int to i32

  ; CHECK-LABEL: @kernel_1d(

  ; CHECK-NOT: __builtin_IB_write_1d_u1i
  ; CHECK: [[IMG1D1:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG1D1]], i32 0, i32 0, i32 0, i32 0, float %floatColor, float undef, float undef, float undef)
  call spir_func void @__builtin_IB_write_1d_u1i(i32 %trunc, i32 0, i32 0, i32 0)

  ; CHECK-NOT: __builtin_IB_write_1d_u2i
  ; CHECK: [[IMG1D2:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG1D2]], i32 0, i32 0, i32 0, i32 0, float %ColorX, float %ColorY, float undef, float undef)
  call spir_func void @__builtin_IB_write_1d_u2i(i32 %trunc, i32 0, <2 x i32> zeroinitializer, i32 0)
  ret void
}

define spir_kernel void @kernel_1d_arr(%spirv.Image._void_0_0_1_0_0_0_1 addrspace(1)* %img_1d_arr) {
  %img_as_int = ptrtoint %spirv.Image._void_0_0_1_0_0_0_1 addrspace(1)* %img_1d_arr to i64
  %trunc = trunc i64 %img_as_int to i32

  ; CHECK-LABEL: @kernel_1d_arr(

  ; CHECK-NOT: __builtin_IB_write_1darr_u1i
  ; CHECK: [[IMG1DARR1:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG1DARR1]], i32 %{{.*}}, i32 %{{.*}}, i32 0, i32 0, float %floatColor, float undef, float undef, float undef)
  call spir_func void @__builtin_IB_write_1darr_u1i(i32 %trunc, <2 x i32> zeroinitializer, i32 0, i32 0)

  ; CHECK-NOT: __builtin_IB_write_1darr_u2i
  ; CHECK: [[IMG1DARR2:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG1DARR2]], i32 %{{.*}}, i32 %{{.*}}, i32 0, i32 0, float %ColorX, float %ColorY, float undef, float undef)
  call spir_func void @__builtin_IB_write_1darr_u2i(i32 %trunc, <2 x i32> zeroinitializer, <2 x i32> zeroinitializer, i32 0)

  ret void
}

define spir_kernel void @kernel_2d(%spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)* %img_2d) {
  %img_as_int = ptrtoint %spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)* %img_2d to i64
  %trunc = trunc i64 %img_as_int to i32

  ; CHECK-LABEL: @kernel_2d(

  ; CHECK-NOT: __builtin_IB_write_2d_u1i
  ; CHECK: [[IMG2D1:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG2D1]], i32 %{{.*}}, i32 %{{.*}}, i32 0, i32 0, float %floatColor, float undef, float undef, float undef)
  call spir_func void @__builtin_IB_write_2d_u1i(i32 %trunc, <2 x i32> zeroinitializer, i32 0, i32 0)

  ; CHECK-NOT: __builtin_IB_write_2d_u2i
  ; CHECK: [[IMG2D2:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG2D2]], i32 %{{.*}}, i32 %{{.*}}, i32 0, i32 0, float %ColorX, float %ColorY, float undef, float undef)
  call spir_func void @__builtin_IB_write_2d_u2i(i32 %trunc, <2 x i32> zeroinitializer, <2 x i32> zeroinitializer, i32 0)

  ret void
}

define spir_kernel void @kernel_2d_arr(%spirv.Image._void_1_0_1_0_0_0_1 addrspace(1)* %img_2d_arr) {
  %img_as_int = ptrtoint %spirv.Image._void_1_0_1_0_0_0_1 addrspace(1)* %img_2d_arr to i64
  %trunc = trunc i64 %img_as_int to i32

  ; CHECK-LABEL: @kernel_2d_arr(

  ; CHECK-NOT: __builtin_IB_write_2darr_u1i
  ; CHECK: [[IMG2DARR1:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG2DARR1]], i32 %{{.*}}, i32 %{{.*}}, i32 %{{.*}}, i32 0, float %floatColor, float undef, float undef, float undef)
  call spir_func void @__builtin_IB_write_2darr_u1i(i32 %trunc, <3 x i32> zeroinitializer, i32 0, i32 0)

  ; CHECK-NOT: __builtin_IB_write_2darr_u2i
  ; CHECK: [[IMG2DARR2:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG2DARR2]], i32 %{{.*}}, i32 %{{.*}}, i32 %{{.*}}, i32 0, float %ColorX, float %ColorY, float undef, float undef)
  call spir_func void @__builtin_IB_write_2darr_u2i(i32 %trunc, <3 x i32> zeroinitializer, <2 x i32> zeroinitializer, i32 0)

  ret void
}

define spir_kernel void @kernel_3d(%spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %img_3d) {
  %img_as_int = ptrtoint %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %img_3d to i64
  %trunc = trunc i64 %img_as_int to i32

  ; CHECK-LABEL: @kernel_3d(

  ; CHECK-NOT: __builtin_IB_write_3d_u1i
  ; CHECK: [[IMG3D1:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG3D1]], i32 %{{.*}}, i32 %{{.*}}, i32 %{{.*}}, i32 0, float %floatColor, float undef, float undef, float undef)
  call spir_func void @__builtin_IB_write_3d_u1i(i32 %trunc, <3 x i32> zeroinitializer, i32 0, i32 0)

  ; CHECK-NOT: __builtin_IB_write_3d_u2i
  ; CHECK: [[IMG3D2:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[IMG3D2]], i32 %{{CoordX.*}}, i32 %{{CoordY.*}}, i32 %{{CoordZ.*}}, i32 0, float %ColorX, float %ColorY, float undef, float undef)
  call spir_func void @__builtin_IB_write_3d_u2i(i32 %trunc, <3 x i32> zeroinitializer, <2 x i32> zeroinitializer, i32 0)

  ret void
}

declare spir_func void @__builtin_IB_write_1d_u1i(i32, i32, i32, i32)
declare spir_func void @__builtin_IB_write_1d_u2i(i32, i32, <2 x i32>, i32)

declare spir_func void @__builtin_IB_write_1darr_u1i(i32, <2 x i32>, i32, i32)
declare spir_func void @__builtin_IB_write_1darr_u2i(i32, <2 x i32>, <2 x i32>, i32)

declare spir_func void @__builtin_IB_write_2d_u1i(i32, <2 x i32>, i32, i32)
declare spir_func void @__builtin_IB_write_2d_u2i(i32, <2 x i32>, <2 x i32>, i32)

declare spir_func void @__builtin_IB_write_2darr_u1i(i32, <3 x i32>, i32, i32)
declare spir_func void @__builtin_IB_write_2darr_u2i(i32, <3 x i32>, <2 x i32>, i32)

declare spir_func void @__builtin_IB_write_3d_u1i(i32, <3 x i32>, i32, i32)
declare spir_func void @__builtin_IB_write_3d_u2i(i32, <3 x i32>, <2 x i32>, i32)

!igc.functions = !{!0, !2, !3, !4, !5}
!IGCMetadata = !{!6}

!0 = !{void (%spirv.Image._void_0_0_0_0_0_0_1 addrspace(1)*)* @kernel_1d, !1}
!1 = !{}
!2 = !{void (%spirv.Image._void_0_0_1_0_0_0_1 addrspace(1)*)* @kernel_1d_arr, !1}
!3 = !{void (%spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)*)* @kernel_2d, !1}
!4 = !{void (%spirv.Image._void_1_0_1_0_0_0_1 addrspace(1)*)* @kernel_2d_arr, !1}
!5 = !{void (%spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*)* @kernel_3d, !1}
!6 = !{!"ModuleMD", !7}
!7 = !{!"FuncMD", !8, !9, !16, !9, !17, !9, !18, !9, !19, !9}
!8 = distinct !{!"FuncMDMap[0]", void (%spirv.Image._void_0_0_0_0_0_0_1 addrspace(1)*)* @kernel_1d}
!9 = !{!"FuncMDValue[0]", !10}
!10 = !{!"resAllocMD", !11}
!11 = !{!"argAllocMDList", !12}
!12 = !{!"argAllocMDListVec[0]", !13, !14, !15}
!13 = !{!"type", i32 2}
!14 = !{!"extensionType", i32 0}
!15 = !{!"indexType", i32 0}
!16 = distinct !{!"FuncMDMap[0]", void (%spirv.Image._void_0_0_1_0_0_0_1 addrspace(1)*)* @kernel_1d_arr}
!17 = distinct !{!"FuncMDMap[0]", void (%spirv.Image._void_1_0_0_0_0_0_1 addrspace(1)*)* @kernel_2d}
!18 = distinct !{!"FuncMDMap[0]", void (%spirv.Image._void_1_0_1_0_0_0_1 addrspace(1)*)* @kernel_2d_arr}
!19 = distinct !{!"FuncMDMap[0]", void (%spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*)* @kernel_3d}
