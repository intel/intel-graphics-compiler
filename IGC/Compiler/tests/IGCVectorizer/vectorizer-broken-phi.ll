; RUN: igc_opt -S  --igc-vectorizer -dce < %s 2>&1 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @triton_tem_fused_0() {
.lr.ph:
  br label %._crit_edge333

._crit_edge333:                                   ; preds = %._crit_edge333, %.lr.ph
  %0 = phi float [ 1.000000e+00, %.lr.ph ], [ 0.000000e+00, %._crit_edge333 ]
  %1 = phi float [ 0.000000e+00, %.lr.ph ], [ %8, %._crit_edge333 ]
  %2 = phi float [ 0.000000e+00, %.lr.ph ], [ %9, %._crit_edge333 ]
  %3 = phi float [ 0.000000e+00, %.lr.ph ], [ %10, %._crit_edge333 ]
  %4 = phi float [ 0.000000e+00, %.lr.ph ], [ 0x7FF8000000000000, %._crit_edge333 ]
  %5 = phi float [ 0.000000e+00, %.lr.ph ], [ 1.000000e+00, %._crit_edge333 ]
  %6 = phi float [ 0.000000e+00, %.lr.ph ], [ %11, %._crit_edge333 ]
  %7 = phi float [ 0.000000e+00, %.lr.ph ], [ %12, %._crit_edge333 ]
  ; CHECK-NOT: vectorized_phi
  %8 = call float @llvm.maxnum.f32(float %1, float 0.000000e+00)
  %9 = call float @llvm.maxnum.f32(float %2, float 0.000000e+00)
  %10 = call float @llvm.maxnum.f32(float %3, float 0.000000e+00)
  %11 = call float @llvm.maxnum.f32(float %6, float 0.000000e+00)
  %12 = call float @llvm.maxnum.f32(float %7, float 0.000000e+00)
  %13 = fptrunc float %0 to half
  %14 = fptrunc float %1 to half
  %15 = fptrunc float %2 to half
  %16 = fptrunc float %3 to half
  %17 = fptrunc float %4 to half
  %18 = fptrunc float %5 to half
  %19 = fptrunc float %6 to half
  %20 = fptrunc float %7 to half
  %21 = insertelement <8 x half> zeroinitializer, half %13, i64 0
  %22 = insertelement <8 x half> %21, half %14, i64 1
  %23 = insertelement <8 x half> %22, half %15, i64 2
  %24 = insertelement <8 x half> %23, half %16, i64 3
  %25 = insertelement <8 x half> %24, half %17, i64 4
  %26 = insertelement <8 x half> %25, half %18, i64 5
  %27 = insertelement <8 x half> %26, half %19, i64 6
  %28 = insertelement <8 x half> %27, half %20, i64 7
  %29 = bitcast <8 x half> %28 to <8 x i16>
  %30 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %29, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %._crit_edge333
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.maxnum.f32(float, float) #0

; uselistorder directives
uselistorder float (float, float)* @llvm.maxnum.f32, { 4, 3, 2, 1, 0 }

attributes #0 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}

!0 = distinct !{void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, half addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8 addrspace(2)*, i8*, i32, i32, i32, i32, i32, i32, i32, i32, i32)* bitcast (void ()* @triton_tem_fused_0 to void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, half addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8 addrspace(2)*, i8*, i32, i32, i32, i32, i32, i32, i32, i32, i32)*), !1}
!1 = distinct !{!2}
!2 = distinct !{!"sub_group_size", i32 16}
