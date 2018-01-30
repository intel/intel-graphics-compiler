;===================== begin_copyright_notice ==================================

;Copyright (c) 2017 Intel Corporation

;Permission is hereby granted, free of charge, to any person obtaining a
;copy of this software and associated documentation files (the
;"Software"), to deal in the Software without restriction, including
;without limitation the rights to use, copy, modify, merge, publish,
;distribute, sublicense, and/or sell copies of the Software, and to
;permit persons to whom the Software is furnished to do so, subject to
;the following conditions:

;The above copyright notice and this permission notice shall be included
;in all copies or substantial portions of the Software.

;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
;OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
;CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
;TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
;SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


;======================= end_copyright_notice ==================================
; RUN: opt -igc-conv-ocl-to-common -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that BuiltinsConverter pass handles images/samplers rights.
;;
;; Input
;;  - Test contains different types of image/sampler access functions
;;  - image and sampler are stored in alloca (mem2reg pass is disabled)
;;
;; Expected output
;;  - Pass does not crash
;;  - image/sampler built-ins are converted (resolved) as expected.
;;
;; Note: this test was semi-auto generated from cl file below.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN8"

%opencl.image2d_t = type opaque
%opencl.image3d_t = type opaque
%opencl.image2d_array_t = type opaque

; Function Attrs: alwaysinline nounwind
define void @test_image_width(i32 addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageWidth, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %i = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load i32* %i, align 4
  %2 = load i32 addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds i32 addrspace(1)* %2, i32 %1
  store i32 %imageWidth, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_height(i32 addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageHeigt, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %i = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load i32* %i, align 4
  %2 = load i32 addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds i32 addrspace(1)* %2, i32 %1
  store i32 %imageHeigt, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_depth(i32 addrspace(1)* %dst, %opencl.image3d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageDepth, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 4
  %img.addr = alloca %opencl.image3d_t addrspace(1)*, align 4
  %i = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 4
  store %opencl.image3d_t addrspace(1)* %img, %opencl.image3d_t addrspace(1)** %img.addr, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load i32* %i, align 4
  %2 = load i32 addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds i32 addrspace(1)* %2, i32 %1
  store i32 %imageDepth, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_channel_data_type(i32 addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageDataType, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %i = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load i32* %i, align 4
  %2 = load i32 addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds i32 addrspace(1)* %2, i32 %1
  store i32 %imageDataType, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_channel_order(i32 addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageOrder, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %i = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load i32* %i, align 4
  %2 = load i32 addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds i32 addrspace(1)* %2, i32 %1
  store i32 %imageOrder, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_dim(<2 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageHeigt, i32 %imageWidth, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <2 x i32> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %i = alloca i32, align 4
  store <2 x i32> addrspace(1)* %dst, <2 x i32> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %2 = insertelement <2 x i32> %1, i32 %imageHeigt, i32 1
  %3 = load i32* %i, align 4
  %4 = load <2 x i32> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <2 x i32> addrspace(1)* %4, i32 %3
  store <2 x i32> %2, <2 x i32> addrspace(1)* %arrayidx, align 8
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_array_size(i32 addrspace(1)* %dst, %opencl.image2d_array_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageArrSize, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_array_t addrspace(1)*, align 4
  %i = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_array_t addrspace(1)* %img, %opencl.image2d_array_t addrspace(1)** %img.addr, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load i32* %i, align 4
  %2 = load i32 addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds i32 addrspace(1)* %2, i32 %1
  store i32 %imageArrSize, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagef_1(<4 x float> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <4 x float> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %sampler.addr = alloca i32, align 4
  %coord.addr = alloca <2 x i32>, align 8
  %samplerNearest = alloca i32, align 4
  %i = alloca i32, align 4
  store <4 x float> addrspace(1)* %dst, <4 x float> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store i32 %sampler, i32* %sampler.addr, align 4
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  store i32 0, i32* %samplerNearest, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load i32* %sampler.addr, align 4
  %3 = load <2 x i32>* %coord.addr, align 8
  %4 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  %5 = extractelement <2 x i32> %3, i32 0
  %6 = sitofp i32 %5 to float
  %7 = insertelement <2 x float> undef, float %6, i32 0
  %8 = extractelement <2 x i32> %3, i32 1
  %9 = sitofp i32 %8 to float
  %10 = insertelement <2 x float> %7, float %9, i32 1
  %11 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %4, i32 %2, <2 x float> %10, float 0.000000e+00) #1
  %12 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %13 = load i32* %samplerNearest, align 4
  %14 = load <2 x i32>* %coord.addr, align 8
  %15 = ptrtoint %opencl.image2d_t addrspace(1)* %12 to i32
  %16 = extractelement <2 x i32> %14, i32 0
  %17 = sitofp i32 %16 to float
  %18 = insertelement <2 x float> undef, float %17, i32 0
  %19 = extractelement <2 x i32> %14, i32 1
  %20 = sitofp i32 %19 to float
  %21 = insertelement <2 x float> %18, float %20, i32 1
  %22 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %15, i32 %13, <2 x float> %21, float 0.000000e+00) #1
  %add = fadd <4 x float> %11, %22
  %23 = load i32* %i, align 4
  %24 = load <4 x float> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <4 x float> addrspace(1)* %24, i32 %23
  store <4 x float> %add, <4 x float> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIF1_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF1_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIF1_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF1_Coord]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIF1_CoordX]], float [[RIF1_CoordY]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIF1_CoordX1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF1_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIF1_CoordY1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF1_Coord1]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIF1_CoordX1]], float [[RIF1_CoordY1]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagef_2(<4 x float> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x float> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %smpSnapWA, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <4 x float> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %sampler.addr = alloca i32, align 4
  %coord.addr = alloca <2 x float>, align 8
  %samplerNearest = alloca i32, align 4
  %i = alloca i32, align 4
  store <4 x float> addrspace(1)* %dst, <4 x float> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store i32 %sampler, i32* %sampler.addr, align 4
  store <2 x float> %coord, <2 x float>* %coord.addr, align 8
  store i32 0, i32* %samplerNearest, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load i32* %sampler.addr, align 4
  %3 = load <2 x float>* %coord.addr, align 8
  %4 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  %5 = icmp eq i32 %smpSnapWA, 0
  br i1 %5, label %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit, label %6

; <label>:6                                       ; preds = %entry
  %7 = extractelement <2 x float> %3, i32 0
  %8 = fcmp olt float %7, 0.000000e+00
  %9 = select i1 %8, float -1.000000e+00, float %7
  %10 = insertelement <2 x float> undef, float %9, i32 0
  %11 = extractelement <2 x float> %3, i32 1
  %12 = fcmp olt float %11, 0.000000e+00
  %13 = select i1 %12, float -1.000000e+00, float %11
  %14 = insertelement <2 x float> %10, float %13, i32 1
  br label %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit

_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit: ; preds = %entry, %6
  %.0.i = phi <2 x float> [ %14, %6 ], [ %3, %entry ]
  %15 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %4, i32 %2, <2 x float> %.0.i, float 0.000000e+00) #1
  %16 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %17 = load i32* %samplerNearest, align 4
  %18 = load <2 x float>* %coord.addr, align 8
  %19 = ptrtoint %opencl.image2d_t addrspace(1)* %16 to i32
  %20 = icmp eq i32 0, 0
  br i1 %20, label %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit2, label %21

; <label>:21                                      ; preds = %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit
  %22 = extractelement <2 x float> %18, i32 0
  %23 = fcmp olt float %22, 0.000000e+00
  %24 = select i1 %23, float -1.000000e+00, float %22
  %25 = insertelement <2 x float> undef, float %24, i32 0
  %26 = extractelement <2 x float> %18, i32 1
  %27 = fcmp olt float %26, 0.000000e+00
  %28 = select i1 %27, float -1.000000e+00, float %26
  %29 = insertelement <2 x float> %25, float %28, i32 1
  br label %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit2

_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit2: ; preds = %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit, %21
  %.0.i1 = phi <2 x float> [ %29, %21 ], [ %18, %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit ]
  %30 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %19, i32 %17, <2 x float> %.0.i1, float 0.000000e+00) #1
  %add = fadd <4 x float> %15, %30
  %31 = load i32* %i, align 4
  %32 = load <4 x float> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <4 x float> addrspace(1)* %32, i32 %31
  store <4 x float> %add, <4 x float> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK: _Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit: ; preds
  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIF2_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF2_Coord:%[a-zA-Z0-9.]+]], i32 0
  ; CHECK: [[RIF2_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF2_Coord]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIF2_CoordX]], float [[RIF2_CoordY]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK: _Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit2: ; preds
  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIF2_CoordX1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF2_Coord:%[a-zA-Z0-9.]+]], i32 0
  ; CHECK: [[RIF2_CoordY1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF2_Coord]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIF2_CoordX1]], float [[RIF2_CoordY1]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagef_3(<4 x float> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <4 x float> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %coord.addr = alloca <2 x i32>, align 8
  %i = alloca i32, align 4
  store <4 x float> addrspace(1)* %dst, <4 x float> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load <2 x i32>* %coord.addr, align 8
  %3 = bitcast %opencl.image2d_t addrspace(1)* %1 to i8*
  %4 = ptrtoint i8* %3 to i32
  %5 = call <4 x float> @__builtin_IB_OCL_2d_ld(i32 %4, <2 x i32> %2, i32 0) #1
  %6 = bitcast <4 x float> %5 to <4 x i32>
  %7 = and <4 x i32> %6, <i32 -2139095041, i32 -2139095041, i32 -2139095041, i32 -2139095041>
  %8 = icmp eq <4 x i32> %7, %6
  %9 = sext <4 x i1> %8 to <4 x i32>
  %10 = and <4 x i32> %6, <i32 -2147483648, i32 -2147483648, i32 -2147483648, i32 -2147483648>
  %11 = bitcast <4 x i32> %10 to <4 x float>
  %12 = icmp slt <4 x i32> %9, zeroinitializer
  %13 = select <4 x i1> %12, <4 x float> %11, <4 x float> %5
  %14 = load i32* %i, align 4
  %15 = load <4 x float> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <4 x float> addrspace(1)* %15, i32 %14
  store <4 x float> %13, <4 x float> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_ld
  ; CHECK: [[RIF3_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIF3_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIF3_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIF3_Coord]], i32 1
  ; CHECK: [[RIF3_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.ldptr.v4f32.p196608f32(i32 [[RIF3_CoordX]], i32 [[RIF3_CoordY]], i32 0, i32 0, float addrspace(196608)* [[RIF3_GetBufferPtr]], i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagei_1(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %smpAddress, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %sampler.addr = alloca i32, align 4
  %coord.addr = alloca <2 x i32>, align 8
  %samplerNearest = alloca i32, align 4
  %i = alloca i32, align 4
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store i32 %sampler, i32* %sampler.addr, align 4
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  store i32 0, i32* %samplerNearest, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load i32* %sampler.addr, align 4
  %3 = load <2 x i32>* %coord.addr, align 8
  %4 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  %5 = and i32 %smpAddress, 7
  %6 = icmp eq i32 %5, 2
  br i1 %6, label %7, label %16

; <label>:7                                       ; preds = %entry
  %8 = extractelement <2 x i32> %3, i32 0
  %9 = sitofp i32 %8 to float
  %10 = insertelement <2 x float> undef, float %9, i32 0
  %11 = extractelement <2 x i32> %3, i32 1
  %12 = sitofp i32 %11 to float
  %13 = insertelement <2 x float> %10, float %12, i32 1
  %14 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %4, i32 %2, <2 x float> %13, float 0.000000e+00) #1
  %15 = bitcast <4 x float> %14 to <4 x i32>
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit

; <label>:16                                      ; preds = %entry
  %17 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %4, <2 x i32> %3, i32 0) #1
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit

_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit: ; preds = %7, %16
  %.0.i.i = phi <4 x i32> [ %15, %7 ], [ %17, %16 ]
  %18 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %19 = load i32* %samplerNearest, align 4
  %20 = load <2 x i32>* %coord.addr, align 8
  %21 = ptrtoint %opencl.image2d_t addrspace(1)* %18 to i32
  %22 = and i32 0, 7
  %23 = icmp eq i32 %22, 2
  br i1 %23, label %24, label %33

; <label>:24                                      ; preds = %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit
  %25 = extractelement <2 x i32> %20, i32 0
  %26 = sitofp i32 %25 to float
  %27 = insertelement <2 x float> undef, float %26, i32 0
  %28 = extractelement <2 x i32> %20, i32 1
  %29 = sitofp i32 %28 to float
  %30 = insertelement <2 x float> %27, float %29, i32 1
  %31 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %21, i32 %19, <2 x float> %30, float 0.000000e+00) #1
  %32 = bitcast <4 x float> %31 to <4 x i32>
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit2

; <label>:33                                      ; preds = %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit
  %34 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %21, <2 x i32> %20, i32 0) #1
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit2

_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit2: ; preds = %24, %33
  %.0.i.i1 = phi <4 x i32> [ %32, %24 ], [ %34, %33 ]
  %add = add <4 x i32> %.0.i.i, %.0.i.i1
  %35 = load i32* %i, align 4
  %36 = load <4 x i32> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <4 x i32> addrspace(1)* %36, i32 %35
  store <4 x i32> %add, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RII1_CoordX1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII1_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII1_CoordY1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII1_Coord1]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RII1_CoordX1]], float [[RII1_CoordY1]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII1_CoordX2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII1_Coord2:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII1_CoordY2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII1_Coord2]], i32 1
  ; CHECK: [[RII1_GetBufferPtr2:%[a-zA-Z0-9]+]]  = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RII1_CoordX2]], i32 [[RII1_CoordY2]], i32 0, i32 0, float addrspace(196608)* [[RII1_GetBufferPtr2]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RII1_CoordX3:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII1_Coord3:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII1_CoordY3:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII1_Coord3]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RII1_CoordX3]], float [[RII1_CoordY3]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII1_CoordX4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII1_Coord4:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII1_CoordY4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII1_Coord4]], i32 1
  ; CHECK: [[RII1_GetBufferPtr4:%[a-zA-Z0-9]+]]  = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RII1_CoordX4]], i32 [[RII1_CoordY4]], i32 0, i32 0, float addrspace(196608)* [[RII1_GetBufferPtr4]], i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagei_2(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x float> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageHeigt, i32 %imageWidth, i32 %smpAddress, i32 %smpNormalized, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %sampler.addr = alloca i32, align 4
  %coord.addr = alloca <2 x float>, align 8
  %samplerNearest = alloca i32, align 4
  %i = alloca i32, align 4
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store i32 %sampler, i32* %sampler.addr, align 4
  store <2 x float> %coord, <2 x float>* %coord.addr, align 8
  store i32 0, i32* %samplerNearest, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load i32* %sampler.addr, align 4
  %3 = load <2 x float>* %coord.addr, align 8
  %4 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  %5 = and i32 %smpAddress, 7
  %6 = icmp eq i32 %5, 1
  br i1 %6, label %7, label %33

; <label>:7                                       ; preds = %entry
  %8 = icmp eq i32 %smpNormalized, 8
  br i1 %8, label %9, label %19

; <label>:9                                       ; preds = %7
  %10 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %11 = insertelement <2 x i32> %10, i32 %imageHeigt, i32 1
  %12 = extractelement <2 x i32> %11, i32 0
  %13 = sitofp i32 %12 to float
  %14 = insertelement <2 x float> undef, float %13, i32 0
  %15 = extractelement <2 x i32> %11, i32 1
  %16 = sitofp i32 %15 to float
  %17 = insertelement <2 x float> %14, float %16, i32 1
  %18 = fmul <2 x float> %17, %3
  br label %19

; <label>:19                                      ; preds = %9, %7
  %.01.i.i = phi <2 x float> [ %18, %9 ], [ %3, %7 ]
  %20 = extractelement <2 x float> %.01.i.i, i32 0
  %21 = call float @__builtin_IB_frnd_ni(float %20) #1
  %22 = insertelement <2 x float> undef, float %21, i32 0
  %23 = extractelement <2 x float> %.01.i.i, i32 1
  %24 = call float @__builtin_IB_frnd_ni(float %23) #1
  %25 = insertelement <2 x float> %22, float %24, i32 1
  %26 = extractelement <2 x float> %25, i32 0
  %27 = fptosi float %26 to i32
  %28 = insertelement <2 x i32> undef, i32 %27, i32 0
  %29 = extractelement <2 x float> %25, i32 1
  %30 = fptosi float %29 to i32
  %31 = insertelement <2 x i32> %28, i32 %30, i32 1
  %32 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %4, <2 x i32> %31, i32 0) #1
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit

; <label>:33                                      ; preds = %entry
  %34 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %4, i32 %2, <2 x float> %3, float 0.000000e+00) #1
  %35 = bitcast <4 x float> %34 to <4 x i32>
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit

_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit: ; preds = %19, %33
  %.0.i.i = phi <4 x i32> [ %32, %19 ], [ %35, %33 ]
  %36 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %37 = load i32* %samplerNearest, align 4
  %38 = load <2 x float>* %coord.addr, align 8
  %39 = ptrtoint %opencl.image2d_t addrspace(1)* %36 to i32
  %40 = and i32 0, 7
  %41 = icmp eq i32 %40, 1
  br i1 %41, label %42, label %68

; <label>:42                                      ; preds = %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit
  %43 = icmp eq i32 0, 8
  br i1 %43, label %44, label %54

; <label>:44                                      ; preds = %42
  %45 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %46 = insertelement <2 x i32> %45, i32 %imageHeigt, i32 1
  %47 = extractelement <2 x i32> %46, i32 0
  %48 = sitofp i32 %47 to float
  %49 = insertelement <2 x float> undef, float %48, i32 0
  %50 = extractelement <2 x i32> %46, i32 1
  %51 = sitofp i32 %50 to float
  %52 = insertelement <2 x float> %49, float %51, i32 1
  %53 = fmul <2 x float> %52, %38
  br label %54

; <label>:54                                      ; preds = %44, %42
  %.01.i.i1 = phi <2 x float> [ %53, %44 ], [ %38, %42 ]
  %55 = extractelement <2 x float> %.01.i.i1, i32 0
  %56 = call float @__builtin_IB_frnd_ni(float %55) #1
  %57 = insertelement <2 x float> undef, float %56, i32 0
  %58 = extractelement <2 x float> %.01.i.i1, i32 1
  %59 = call float @__builtin_IB_frnd_ni(float %58) #1
  %60 = insertelement <2 x float> %57, float %59, i32 1
  %61 = extractelement <2 x float> %60, i32 0
  %62 = fptosi float %61 to i32
  %63 = insertelement <2 x i32> undef, i32 %62, i32 0
  %64 = extractelement <2 x float> %60, i32 1
  %65 = fptosi float %64 to i32
  %66 = insertelement <2 x i32> %63, i32 %65, i32 1
  %67 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %39, <2 x i32> %66, i32 0) #1
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit3

; <label>:68                                      ; preds = %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit
  %69 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %39, i32 %37, <2 x float> %38, float 0.000000e+00) #1
  %70 = bitcast <4 x float> %69 to <4 x i32>
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit3

_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit3: ; preds = %54, %68
  %.0.i.i2 = phi <4 x i32> [ %67, %54 ], [ %70, %68 ]
  %add = add <4 x i32> %.0.i.i, %.0.i.i2
  %71 = load i32* %i, align 4
  %72 = load <4 x i32> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <4 x i32> addrspace(1)* %72, i32 %71
  store <4 x i32> %add, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII2_CoordX1:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII2_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII2_CoordY1:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII2_Coord1]], i32 1
  ; CHECK: [[RII2_GetBufferPtr1:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RII2_CoordX1]], i32 [[RII2_CoordY1]], i32 0, i32 0, float addrspace(196608)* [[RII2_GetBufferPtr1]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RII2_CoordX2:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII2_Coord2:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII2_CoordY2:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII2_Coord2]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RII2_CoordX2]], float [[RII2_CoordY2]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII2_CoordX3:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII2_Coord3:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII2_CoordY3:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII2_Coord3]], i32 1
  ; CHECK: [[RII2_GetBufferPtr3:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RII2_CoordX3]], i32 [[RII2_CoordY3]], i32 0, i32 0, float addrspace(196608)* [[RII2_GetBufferPtr3]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RII2_CoordX4:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII2_Coord4:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII2_CoordY4:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII2_Coord4]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RII2_CoordX4]], float [[RII2_CoordY4]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagei_3(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %coord.addr = alloca <2 x i32>, align 8
  %i = alloca i32, align 4
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load <2 x i32>* %coord.addr, align 8
  %3 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  %4 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %3, <2 x i32> %2, i32 0) #1
  %5 = load i32* %i, align 4
  %6 = load <4 x i32> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <4 x i32> addrspace(1)* %6, i32 %5
  store <4 x i32> %4, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII3_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII3_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII3_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII3_Coord]], i32 1
  ; CHECK: [[RII3_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RII3_CoordX]], i32 [[RII3_CoordY]], i32 0, i32 0, float addrspace(196608)* [[RII3_GetBufferPtr]], i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imageui_1(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %smpAddress, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %sampler.addr = alloca i32, align 4
  %coord.addr = alloca <2 x i32>, align 8
  %samplerNearest = alloca i32, align 4
  %i = alloca i32, align 4
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store i32 %sampler, i32* %sampler.addr, align 4
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  store i32 0, i32* %samplerNearest, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load i32* %sampler.addr, align 4
  %3 = load <2 x i32>* %coord.addr, align 8
  %4 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  %5 = and i32 %smpAddress, 7
  %6 = icmp eq i32 %5, 2
  br i1 %6, label %7, label %16

; <label>:7                                       ; preds = %entry
  %8 = extractelement <2 x i32> %3, i32 0
  %9 = sitofp i32 %8 to float
  %10 = insertelement <2 x float> undef, float %9, i32 0
  %11 = extractelement <2 x i32> %3, i32 1
  %12 = sitofp i32 %11 to float
  %13 = insertelement <2 x float> %10, float %12, i32 1
  %14 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %4, i32 %2, <2 x float> %13, float 0.000000e+00) #1
  %15 = bitcast <4 x float> %14 to <4 x i32>
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit

; <label>:16                                      ; preds = %entry
  %17 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %4, <2 x i32> %3, i32 0) #1
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit

_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit: ; preds = %7, %16
  %.0.i = phi <4 x i32> [ %15, %7 ], [ %17, %16 ]
  %18 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %19 = load i32* %samplerNearest, align 4
  %20 = load <2 x i32>* %coord.addr, align 8
  %21 = ptrtoint %opencl.image2d_t addrspace(1)* %18 to i32
  %22 = and i32 0, 7
  %23 = icmp eq i32 %22, 2
  br i1 %23, label %24, label %33

; <label>:24                                      ; preds = %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit
  %25 = extractelement <2 x i32> %20, i32 0
  %26 = sitofp i32 %25 to float
  %27 = insertelement <2 x float> undef, float %26, i32 0
  %28 = extractelement <2 x i32> %20, i32 1
  %29 = sitofp i32 %28 to float
  %30 = insertelement <2 x float> %27, float %29, i32 1
  %31 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %21, i32 %19, <2 x float> %30, float 0.000000e+00) #1
  %32 = bitcast <4 x float> %31 to <4 x i32>
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit2

; <label>:33                                      ; preds = %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit
  %34 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %21, <2 x i32> %20, i32 0) #1
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit2

_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit2: ; preds = %24, %33
  %.0.i1 = phi <4 x i32> [ %32, %24 ], [ %34, %33 ]
  %add = add <4 x i32> %.0.i, %.0.i1
  %35 = load i32* %i, align 4
  %36 = load <4 x i32> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <4 x i32> addrspace(1)* %36, i32 %35
  store <4 x i32> %add, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIUI1_CoordX1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI1_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI1_CoordY1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI1_Coord1]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIUI1_CoordX1]], float [[RIUI1_CoordY1]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI1_CoordX2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI1_Coord2:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI1_CoordY2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI1_Coord2]], i32 1
  ; CHECK: [[RIUI1_GetBufferPtr2:%[a-zA-Z0-9]+]]  = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RIUI1_CoordX2]], i32 [[RIUI1_CoordY2]], i32 0, i32 0, float addrspace(196608)* [[RIUI1_GetBufferPtr2]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIUI1_CoordX3:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI1_Coord3:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI1_CoordY3:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI1_Coord3]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIUI1_CoordX3]], float [[RIUI1_CoordY3]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI1_CoordX4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI1_Coord4:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI1_CoordY4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI1_Coord4]], i32 1
  ; CHECK: [[RIUI1_GetBufferPtr4:%[a-zA-Z0-9]+]]  = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RIUI1_CoordX4]], i32 [[RIUI1_CoordY4]], i32 0, i32 0, float addrspace(196608)* [[RIUI1_GetBufferPtr4]], i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imageui_2(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x float> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageHeigt, i32 %imageWidth, i32 %smpAddress, i32 %smpNormalized, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %sampler.addr = alloca i32, align 4
  %coord.addr = alloca <2 x float>, align 8
  %samplerNearest = alloca i32, align 4
  %i = alloca i32, align 4
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store i32 %sampler, i32* %sampler.addr, align 4
  store <2 x float> %coord, <2 x float>* %coord.addr, align 8
  store i32 0, i32* %samplerNearest, align 4
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load i32* %sampler.addr, align 4
  %3 = load <2 x float>* %coord.addr, align 8
  %4 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  %5 = and i32 %smpAddress, 7
  %6 = icmp eq i32 %5, 1
  br i1 %6, label %7, label %33

; <label>:7                                       ; preds = %entry
  %8 = icmp eq i32 %smpNormalized, 8
  br i1 %8, label %9, label %19

; <label>:9                                       ; preds = %7
  %10 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %11 = insertelement <2 x i32> %10, i32 %imageHeigt, i32 1
  %12 = extractelement <2 x i32> %11, i32 0
  %13 = sitofp i32 %12 to float
  %14 = insertelement <2 x float> undef, float %13, i32 0
  %15 = extractelement <2 x i32> %11, i32 1
  %16 = sitofp i32 %15 to float
  %17 = insertelement <2 x float> %14, float %16, i32 1
  %18 = fmul <2 x float> %17, %3
  br label %19

; <label>:19                                      ; preds = %9, %7
  %.01.i = phi <2 x float> [ %18, %9 ], [ %3, %7 ]
  %20 = extractelement <2 x float> %.01.i, i32 0
  %21 = call float @__builtin_IB_frnd_ni(float %20) #1
  %22 = insertelement <2 x float> undef, float %21, i32 0
  %23 = extractelement <2 x float> %.01.i, i32 1
  %24 = call float @__builtin_IB_frnd_ni(float %23) #1
  %25 = insertelement <2 x float> %22, float %24, i32 1
  %26 = extractelement <2 x float> %25, i32 0
  %27 = fptosi float %26 to i32
  %28 = insertelement <2 x i32> undef, i32 %27, i32 0
  %29 = extractelement <2 x float> %25, i32 1
  %30 = fptosi float %29 to i32
  %31 = insertelement <2 x i32> %28, i32 %30, i32 1
  %32 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %4, <2 x i32> %31, i32 0) #1
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit

; <label>:33                                      ; preds = %entry
  %34 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %4, i32 %2, <2 x float> %3, float 0.000000e+00) #1
  %35 = bitcast <4 x float> %34 to <4 x i32>
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit

_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit: ; preds = %19, %33
  %.0.i = phi <4 x i32> [ %32, %19 ], [ %35, %33 ]
  %36 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %37 = load i32* %samplerNearest, align 4
  %38 = load <2 x float>* %coord.addr, align 8
  %39 = ptrtoint %opencl.image2d_t addrspace(1)* %36 to i32
  %40 = and i32 0, 7
  %41 = icmp eq i32 %40, 1
  br i1 %41, label %42, label %68

; <label>:42                                      ; preds = %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit
  %43 = icmp eq i32 0, 8
  br i1 %43, label %44, label %54

; <label>:44                                      ; preds = %42
  %45 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %46 = insertelement <2 x i32> %45, i32 %imageHeigt, i32 1
  %47 = extractelement <2 x i32> %46, i32 0
  %48 = sitofp i32 %47 to float
  %49 = insertelement <2 x float> undef, float %48, i32 0
  %50 = extractelement <2 x i32> %46, i32 1
  %51 = sitofp i32 %50 to float
  %52 = insertelement <2 x float> %49, float %51, i32 1
  %53 = fmul <2 x float> %52, %38
  br label %54

; <label>:54                                      ; preds = %44, %42
  %.01.i1 = phi <2 x float> [ %53, %44 ], [ %38, %42 ]
  %55 = extractelement <2 x float> %.01.i1, i32 0
  %56 = call float @__builtin_IB_frnd_ni(float %55) #1
  %57 = insertelement <2 x float> undef, float %56, i32 0
  %58 = extractelement <2 x float> %.01.i1, i32 1
  %59 = call float @__builtin_IB_frnd_ni(float %58) #1
  %60 = insertelement <2 x float> %57, float %59, i32 1
  %61 = extractelement <2 x float> %60, i32 0
  %62 = fptosi float %61 to i32
  %63 = insertelement <2 x i32> undef, i32 %62, i32 0
  %64 = extractelement <2 x float> %60, i32 1
  %65 = fptosi float %64 to i32
  %66 = insertelement <2 x i32> %63, i32 %65, i32 1
  %67 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %39, <2 x i32> %66, i32 0) #1
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit3

; <label>:68                                      ; preds = %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit
  %69 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %39, i32 %37, <2 x float> %38, float 0.000000e+00) #1
  %70 = bitcast <4 x float> %69 to <4 x i32>
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit3

_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit3: ; preds = %54, %68
  %.0.i2 = phi <4 x i32> [ %67, %54 ], [ %70, %68 ]
  %add = add <4 x i32> %.0.i, %.0.i2
  %71 = load i32* %i, align 4
  %72 = load <4 x i32> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <4 x i32> addrspace(1)* %72, i32 %71
  store <4 x i32> %add, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI2_CoordX1:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI2_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI2_CoordY1:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI2_Coord1]], i32 1
  ; CHECK: [[RIUI2_GetBufferPtr1:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RIUI2_CoordX1]], i32 [[RIUI2_CoordY1]], i32 0, i32 0, float addrspace(196608)* [[RIUI2_GetBufferPtr1]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIUI2_CoordX2:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI2_Coord2:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI2_CoordY2:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI2_Coord2]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIUI2_CoordX2]], float [[RIUI2_CoordY2]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI2_CoordX3:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI2_Coord3:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI2_CoordY3:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI2_Coord3]], i32 1
  ; CHECK: [[RIUI2_GetBufferPtr3:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RIUI2_CoordX3]], i32 [[RIUI2_CoordY3]], i32 0, i32 0, float addrspace(196608)* [[RIUI2_GetBufferPtr3]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIUI2_CoordX4:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI2_Coord4:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI2_CoordY4:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI2_Coord4]], i32 1
  ; CHECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIUI2_CoordX4]], float [[RIUI2_CoordY4]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imageui_3(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %coord.addr = alloca <2 x i32>, align 8
  %i = alloca i32, align 4
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load <2 x i32>* %coord.addr, align 8
  %3 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  %4 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %3, <2 x i32> %2, i32 0) #1
  %5 = load i32* %i, align 4
  %6 = load <4 x i32> addrspace(1)** %dst.addr, align 4
  %arrayidx = getelementptr inbounds <4 x i32> addrspace(1)* %6, i32 %5
  store <4 x i32> %4, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI3_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI3_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI3_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI3_Coord]], i32 1
  ; CHECK: [[RIUI3_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldui.ptr.p196608f32(i32 [[RIUI3_CoordX]], i32 [[RIUI3_CoordY]], i32 0, i32 0, float addrspace(196608)* [[RIUI3_GetBufferPtr]], i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_write_imagef_1(<4 x float> addrspace(1)* %src, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %src.addr = alloca <4 x float> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %coord.addr = alloca <2 x i32>, align 8
  %i = alloca i32, align 4
  store <4 x float> addrspace(1)* %src, <4 x float> addrspace(1)** %src.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load <2 x i32>* %coord.addr, align 8
  %3 = load i32* %i, align 4
  %4 = load <4 x float> addrspace(1)** %src.addr, align 4
  %arrayidx = getelementptr inbounds <4 x float> addrspace(1)* %4, i32 %3
  %5 = load <4 x float> addrspace(1)* %arrayidx, align 16
  %6 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  call void @__builtin_IB_write_2d_f(i32 %6, i32 1, i32 0, <2 x i32> %2, <4 x float> %5, i32 0) #1
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_write_2d_f
  ; CHECK: [[WIF1_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(131074)* @genx.GenISA.GetBufferPtr.p131074f32(i32 2, i32 1)
  ; CHECK: [[WIF1_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32>  [[WIF1_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[WIF1_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[WIF1_Coord]], i32 1
  ; CHECK: [[WIF1_ColorX:%[a-zA-Z0-9]+]] = extractelement <4 x float> %5, i32 0
  ; CHECK: [[WIF1_ColorY:%[a-zA-Z0-9]+]] = extractelement <4 x float> %5, i32 1
  ; CHECK: [[WIF1_ColorZ:%[a-zA-Z0-9]+]] = extractelement <4 x float> %5, i32 2
  ; CHECK: [[WIF1_ColorW:%[a-zA-Z0-9]+]] = extractelement <4 x float> %5, i32 3
  ; CHECK: call void @genx.GenISA.typedwrite.p131074f32(float addrspace(131074)* [[WIF1_GetBufferPtr]], i32 [[WIF1_CoordX]], i32 [[WIF1_CoordY]], i32 0, i32 0, float [[WIF1_ColorX]], float [[WIF1_ColorY]], float [[WIF1_ColorZ]], float [[WIF1_ColorW]])

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_write_imagei_1(<4 x i32> addrspace(1)* %src, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %src.addr = alloca <4 x i32> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %coord.addr = alloca <2 x i32>, align 8
  %i = alloca i32, align 4
  store <4 x i32> addrspace(1)* %src, <4 x i32> addrspace(1)** %src.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load <2 x i32>* %coord.addr, align 8
  %3 = load i32* %i, align 4
  %4 = load <4 x i32> addrspace(1)** %src.addr, align 4
  %arrayidx = getelementptr inbounds <4 x i32> addrspace(1)* %4, i32 %3
  %5 = load <4 x i32> addrspace(1)* %arrayidx, align 16
  %6 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  call void @__builtin_IB_write_2d_i(i32 %6, i32 1, i32 1, <2 x i32> %2, <4 x i32> %5, i32 0) #1
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_write_2d_i
  ; CHECK: [[WII1_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(131074)* @genx.GenISA.GetBufferPtr.p131074f32(i32 2, i32 1)
  ; CHECK: [[WII1_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32>  [[WII1_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[WII1_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[WII1_Coord]], i32 1
  ; CHECK: [[WII1_floatColor:%[a-zA-Z0-9]+]] = bitcast <4 x i32> %5 to <4 x float>
  ; CHECK: [[WII1_ColorX:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WII1_floatColor]], i32 0
  ; CHECK: [[WII1_ColorY:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WII1_floatColor]], i32 1
  ; CHECK: [[WII1_ColorZ:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WII1_floatColor]], i32 2
  ; CHECK: [[WII1_ColorW:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WII1_floatColor]], i32 3
  ; CHECK: call void @genx.GenISA.typedwrite.p131074f32(float addrspace(131074)* [[WII1_GetBufferPtr]], i32 [[WII1_CoordX]], i32 [[WII1_CoordY]], i32 0, i32 0, float [[WII1_ColorX]], float [[WII1_ColorY]], float [[WII1_ColorZ]], float [[WII1_ColorW]])

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_write_imageui_1(<4 x i32> addrspace(1)* %src, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %src.addr = alloca <4 x i32> addrspace(1)*, align 4
  %img.addr = alloca %opencl.image2d_t addrspace(1)*, align 4
  %coord.addr = alloca <2 x i32>, align 8
  %i = alloca i32, align 4
  store <4 x i32> addrspace(1)* %src, <4 x i32> addrspace(1)** %src.addr, align 4
  store %opencl.image2d_t addrspace(1)* %img, %opencl.image2d_t addrspace(1)** %img.addr, align 4
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = zext i16 %localIdX to i32
  store i32 %0, i32* %i, align 4
  %1 = load %opencl.image2d_t addrspace(1)** %img.addr, align 4
  %2 = load <2 x i32>* %coord.addr, align 8
  %3 = load i32* %i, align 4
  %4 = load <4 x i32> addrspace(1)** %src.addr, align 4
  %arrayidx = getelementptr inbounds <4 x i32> addrspace(1)* %4, i32 %3
  %5 = load <4 x i32> addrspace(1)* %arrayidx, align 16
  %6 = ptrtoint %opencl.image2d_t addrspace(1)* %1 to i32
  call void @__builtin_IB_write_2d_ui(i32 %6, i32 1, i32 2, <2 x i32> %2, <4 x i32> %5, i32 0) #1
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_write_2d_ui
  ; CHECK: [[WIUI1_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(131074)* @genx.GenISA.GetBufferPtr.p131074f32(i32 2, i32 1)
  ; CHECK: [[WIUI1_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32>  [[WIUI1_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[WIUI1_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[WIUI1_Coord]], i32 1
  ; CHECK: [[WIUI1_floatColor:%[a-zA-Z0-9]+]] = bitcast <4 x i32> %5 to <4 x float>
  ; CHECK: [[WIUI1_ColorX:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WIUI1_floatColor]], i32 0
  ; CHECK: [[WIUI1_ColorY:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WIUI1_floatColor]], i32 1
  ; CHECK: [[WIUI1_ColorZ:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WIUI1_floatColor]], i32 2
  ; CHECK: [[WIUI1_ColorW:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WIUI1_floatColor]], i32 3
  ; CHECK: call void @genx.GenISA.typedwrite.p131074f32(float addrspace(131074)* [[WIUI1_GetBufferPtr]], i32 [[WIUI1_CoordX]], i32 [[WIUI1_CoordY]], i32 0, i32 0, float [[WIUI1_ColorX]], float [[WIUI1_ColorY]], float [[WIUI1_ColorZ]], float [[WIUI1_ColorW]])

  ; CHECK: ret void
}

declare void @__builtin_IB_write_2d_ui(i32, i32, i32, <2 x i32>, <4 x i32>, i32)

declare void @__builtin_IB_write_2d_i(i32, i32, i32, <2 x i32>, <4 x i32>, i32)

declare void @__builtin_IB_write_2d_f(i32, i32, i32, <2 x i32>, <4 x float>, i32)

declare <4 x i32> @__builtin_IB_OCL_2d_ldui(i32, <2 x i32>, i32)

declare i32 @__builtin_IB_get_address_mode(i32)

declare i32 @__builtin_IB_is_normalized_coords(i32)

declare <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

declare float @__builtin_IB_frnd_ni(float)

declare <4 x float> @__builtin_IB_OCL_2d_ld(i32, <2 x i32>, i32)

declare i32 @__builtin_IB_get_snap_wa_reqd(i32)

declare i32 @__builtin_IB_get_image_array_size(i32)

declare i32 @__builtin_IB_get_image_width(i32)

declare i32 @__builtin_IB_get_image_height(i32)

declare i32 @__builtin_IB_get_image_channel_order(i32)

declare i32 @__builtin_IB_get_image_channel_data_type(i32)

declare i32 @__builtin_IB_get_image_depth(i32)

declare i16 @__builtin_IB_get_local_id_x()

attributes #0 = { alwaysinline nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-frame-pointer-elim-non-leaf"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }

!igc.version = !{!0}
!igc.input.ir = !{!1}
!igc.input.lang.info = !{!1}
!igc.functions = !{!2, !29, !33, !39, !43, !47, !54, !60, !73, !82, !90, !96, !104, !108, !112, !116, !120, !129, !131}
!igc.compiler.options = !{!133, !134, !135}
!igc.inline.constants = !{}
!igc.inline.globals = !{}
!igc.inline.programscope.offsets = !{}
!igc.global.pointer.info = !{}
!igc.constant.pointer.info = !{}
!igc.enable.FP_CONTRACT = !{}

!0 = metadata !{i32 1, i32 0}
!1 = metadata !{metadata !"ocl", i32 1, i32 2}
!2 = metadata !{void (i32 addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_width, metadata !3}
!3 = metadata !{metadata !4, metadata !5, metadata !14, metadata !23, metadata !24, metadata !25, metadata !26, metadata !27, metadata !28}
!4 = metadata !{metadata !"function_type", i32 0}
!5 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !11, metadata !13}
!6 = metadata !{i32 0}
!7 = metadata !{i32 1}
!8 = metadata !{i32 5}
!9 = metadata !{i32 6}
!10 = metadata !{i32 7}
!11 = metadata !{i32 18, metadata !12}
!12 = metadata !{metadata !"explicit_arg_num", i32 1}
!13 = metadata !{i32 10}
!14 = metadata !{metadata !"resource_alloc", metadata !15, metadata !16, metadata !17, metadata !18}
!15 = metadata !{metadata !"uavs_num", i32 2}
!16 = metadata !{metadata !"srvs_num", i32 1}
!17 = metadata !{metadata !"samplers_num", i32 0}
!18 = metadata !{metadata !"arg_allocs", metadata !19, metadata !20, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !22}
!19 = metadata !{i32 1, null, i32 0}
!20 = metadata !{i32 2, i32 0, i32 0}
!21 = metadata !{i32 0, null, null}
!22 = metadata !{i32 1, null, i32 1}
!23 = metadata !{metadata !"opencl_kernel_arg_addr_space", i32 1, i32 1}
!24 = metadata !{metadata !"opencl_kernel_arg_access_qual", metadata !"none", metadata !"read_only"}
!25 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"int*", metadata !"image2d_t"}
!26 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"int*", metadata !"image2d_t"}
!27 = metadata !{metadata !"opencl_kernel_arg_type_qual", metadata !"", metadata !""}
!28 = metadata !{metadata !"opencl_kernel_arg_name", metadata !"dst", metadata !"img"}
!29 = metadata !{void (i32 addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_height, metadata !30}
!30 = metadata !{metadata !4, metadata !31, metadata !14, metadata !23, metadata !24, metadata !25, metadata !26, metadata !27, metadata !28}
!31 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !32, metadata !13}
!32 = metadata !{i32 17, metadata !12}
!33 = metadata !{void (i32 addrspace(1)*, %opencl.image3d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_depth, metadata !34}
!34 = metadata !{metadata !4, metadata !35, metadata !14, metadata !23, metadata !24, metadata !37, metadata !38, metadata !27, metadata !28}
!35 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !36, metadata !13}
!36 = metadata !{i32 19, metadata !12}
!37 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"int*", metadata !"image3d_t"}
!38 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"int*", metadata !"image3d_t"}
!39 = metadata !{void (i32 addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_channel_data_type, metadata !40}
!40 = metadata !{metadata !4, metadata !41, metadata !14, metadata !23, metadata !24, metadata !25, metadata !26, metadata !27, metadata !28}
!41 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !42, metadata !13}
!42 = metadata !{i32 21, metadata !12}
!43 = metadata !{void (i32 addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_channel_order, metadata !44}
!44 = metadata !{metadata !4, metadata !45, metadata !14, metadata !23, metadata !24, metadata !25, metadata !26, metadata !27, metadata !28}
!45 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !46, metadata !13}
!46 = metadata !{i32 22, metadata !12}
!47 = metadata !{void (<2 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i32, i8*)* @test_image_dim, metadata !48}
!48 = metadata !{metadata !4, metadata !49, metadata !50, metadata !23, metadata !24, metadata !52, metadata !53, metadata !27, metadata !28}
!49 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !32, metadata !11, metadata !13}
!50 = metadata !{metadata !"resource_alloc", metadata !15, metadata !16, metadata !17, metadata !51}
!51 = metadata !{metadata !"arg_allocs", metadata !19, metadata !20, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !22}
!52 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"int2*", metadata !"image2d_t"}
!53 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"int2*", metadata !"image2d_t"}
!54 = metadata !{void (i32 addrspace(1)*, %opencl.image2d_array_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_array_size, metadata !55}
!55 = metadata !{metadata !4, metadata !56, metadata !14, metadata !23, metadata !24, metadata !58, metadata !59, metadata !27, metadata !28}
!56 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !57, metadata !13}
!57 = metadata !{i32 23, metadata !12}
!58 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"size_t*", metadata !"image2d_array_t"}
!59 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"uint*", metadata !"image2d_array_t"}
!60 = metadata !{void (<4 x float> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_read_imagef_1, metadata !61}
!61 = metadata !{metadata !4, metadata !62, metadata !63, metadata !67, metadata !68, metadata !69, metadata !70, metadata !71, metadata !72}
!62 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !13}
!63 = metadata !{metadata !"resource_alloc", metadata !15, metadata !16, metadata !64, metadata !65}
!64 = metadata !{metadata !"samplers_num", i32 1}
!65 = metadata !{metadata !"arg_allocs", metadata !19, metadata !20, metadata !66, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !22}
!66 = metadata !{i32 3, i32 0, i32 0}
!67 = metadata !{metadata !"opencl_kernel_arg_addr_space", i32 1, i32 1, i32 0, i32 0}
!68 = metadata !{metadata !"opencl_kernel_arg_access_qual", metadata !"none", metadata !"read_only", metadata !"none", metadata !"none"}
!69 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"float4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"int2"}
!70 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"float4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"int2"}
!71 = metadata !{metadata !"opencl_kernel_arg_type_qual", metadata !"", metadata !"", metadata !"", metadata !""}
!72 = metadata !{metadata !"opencl_kernel_arg_name", metadata !"dst", metadata !"img", metadata !"sampler", metadata !"coord"}
!73 = metadata !{void (<4 x float> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x float>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_read_imagef_2, metadata !74}
!74 = metadata !{metadata !4, metadata !75, metadata !78, metadata !67, metadata !68, metadata !80, metadata !81, metadata !71, metadata !72}
!75 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !76, metadata !13}
!76 = metadata !{i32 26, metadata !77}
!77 = metadata !{metadata !"explicit_arg_num", i32 2}
!78 = metadata !{metadata !"resource_alloc", metadata !15, metadata !16, metadata !64, metadata !79}
!79 = metadata !{metadata !"arg_allocs", metadata !19, metadata !20, metadata !66, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !22}
!80 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"float4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"float2"}
!81 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"float4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"float2"}
!82 = metadata !{void (<4 x float> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_read_imagef_3, metadata !83}
!83 = metadata !{metadata !4, metadata !62, metadata !14, metadata !84, metadata !85, metadata !86, metadata !87, metadata !88, metadata !89}
!84 = metadata !{metadata !"opencl_kernel_arg_addr_space", i32 1, i32 1, i32 0}
!85 = metadata !{metadata !"opencl_kernel_arg_access_qual", metadata !"none", metadata !"read_only", metadata !"none"}
!86 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"float4*", metadata !"image2d_t", metadata !"int2"}
!87 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"float4*", metadata !"image2d_t", metadata !"int2"}
!88 = metadata !{metadata !"opencl_kernel_arg_type_qual", metadata !"", metadata !"", metadata !""}
!89 = metadata !{metadata !"opencl_kernel_arg_name", metadata !"dst", metadata !"img", metadata !"coord"}
!90 = metadata !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_read_imagei_1, metadata !91}
!91 = metadata !{metadata !4, metadata !92, metadata !78, metadata !67, metadata !68, metadata !94, metadata !95, metadata !71, metadata !72}
!92 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !93, metadata !13}
!93 = metadata !{i32 24, metadata !77}
!94 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"int4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"int2"}
!95 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"int4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"int2"}
!96 = metadata !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x float>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i32, i32, i32, i8*)* @test_read_imagei_2, metadata !97}
!97 = metadata !{metadata !4, metadata !98, metadata !100, metadata !67, metadata !68, metadata !102, metadata !103, metadata !71, metadata !72}
!98 = metadata !{metadata !"implicit_arg_desc", metadata !6, metadata !7, metadata !8, metadata !9, metadata !10, metadata !32, metadata !11, metadata !93, metadata !99, metadata !13}
!99 = metadata !{i32 25, metadata !77}
!100 = metadata !{metadata !"resource_alloc", metadata !15, metadata !16, metadata !64, metadata !101}
!101 = metadata !{metadata !"arg_allocs", metadata !19, metadata !20, metadata !66, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !22}
!102 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"int4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"float2"}
!103 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"int4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"float2"}
!104 = metadata !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_read_imagei_3, metadata !105}
!105 = metadata !{metadata !4, metadata !62, metadata !14, metadata !84, metadata !85, metadata !106, metadata !107, metadata !88, metadata !89}
!106 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"int4*", metadata !"image2d_t", metadata !"int2"}
!107 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"int4*", metadata !"image2d_t", metadata !"int2"}
!108 = metadata !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_read_imageui_1, metadata !109}
!109 = metadata !{metadata !4, metadata !92, metadata !78, metadata !67, metadata !68, metadata !110, metadata !111, metadata !71, metadata !72}
!110 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"uint4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"int2"}
!111 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"uint4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"int2"}
!112 = metadata !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x float>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i32, i32, i32, i8*)* @test_read_imageui_2, metadata !113}
!113 = metadata !{metadata !4, metadata !98, metadata !100, metadata !67, metadata !68, metadata !114, metadata !115, metadata !71, metadata !72}
!114 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"uint4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"float2"}
!115 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"uint4*", metadata !"image2d_t", metadata !"sampler_t", metadata !"float2"}
!116 = metadata !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_read_imageui_3, metadata !117}
!117 = metadata !{metadata !4, metadata !62, metadata !14, metadata !84, metadata !85, metadata !118, metadata !119, metadata !88, metadata !89}
!118 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"uint4*", metadata !"image2d_t", metadata !"int2"}
!119 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"uint4*", metadata !"image2d_t", metadata !"int2"}
!120 = metadata !{void (<4 x float> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_write_imagef_1, metadata !121}
!121 = metadata !{metadata !4, metadata !62, metadata !122, metadata !84, metadata !127, metadata !86, metadata !87, metadata !88, metadata !128}
!122 = metadata !{metadata !"resource_alloc", metadata !123, metadata !124, metadata !17, metadata !125}
!123 = metadata !{metadata !"uavs_num", i32 3}
!124 = metadata !{metadata !"srvs_num", i32 0}
!125 = metadata !{metadata !"arg_allocs", metadata !19, metadata !126, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !21, metadata !22}
!126 = metadata !{i32 1, i32 0, i32 2}
!127 = metadata !{metadata !"opencl_kernel_arg_access_qual", metadata !"none", metadata !"write_only", metadata !"none"}
!128 = metadata !{metadata !"opencl_kernel_arg_name", metadata !"src", metadata !"img", metadata !"coord"}
!129 = metadata !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_write_imagei_1, metadata !130}
!130 = metadata !{metadata !4, metadata !62, metadata !122, metadata !84, metadata !127, metadata !106, metadata !107, metadata !88, metadata !128}
!131 = metadata !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_write_imageui_1, metadata !132}
!132 = metadata !{metadata !4, metadata !62, metadata !122, metadata !84, metadata !127, metadata !118, metadata !119, metadata !88, metadata !128}
!133 = metadata !{metadata !"-opt-disable"}
!134 = metadata !{metadata !"-std=CL1.2"}
!135 = metadata !{metadata !"-kernel-arg-info"}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CL source code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;__kernel void test_image_width(global int *dst, read_only image2d_t img)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = get_image_width(img);
;;}
;;
;;__kernel void test_image_height(global int *dst, read_only image2d_t img)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = get_image_height(img);
;;}
;;
;;__kernel void test_image_depth(global int *dst, read_only image3d_t img)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = get_image_depth(img);
;;}
;;
;;__kernel void test_image_channel_data_type(global int *dst, read_only image2d_t img)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = get_image_channel_data_type(img);
;;}
;;
;;__kernel void test_image_channel_order(global int *dst, read_only image2d_t img)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = get_image_channel_order(img);
;;}
;;
;;__kernel void test_image_dim(global int2 *dst, read_only image2d_t img)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = get_image_dim(img);
;;}
;;
;;__kernel void test_image_array_size(global size_t *dst, read_only image2d_array_t img)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = get_image_array_size(img);
;;}
;;
;;__kernel void test_read_imagef_1(global float4 *dst, read_only image2d_t img, sampler_t sampler, int2 coord)
;;{
;;    const sampler_t samplerNearest = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;
;;    int i = get_local_id( 0 );
;;    dst[i] = read_imagef (img, sampler, coord) + read_imagef (img, samplerNearest, coord);
;;}
;;
;;__kernel void test_read_imagef_2(global float4 *dst, read_only image2d_t img, sampler_t sampler, float2 coord)
;;{
;;    const sampler_t samplerNearest = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;
;;    int i = get_local_id( 0 );
;;    dst[i] = read_imagef (img, sampler, coord) + read_imagef (img, samplerNearest, coord);
;;}
;;
;;__kernel void test_read_imagef_3(global float4 *dst, read_only image2d_t img, int2 coord)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = read_imagef (img, coord);
;;}
;;
;;__kernel void test_read_imagei_1(global int4 *dst, read_only image2d_t img, sampler_t sampler, int2 coord)
;;{
;;    const sampler_t samplerNearest = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;
;;    int i = get_local_id( 0 );
;;    dst[i] = read_imagei (img, sampler, coord) + read_imagei (img, samplerNearest, coord);
;;}
;;
;;__kernel void test_read_imagei_2(global int4 *dst, read_only image2d_t img, sampler_t sampler, float2 coord)
;;{
;;    const sampler_t samplerNearest = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;
;;    int i = get_local_id( 0 );
;;    dst[i] = read_imagei (img, sampler, coord) + read_imagei (img, samplerNearest, coord);
;;}
;;
;;__kernel void test_read_imagei_3(global int4 *dst, read_only image2d_t img, int2 coord)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = read_imagei (img, coord);
;;}
;;
;;__kernel void test_read_imageui_1(global uint4 *dst, read_only image2d_t img, sampler_t sampler, int2 coord)
;;{
;;    const sampler_t samplerNearest = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;
;;    int i = get_local_id( 0 );
;;    dst[i] = read_imageui (img, sampler, coord) + read_imageui (img, samplerNearest, coord);
;;}
;;
;;__kernel void test_read_imageui_2(global uint4 *dst, read_only image2d_t img, sampler_t sampler, float2 coord)
;;{
;;   const sampler_t samplerNearest = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;
;;    int i = get_local_id( 0 );
;;    dst[i] = read_imageui (img, sampler, coord) + read_imageui (img, samplerNearest, coord);
;;}
;;
;;__kernel void test_read_imageui_3(global uint4 *dst, read_only image2d_t img, int2 coord)
;;{
;;    int i = get_local_id( 0 );
;;    dst[i] = read_imageui (img, coord);
;;}
;;
;;__kernel void test_write_imagef_1(global float4 *src, write_only image2d_t img, int2 coord)
;;{
;;    int i = get_local_id( 0 );
;;    write_imagef (img, coord, src[i]);
;;}
;;
;;__kernel void test_write_imagei_1(global int4 *src, write_only image2d_t img, int2 coord)
;;{
;;    int i = get_local_id( 0 );
;;    write_imagei (img, coord, src[i]);
;;}
;;
;;__kernel void test_write_imageui_1(global uint4 *src, write_only image2d_t img, int2 coord)
;;{
;;    int i = get_local_id( 0 );
;;    write_imageui (img, coord, src[i]);
;;}
