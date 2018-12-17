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
; RUN: igc_opt -igc-conv-ocl-to-common -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that BuiltinsConverter pass handles images/samplers rights.
;;
;; Input
;;  - Test contains different types of image/sampler access functions
;;  - image and sampler are not stored in alloca (mem2reg pass is enabled)
;;
;; Expected output
;;  - Pass does not crash
;;  - image/sampler built-ins are converted (resolved) as expected.
;;
;; Note: this test was semi-auto generated from cl file below.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN8"

%opencl.image2d_t = type opaque
%opencl.image3d_t = type opaque
%opencl.image2d_array_t = type opaque

; Function Attrs: alwaysinline nounwind
define void @test_image_width(i32 addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageWidth, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i32 %0
  store i32 %imageWidth, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_height(i32 addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageHeigt, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i32 %0
  store i32 %imageHeigt, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_depth(i32 addrspace(1)* %dst, %opencl.image3d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageDepth, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i32 %0
  store i32 %imageDepth, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_channel_data_type(i32 addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageDataType, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i32 %0
  store i32 %imageDataType, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_channel_order(i32 addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageOrder, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i32 %0
  store i32 %imageOrder, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_dim(<2 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageHeigt, i32 %imageWidth, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %2 = insertelement <2 x i32> %1, i32 %imageHeigt, i32 1
  %arrayidx = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %dst, i32 %0
  store <2 x i32> %2, <2 x i32> addrspace(1)* %arrayidx, align 8
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_image_array_size(i32 addrspace(1)* %dst, %opencl.image2d_array_t addrspace(1)* %img, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageArrSize, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i32 %0
  store i32 %imageArrSize, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagef_1(<4 x float> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %2 = extractelement <2 x i32> %coord, i32 0
  %3 = sitofp i32 %2 to float
  %4 = insertelement <2 x float> undef, float %3, i32 0
  %5 = extractelement <2 x i32> %coord, i32 1
  %6 = sitofp i32 %5 to float
  %7 = insertelement <2 x float> %4, float %6, i32 1
  %8 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %1, i32 %sampler, <2 x float> %7, float 0.000000e+00) #1
  %9 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %10 = extractelement <2 x i32> %coord, i32 0
  %11 = sitofp i32 %10 to float
  %12 = insertelement <2 x float> undef, float %11, i32 0
  %13 = extractelement <2 x i32> %coord, i32 1
  %14 = sitofp i32 %13 to float
  %15 = insertelement <2 x float> %12, float %14, i32 1
  %16 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %9, i32 0, <2 x float> %15, float 0.000000e+00) #1
  %add = fadd <4 x float> %8, %16
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %dst, i32 %0
  store <4 x float> %add, <4 x float> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIF1_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF1_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIF1_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF1_Coord]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524288i32(float 0.000000e+00, float [[RIF1_CoordX]], float [[RIF1_CoordY]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524288)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIF1_CoordX]], float [[RIF1_CoordY]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIF1_CoordX1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF1_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIF1_CoordY1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF1_Coord1]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524289i32(float 0.000000e+00, float [[RIF1_CoordX1]], float [[RIF1_CoordY1]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524289)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIF1_CoordX1]], float [[RIF1_CoordY1]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagef_2(<4 x float> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x float> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %smpSnapWA, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %2 = icmp eq i32 %smpSnapWA, 0
  br i1 %2, label %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit, label %3

; <label>:3                                       ; preds = %entry
  %4 = extractelement <2 x float> %coord, i32 0
  %5 = fcmp olt float %4, 0.000000e+00
  %6 = select i1 %5, float -1.000000e+00, float %4
  %7 = insertelement <2 x float> undef, float %6, i32 0
  %8 = extractelement <2 x float> %coord, i32 1
  %9 = fcmp olt float %8, 0.000000e+00
  %10 = select i1 %9, float -1.000000e+00, float %8
  %11 = insertelement <2 x float> %7, float %10, i32 1
  br label %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit

_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit: ; preds = %entry, %3
  %.0.i = phi <2 x float> [ %11, %3 ], [ %coord, %entry ]
  %12 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %1, i32 %sampler, <2 x float> %.0.i, float 0.000000e+00) #1
  %13 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %14 = icmp eq i32 0, 0
  br i1 %14, label %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit2, label %15

; <label>:15                                      ; preds = %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit
  %16 = extractelement <2 x float> %coord, i32 0
  %17 = fcmp olt float %16, 0.000000e+00
  %18 = select i1 %17, float -1.000000e+00, float %16
  %19 = insertelement <2 x float> undef, float %18, i32 0
  %20 = extractelement <2 x float> %coord, i32 1
  %21 = fcmp olt float %20, 0.000000e+00
  %22 = select i1 %21, float -1.000000e+00, float %20
  %23 = insertelement <2 x float> %19, float %22, i32 1
  br label %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit2

_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit2: ; preds = %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit, %15
  %.0.i1 = phi <2 x float> [ %23, %15 ], [ %coord, %_Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit ]
  %24 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %13, i32 0, <2 x float> %.0.i1, float 0.000000e+00) #1
  %add = fadd <4 x float> %12, %24
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %dst, i32 %0
  store <4 x float> %add, <4 x float> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK: _Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit: ; preds
  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIF2_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF2_Coord:%[a-zA-Z0-9.]+]], i32 0
  ; CHECK: [[RIF2_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF2_Coord]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524288i32(float 0.000000e+00, float [[RIF2_CoordX]], float [[RIF2_CoordY]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524288)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIF2_CoordX]], float [[RIF2_CoordY]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK: _Z11read_imagef11ocl_image2d11ocl_samplerDv2_f.exit2: ; preds
  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIF2_CoordX1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF2_Coord:%[a-zA-Z0-9.]+]], i32 0
  ; CHECK: [[RIF2_CoordY1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIF2_Coord]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524289i32(float 0.000000e+00, float [[RIF2_CoordX1]], float [[RIF2_CoordY1]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524289)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIF2_CoordX1]], float [[RIF2_CoordY1]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagef_3(<4 x float> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = bitcast %opencl.image2d_t addrspace(1)* %img to i8 addrspace(1)*
  %2 = ptrtoint i8 addrspace(1)* %1 to i32
  %3 = call <4 x float> @__builtin_IB_OCL_2d_ld(i32 %2, <2 x i32> %coord, i32 0) #1
  %4 = bitcast <4 x float> %3 to <4 x i32>
  %5 = and <4 x i32> %4, <i32 -2139095041, i32 -2139095041, i32 -2139095041, i32 -2139095041>
  %6 = icmp eq <4 x i32> %5, %4
  %7 = sext <4 x i1> %6 to <4 x i32>
  %8 = and <4 x i32> %4, <i32 -2147483648, i32 -2147483648, i32 -2147483648, i32 -2147483648>
  %9 = bitcast <4 x i32> %8 to <4 x float>
  %10 = icmp slt <4 x i32> %7, zeroinitializer
  %11 = select <4 x i1> %10, <4 x float> %9, <4 x float> %3
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %dst, i32 %0
  store <4 x float> %11, <4 x float> addrspace(1)* %arrayidx, align 16
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
  %0 = zext i16 %localIdX to i32
  %1 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %2 = and i32 %smpAddress, 7
  %3 = icmp eq i32 %2, 2
  br i1 %3, label %4, label %13

; <label>:4                                       ; preds = %entry
  %5 = extractelement <2 x i32> %coord, i32 0
  %6 = sitofp i32 %5 to float
  %7 = insertelement <2 x float> undef, float %6, i32 0
  %8 = extractelement <2 x i32> %coord, i32 1
  %9 = sitofp i32 %8 to float
  %10 = insertelement <2 x float> %7, float %9, i32 1
  %11 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %1, i32 %sampler, <2 x float> %10, float 0.000000e+00) #1
  %12 = bitcast <4 x float> %11 to <4 x i32>
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit

; <label>:13                                      ; preds = %entry
  %14 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %1, <2 x i32> %coord, i32 0) #1
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit

_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit: ; preds = %4, %13
  %.0.i.i = phi <4 x i32> [ %12, %4 ], [ %14, %13 ]
  %15 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %16 = and i32 0, 7
  %17 = icmp eq i32 %16, 2
  br i1 %17, label %18, label %27

; <label>:18                                      ; preds = %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit
  %19 = extractelement <2 x i32> %coord, i32 0
  %20 = sitofp i32 %19 to float
  %21 = insertelement <2 x float> undef, float %20, i32 0
  %22 = extractelement <2 x i32> %coord, i32 1
  %23 = sitofp i32 %22 to float
  %24 = insertelement <2 x float> %21, float %23, i32 1
  %25 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %15, i32 0, <2 x float> %24, float 0.000000e+00) #1
  %26 = bitcast <4 x float> %25 to <4 x i32>
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit2

; <label>:27                                      ; preds = %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit
  %28 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %15, <2 x i32> %coord, i32 0) #1
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit2

_Z11read_imagei11ocl_image2d11ocl_samplerDv2_i.exit2: ; preds = %18, %27
  %.0.i.i1 = phi <4 x i32> [ %26, %18 ], [ %28, %27 ]
  %add = add <4 x i32> %.0.i.i, %.0.i.i1
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %dst, i32 %0
  store <4 x i32> %add, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RII1_CoordX1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII1_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII1_CoordY1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII1_Coord1]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524288i32(float 0.000000e+00, float [[RII1_CoordX1]], float [[RII1_CoordY1]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524288)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RII1_CoordX1]], float [[RII1_CoordY1]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII1_CoordX2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII1_Coord2:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII1_CoordY2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII1_Coord2]], i32 1
  ; CHECK: [[RII1_GetBufferPtr2:%[a-zA-Z0-9]+]]  = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RII1_CoordX2]], i32 [[RII1_CoordY2]], i32 0, i32 0, float addrspace(196608)* [[RII1_GetBufferPtr2]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RII1_CoordX3:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII1_Coord3:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII1_CoordY3:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII1_Coord3]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524289i32(float 0.000000e+00, float [[RII1_CoordX3]], float [[RII1_CoordY3]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524289)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RII1_CoordX3]], float [[RII1_CoordY3]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII1_CoordX4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII1_Coord4:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII1_CoordY4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII1_Coord4]], i32 1
  ; CHECK: [[RII1_GetBufferPtr4:%[a-zA-Z0-9]+]]  = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RII1_CoordX4]], i32 [[RII1_CoordY4]], i32 0, i32 0, float addrspace(196608)* [[RII1_GetBufferPtr4]], i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagei_2(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x float> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageHeigt, i32 %imageWidth, i32 %smpAddress, i32 %smpNormalized, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %2 = and i32 %smpAddress, 7
  %3 = icmp eq i32 %2, 1
  br i1 %3, label %4, label %30

; <label>:4                                       ; preds = %entry
  %5 = icmp eq i32 %smpNormalized, 8
  br i1 %5, label %6, label %16

; <label>:6                                       ; preds = %4
  %7 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %8 = insertelement <2 x i32> %7, i32 %imageHeigt, i32 1
  %9 = extractelement <2 x i32> %8, i32 0
  %10 = sitofp i32 %9 to float
  %11 = insertelement <2 x float> undef, float %10, i32 0
  %12 = extractelement <2 x i32> %8, i32 1
  %13 = sitofp i32 %12 to float
  %14 = insertelement <2 x float> %11, float %13, i32 1
  %15 = fmul <2 x float> %14, %coord
  br label %16

; <label>:16                                      ; preds = %6, %4
  %.01.i.i = phi <2 x float> [ %15, %6 ], [ %coord, %4 ]
  %17 = extractelement <2 x float> %.01.i.i, i32 0
  %18 = call float @__builtin_IB_frnd_ni(float %17) #1
  %19 = insertelement <2 x float> undef, float %18, i32 0
  %20 = extractelement <2 x float> %.01.i.i, i32 1
  %21 = call float @__builtin_IB_frnd_ni(float %20) #1
  %22 = insertelement <2 x float> %19, float %21, i32 1
  %23 = extractelement <2 x float> %22, i32 0
  %24 = fptosi float %23 to i32
  %25 = insertelement <2 x i32> undef, i32 %24, i32 0
  %26 = extractelement <2 x float> %22, i32 1
  %27 = fptosi float %26 to i32
  %28 = insertelement <2 x i32> %25, i32 %27, i32 1
  %29 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %1, <2 x i32> %28, i32 0) #1
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit

; <label>:30                                      ; preds = %entry
  %31 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %1, i32 %sampler, <2 x float> %coord, float 0.000000e+00) #1
  %32 = bitcast <4 x float> %31 to <4 x i32>
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit

_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit: ; preds = %16, %30
  %.0.i.i = phi <4 x i32> [ %29, %16 ], [ %32, %30 ]
  %33 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %34 = and i32 0, 7
  %35 = icmp eq i32 %34, 1
  br i1 %35, label %36, label %62

; <label>:36                                      ; preds = %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit
  %37 = icmp eq i32 0, 8
  br i1 %37, label %38, label %48

; <label>:38                                      ; preds = %36
  %39 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %40 = insertelement <2 x i32> %39, i32 %imageHeigt, i32 1
  %41 = extractelement <2 x i32> %40, i32 0
  %42 = sitofp i32 %41 to float
  %43 = insertelement <2 x float> undef, float %42, i32 0
  %44 = extractelement <2 x i32> %40, i32 1
  %45 = sitofp i32 %44 to float
  %46 = insertelement <2 x float> %43, float %45, i32 1
  %47 = fmul <2 x float> %46, %coord
  br label %48

; <label>:48                                      ; preds = %38, %36
  %.01.i.i1 = phi <2 x float> [ %47, %38 ], [ %coord, %36 ]
  %49 = extractelement <2 x float> %.01.i.i1, i32 0
  %50 = call float @__builtin_IB_frnd_ni(float %49) #1
  %51 = insertelement <2 x float> undef, float %50, i32 0
  %52 = extractelement <2 x float> %.01.i.i1, i32 1
  %53 = call float @__builtin_IB_frnd_ni(float %52) #1
  %54 = insertelement <2 x float> %51, float %53, i32 1
  %55 = extractelement <2 x float> %54, i32 0
  %56 = fptosi float %55 to i32
  %57 = insertelement <2 x i32> undef, i32 %56, i32 0
  %58 = extractelement <2 x float> %54, i32 1
  %59 = fptosi float %58 to i32
  %60 = insertelement <2 x i32> %57, i32 %59, i32 1
  %61 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %33, <2 x i32> %60, i32 0) #1
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit3

; <label>:62                                      ; preds = %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit
  %63 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %33, i32 0, <2 x float> %coord, float 0.000000e+00) #1
  %64 = bitcast <4 x float> %63 to <4 x i32>
  br label %_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit3

_Z11read_imagei11ocl_image2d11ocl_samplerDv2_f.exit3: ; preds = %48, %62
  %.0.i.i2 = phi <4 x i32> [ %61, %48 ], [ %64, %62 ]
  %add = add <4 x i32> %.0.i.i, %.0.i.i2
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %dst, i32 %0
  store <4 x i32> %add, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII2_CoordX1:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII2_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII2_CoordY1:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII2_Coord1]], i32 1
  ; CHECK: [[RII2_GetBufferPtr1:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RII2_CoordX1]], i32 [[RII2_CoordY1]], i32 0, i32 0, float addrspace(196608)* [[RII2_GetBufferPtr1]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RII2_CoordX2:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII2_Coord2:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII2_CoordY2:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII2_Coord2]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524288i32(float 0.000000e+00, float [[RII2_CoordX2]], float [[RII2_CoordY2]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524288)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RII2_CoordX2]], float [[RII2_CoordY2]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII2_CoordX3:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII2_Coord3:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII2_CoordY3:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII2_Coord3]], i32 1
  ; CHECK: [[RII2_GetBufferPtr3:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RII2_CoordX3]], i32 [[RII2_CoordY3]], i32 0, i32 0, float addrspace(196608)* [[RII2_GetBufferPtr3]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RII2_CoordX4:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII2_Coord4:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII2_CoordY4:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RII2_Coord4]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524289i32(float 0.000000e+00, float [[RII2_CoordX4]], float [[RII2_CoordY4]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524289)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RII2_CoordX4]], float [[RII2_CoordY4]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imagei_3(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %2 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %1, <2 x i32> %coord, i32 0) #1
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %dst, i32 %0
  store <4 x i32> %2, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RII3_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII3_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RII3_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RII3_Coord]], i32 1
  ; CHECK: [[RII3_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RII3_CoordX]], i32 [[RII3_CoordY]], i32 0, i32 0, float addrspace(196608)* [[RII3_GetBufferPtr]], i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imageui_1(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %smpAddress, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %2 = and i32 %smpAddress, 7
  %3 = icmp eq i32 %2, 2
  br i1 %3, label %4, label %13

; <label>:4                                       ; preds = %entry
  %5 = extractelement <2 x i32> %coord, i32 0
  %6 = sitofp i32 %5 to float
  %7 = insertelement <2 x float> undef, float %6, i32 0
  %8 = extractelement <2 x i32> %coord, i32 1
  %9 = sitofp i32 %8 to float
  %10 = insertelement <2 x float> %7, float %9, i32 1
  %11 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %1, i32 %sampler, <2 x float> %10, float 0.000000e+00) #1
  %12 = bitcast <4 x float> %11 to <4 x i32>
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit

; <label>:13                                      ; preds = %entry
  %14 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %1, <2 x i32> %coord, i32 0) #1
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit

_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit: ; preds = %4, %13
  %.0.i = phi <4 x i32> [ %12, %4 ], [ %14, %13 ]
  %15 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %16 = and i32 0, 7
  %17 = icmp eq i32 %16, 2
  br i1 %17, label %18, label %27

; <label>:18                                      ; preds = %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit
  %19 = extractelement <2 x i32> %coord, i32 0
  %20 = sitofp i32 %19 to float
  %21 = insertelement <2 x float> undef, float %20, i32 0
  %22 = extractelement <2 x i32> %coord, i32 1
  %23 = sitofp i32 %22 to float
  %24 = insertelement <2 x float> %21, float %23, i32 1
  %25 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %15, i32 0, <2 x float> %24, float 0.000000e+00) #1
  %26 = bitcast <4 x float> %25 to <4 x i32>
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit2

; <label>:27                                      ; preds = %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit
  %28 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %15, <2 x i32> %coord, i32 0) #1
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit2

_Z12read_imageui11ocl_image2d11ocl_samplerDv2_i.exit2: ; preds = %18, %27
  %.0.i1 = phi <4 x i32> [ %26, %18 ], [ %28, %27 ]
  %add = add <4 x i32> %.0.i, %.0.i1
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %dst, i32 %0
  store <4 x i32> %add, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIUI1_CoordX1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI1_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI1_CoordY1:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI1_Coord1]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524288i32(float 0.000000e+00, float [[RIUI1_CoordX1]], float [[RIUI1_CoordY1]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524288)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIUI1_CoordX1]], float [[RIUI1_CoordY1]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI1_CoordX2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI1_Coord2:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI1_CoordY2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI1_Coord2]], i32 1
  ; CHECK: [[RIUI1_GetBufferPtr2:%[a-zA-Z0-9]+]]  = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RIUI1_CoordX2]], i32 [[RIUI1_CoordY2]], i32 0, i32 0, float addrspace(196608)* [[RIUI1_GetBufferPtr2]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIUI1_CoordX3:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI1_Coord3:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI1_CoordY3:%[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI1_Coord3]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524289i32(float 0.000000e+00, float [[RIUI1_CoordX3]], float [[RIUI1_CoordY3]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524289)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIUI1_CoordX3]], float [[RIUI1_CoordY3]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI1_CoordX4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI1_Coord4:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI1_CoordY4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI1_Coord4]], i32 1
  ; CHECK: [[RIUI1_GetBufferPtr4:%[a-zA-Z0-9]+]]  = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RIUI1_CoordX4]], i32 [[RIUI1_CoordY4]], i32 0, i32 0, float addrspace(196608)* [[RIUI1_GetBufferPtr4]], i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imageui_2(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, i32 %sampler, <2 x float> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %imageHeigt, i32 %imageWidth, i32 %smpAddress, i32 %smpNormalized, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %2 = and i32 %smpAddress, 7
  %3 = icmp eq i32 %2, 1
  br i1 %3, label %4, label %30

; <label>:4                                       ; preds = %entry
  %5 = icmp eq i32 %smpNormalized, 8
  br i1 %5, label %6, label %16

; <label>:6                                       ; preds = %4
  %7 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %8 = insertelement <2 x i32> %7, i32 %imageHeigt, i32 1
  %9 = extractelement <2 x i32> %8, i32 0
  %10 = sitofp i32 %9 to float
  %11 = insertelement <2 x float> undef, float %10, i32 0
  %12 = extractelement <2 x i32> %8, i32 1
  %13 = sitofp i32 %12 to float
  %14 = insertelement <2 x float> %11, float %13, i32 1
  %15 = fmul <2 x float> %14, %coord
  br label %16

; <label>:16                                      ; preds = %6, %4
  %.01.i = phi <2 x float> [ %15, %6 ], [ %coord, %4 ]
  %17 = extractelement <2 x float> %.01.i, i32 0
  %18 = call float @__builtin_IB_frnd_ni(float %17) #1
  %19 = insertelement <2 x float> undef, float %18, i32 0
  %20 = extractelement <2 x float> %.01.i, i32 1
  %21 = call float @__builtin_IB_frnd_ni(float %20) #1
  %22 = insertelement <2 x float> %19, float %21, i32 1
  %23 = extractelement <2 x float> %22, i32 0
  %24 = fptosi float %23 to i32
  %25 = insertelement <2 x i32> undef, i32 %24, i32 0
  %26 = extractelement <2 x float> %22, i32 1
  %27 = fptosi float %26 to i32
  %28 = insertelement <2 x i32> %25, i32 %27, i32 1
  %29 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %1, <2 x i32> %28, i32 0) #1
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit

; <label>:30                                      ; preds = %entry
  %31 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %1, i32 %sampler, <2 x float> %coord, float 0.000000e+00) #1
  %32 = bitcast <4 x float> %31 to <4 x i32>
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit

_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit: ; preds = %16, %30
  %.0.i = phi <4 x i32> [ %29, %16 ], [ %32, %30 ]
  %33 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %34 = and i32 0, 7
  %35 = icmp eq i32 %34, 1
  br i1 %35, label %36, label %62

; <label>:36                                      ; preds = %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit
  %37 = icmp eq i32 0, 8
  br i1 %37, label %38, label %48

; <label>:38                                      ; preds = %36
  %39 = insertelement <2 x i32> undef, i32 %imageWidth, i32 0
  %40 = insertelement <2 x i32> %39, i32 %imageHeigt, i32 1
  %41 = extractelement <2 x i32> %40, i32 0
  %42 = sitofp i32 %41 to float
  %43 = insertelement <2 x float> undef, float %42, i32 0
  %44 = extractelement <2 x i32> %40, i32 1
  %45 = sitofp i32 %44 to float
  %46 = insertelement <2 x float> %43, float %45, i32 1
  %47 = fmul <2 x float> %46, %coord
  br label %48

; <label>:48                                      ; preds = %38, %36
  %.01.i1 = phi <2 x float> [ %47, %38 ], [ %coord, %36 ]
  %49 = extractelement <2 x float> %.01.i1, i32 0
  %50 = call float @__builtin_IB_frnd_ni(float %49) #1
  %51 = insertelement <2 x float> undef, float %50, i32 0
  %52 = extractelement <2 x float> %.01.i1, i32 1
  %53 = call float @__builtin_IB_frnd_ni(float %52) #1
  %54 = insertelement <2 x float> %51, float %53, i32 1
  %55 = extractelement <2 x float> %54, i32 0
  %56 = fptosi float %55 to i32
  %57 = insertelement <2 x i32> undef, i32 %56, i32 0
  %58 = extractelement <2 x float> %54, i32 1
  %59 = fptosi float %58 to i32
  %60 = insertelement <2 x i32> %57, i32 %59, i32 1
  %61 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %33, <2 x i32> %60, i32 0) #1
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit3

; <label>:62                                      ; preds = %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit
  %63 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %33, i32 0, <2 x float> %coord, float 0.000000e+00) #1
  %64 = bitcast <4 x float> %63 to <4 x i32>
  br label %_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit3

_Z12read_imageui11ocl_image2d11ocl_samplerDv2_f.exit3: ; preds = %48, %62
  %.0.i2 = phi <4 x i32> [ %61, %48 ], [ %64, %62 ]
  %add = add <4 x i32> %.0.i, %.0.i2
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %dst, i32 %0
  store <4 x i32> %add, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI2_CoordX1:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI2_Coord1:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI2_CoordY1:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI2_Coord1]], i32 1
  ; CHECK: [[RIUI2_GetBufferPtr1:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RIUI2_CoordX1]], i32 [[RIUI2_CoordY1]], i32 0, i32 0, float addrspace(196608)* [[RIUI2_GetBufferPtr1]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIUI2_CoordX2:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI2_Coord2:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI2_CoordY2:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI2_Coord2]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524288i32(float 0.000000e+00, float [[RIUI2_CoordX2]], float [[RIUI2_CoordY2]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524288)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIUI2_CoordX2]], float [[RIUI2_CoordY2]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI2_CoordX3:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI2_Coord3:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI2_CoordY3:%Coord[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI2_Coord3]], i32 1
  ; CHECK: [[RIUI2_GetBufferPtr3:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RIUI2_CoordX3]], i32 [[RIUI2_CoordY3]], i32 0, i32 0, float addrspace(196608)* [[RIUI2_GetBufferPtr3]], i32 0, i32 0, i32 0)

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
  ; CHECK: [[RIUI2_CoordX4:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI2_Coord4:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI2_CoordY4:%Coord[a-zA-Z0-9]+]] = extractelement <2 x float> [[RIUI2_Coord4]], i32 1
  ; CHECK: [[BuffPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x float> @genx.GenISA.sampleLptr.v4f32.f32.p196608f32.p524289i32(float 0.000000e+00, float [[RIUI2_CoordX4]], float [[RIUI2_CoordY4]], float 0.000000e+00, float 0.000000e+00, float addrspace(196608)* [[BuffPtr]], i32 addrspace(524289)* null, i32 0, i32 0, i32 0)
  ; C HECK: call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[RIUI2_CoordX4]], float [[RIUI2_CoordY4]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 1, i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_read_imageui_3(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %1 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  %2 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %1, <2 x i32> %coord, i32 0) #1
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %dst, i32 %0
  store <4 x i32> %2, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void

  ; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_ldui
  ; CHECK: [[RIUI3_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI3_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[RIUI3_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[RIUI3_Coord]], i32 1
  ; CHECK: [[RIUI3_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
  ; CHECK: call <4 x i32> @genx.GenISA.ldptr.v4i32.p196608f32(i32 [[RIUI3_CoordX]], i32 [[RIUI3_CoordY]], i32 0, i32 0, float addrspace(196608)* [[RIUI3_GetBufferPtr]], i32 0, i32 0, i32 0)

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_write_imagef_1(<4 x float> addrspace(1)* %src, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %src, i32 %0
  %1 = load <4 x float>, <4 x float> addrspace(1)* %arrayidx, align 16
  %2 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  call void @__builtin_IB_write_2d_f(i32 %2, <2 x i32> %coord, <4 x float> %1, i32 0) #1
  ret void

  ; CHECK-NOT: call void @__builtin_IB_write_2d_f
  ; CHECK: [[WIF1_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(131074)* @genx.GenISA.GetBufferPtr.p131074f32(i32 2, i32 1)
  ; CHECK: [[WIF1_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32>  [[WIF1_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[WIF1_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[WIF1_Coord]], i32 1
  ; CHECK: [[WIF1_ColorX:%[a-zA-Z0-9]+]] = extractelement <4 x float> %1, i32 0
  ; CHECK: [[WIF1_ColorY:%[a-zA-Z0-9]+]] = extractelement <4 x float> %1, i32 1
  ; CHECK: [[WIF1_ColorZ:%[a-zA-Z0-9]+]] = extractelement <4 x float> %1, i32 2
  ; CHECK: [[WIF1_ColorW:%[a-zA-Z0-9]+]] = extractelement <4 x float> %1, i32 3
  ; CHECK: call void @genx.GenISA.typedwrite.p131074f32(float addrspace(131074)* [[WIF1_GetBufferPtr]], i32 [[WIF1_CoordX]], i32 [[WIF1_CoordY]], i32 0, i32 0, float [[WIF1_ColorX]], float [[WIF1_ColorY]], float [[WIF1_ColorZ]], float [[WIF1_ColorW]])

  ; CHECK: ret void
}

; Function Attrs: alwaysinline nounwind
define void @test_write_imageui_1(<4 x i32> addrspace(1)* %src, %opencl.image2d_t addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) #0 {
entry:
  %0 = zext i16 %localIdX to i32
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %src, i32 %0
  %1 = load <4 x i32>, <4 x i32> addrspace(1)* %arrayidx, align 16
  %2 = ptrtoint %opencl.image2d_t addrspace(1)* %img to i32
  call void @__builtin_IB_write_2d_ui(i32 %2, <2 x i32> %coord, <4 x i32> %1, i32 0) #1
  ret void

  ; CHECK-NOT: call void @__builtin_IB_write_2d_ui
  ; CHECK: [[WIUI1_GetBufferPtr:%[a-zA-Z0-9]+]] = call float addrspace(131074)* @genx.GenISA.GetBufferPtr.p131074f32(i32 2, i32 1)
  ; CHECK: [[WIUI1_CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x i32>  [[WIUI1_Coord:%[a-zA-Z0-9]+]], i32 0
  ; CHECK: [[WIUI1_CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[WIUI1_Coord]], i32 1
  ; CHECK: [[WIUI1_floatColor:%[a-zA-Z0-9]+]] = bitcast <4 x i32> %1 to <4 x float>
  ; CHECK: [[WIUI1_ColorX:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WIUI1_floatColor]], i32 0
  ; CHECK: [[WIUI1_ColorY:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WIUI1_floatColor]], i32 1
  ; CHECK: [[WIUI1_ColorZ:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WIUI1_floatColor]], i32 2
  ; CHECK: [[WIUI1_ColorW:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[WIUI1_floatColor]], i32 3
  ; CHECK: call void @genx.GenISA.typedwrite.p131074f32(float addrspace(131074)* [[WIUI1_GetBufferPtr]], i32 [[WIUI1_CoordX]], i32 [[WIUI1_CoordY]], i32 0, i32 0, float [[WIUI1_ColorX]], float [[WIUI1_ColorY]], float [[WIUI1_ColorZ]], float [[WIUI1_ColorW]])

  ; CHECK: ret void
}

declare void @__builtin_IB_write_2d_ui(i32, <2 x i32>, <4 x i32>, i32)

declare void @__builtin_IB_write_2d_i(i32, <2 x i32>, <4 x i32>, i32)

declare void @__builtin_IB_write_2d_f(i32, <2 x i32>, <4 x float>, i32)

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
!igc.functions = !{!2, !27, !42, !53, !57, !70, !72, !81, !84, !90, !94, !98, !105, !111, !117, !123, !125, !129}
!igc.compiler.options = !{!133, !134, !135}
!igc.inline.constants = !{}
!igc.inline.globals = !{}
!igc.inline.programscope.offsets = !{}
!igc.global.pointer.info = !{}
!igc.constant.pointer.info = !{}
!igc.enable.FP_CONTRACT = !{}

!0 = !{i32 1, i32 0}
!1 = !{!"ocl", i32 1, i32 2}
!2 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_read_imagei_3, !3}
!3 = !{!4, !5, !12, !21, !22, !23, !24, !25, !26}
!4 = !{!"function_type", i32 0}
!5 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !11}
!6 = !{i32 0}
!7 = !{i32 1}
!8 = !{i32 5}
!9 = !{i32 6}
!10 = !{i32 7}
!11 = !{i32 10}
!12 = !{!"resource_alloc", !13, !14, !15, !16}
!13 = !{!"uavs_num", i32 2}
!14 = !{!"srvs_num", i32 1}
!15 = !{!"samplers_num", i32 0}
!16 = !{!"arg_allocs", !17, !18, !19, !19, !19, !19, !19, !19, !20}
!17 = !{i32 1, null, i32 0}
!18 = !{i32 2, i32 0, i32 0}
!19 = !{i32 0, null, null}
!20 = !{i32 1, null, i32 1}
!21 = !{!"opencl_kernel_arg_addr_space", i32 1, i32 1, i32 0}
!22 = !{!"opencl_kernel_arg_access_qual", !"none", !"read_only", !"none"}
!23 = !{!"opencl_kernel_arg_type", !"int4*", !"image2d_t", !"int2"}
!24 = !{!"opencl_kernel_arg_base_type", !"int4*", !"image2d_t", !"int2"}
!25 = !{!"opencl_kernel_arg_type_qual", !"", !"", !""}
!26 = !{!"opencl_kernel_arg_name", !"dst", !"img", !"coord"}
!27 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_read_imageui_1, !28}
!28 = !{!4, !29, !32, !36, !37, !38, !39, !40, !41}
!29 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !30, !11}
!30 = !{i32 24, !31}
!31 = !{!"explicit_arg_num", i32 2}
!32 = !{!"resource_alloc", !13, !14, !33, !34}
!33 = !{!"samplers_num", i32 1}
!34 = !{!"arg_allocs", !17, !18, !35, !19, !19, !19, !19, !19, !19, !19, !20}
!35 = !{i32 3, i32 0, i32 0}
!36 = !{!"opencl_kernel_arg_addr_space", i32 1, i32 1, i32 0, i32 0}
!37 = !{!"opencl_kernel_arg_access_qual", !"none", !"read_only", !"none", !"none"}
!38 = !{!"opencl_kernel_arg_type", !"uint4*", !"image2d_t", !"sampler_t", !"int2"}
!39 = !{!"opencl_kernel_arg_base_type", !"uint4*", !"image2d_t", !"sampler_t", !"int2"}
!40 = !{!"opencl_kernel_arg_type_qual", !"", !"", !"", !""}
!41 = !{!"opencl_kernel_arg_name", !"dst", !"img", !"sampler", !"coord"}
!42 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x float>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i32, i32, i32, i8*)* @test_read_imageui_2, !43}
!43 = !{!4, !44, !49, !36, !37, !51, !52, !40, !41}
!44 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !45, !47, !30, !48, !11}
!45 = !{i32 17, !46}
!46 = !{!"explicit_arg_num", i32 1}
!47 = !{i32 18, !46}
!48 = !{i32 25, !31}
!49 = !{!"resource_alloc", !13, !14, !33, !50}
!50 = !{!"arg_allocs", !17, !18, !35, !19, !19, !19, !19, !19, !19, !19, !19, !19, !19, !20}
!51 = !{!"opencl_kernel_arg_type", !"uint4*", !"image2d_t", !"sampler_t", !"float2"}
!52 = !{!"opencl_kernel_arg_base_type", !"uint4*", !"image2d_t", !"sampler_t", !"float2"}
!53 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_read_imageui_3, !54}
!54 = !{!4, !5, !12, !21, !22, !55, !56, !25, !26}
!55 = !{!"opencl_kernel_arg_type", !"uint4*", !"image2d_t", !"int2"}
!56 = !{!"opencl_kernel_arg_base_type", !"uint4*", !"image2d_t", !"int2"}
!57 = !{void (<4 x float> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_write_imagef_1, !58}
!58 = !{!4, !5, !59, !21, !64, !65, !66, !25, !67}
!59 = !{!"resource_alloc", !60, !61, !15, !62}
!60 = !{!"uavs_num", i32 3}
!61 = !{!"srvs_num", i32 0}
!62 = !{!"arg_allocs", !17, !63, !19, !19, !19, !19, !19, !19, !20}
!63 = !{i32 1, i32 0, i32 2}
!64 = !{!"opencl_kernel_arg_access_qual", !"none", !"write_only", !"none"}
!65 = !{!"opencl_kernel_arg_type", !"float4*", !"image2d_t", !"int2"}
!66 = !{!"opencl_kernel_arg_base_type", !"float4*", !"image2d_t", !"int2"}
!67 = !{!"opencl_kernel_arg_name", !"src", !"img", !"coord"}
!69 = !{!4, !5, !59, !21, !64, !23, !24, !25, !67}
!70 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_write_imageui_1, !71}
!71 = !{!4, !5, !59, !21, !64, !55, !56, !25, !67}
!72 = !{void (i32 addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_width, !73}
!73 = !{!4, !74, !12, !75, !76, !77, !78, !79, !80}
!74 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !47, !11}
!75 = !{!"opencl_kernel_arg_addr_space", i32 1, i32 1}
!76 = !{!"opencl_kernel_arg_access_qual", !"none", !"read_only"}
!77 = !{!"opencl_kernel_arg_type", !"int*", !"image2d_t"}
!78 = !{!"opencl_kernel_arg_base_type", !"int*", !"image2d_t"}
!79 = !{!"opencl_kernel_arg_type_qual", !"", !""}
!80 = !{!"opencl_kernel_arg_name", !"dst", !"img"}
!81 = !{void (i32 addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_height, !82}
!82 = !{!4, !83, !12, !75, !76, !77, !78, !79, !80}
!83 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !45, !11}
!84 = !{void (i32 addrspace(1)*, %opencl.image3d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_depth, !85}
!85 = !{!4, !86, !12, !75, !76, !88, !89, !79, !80}
!86 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !87, !11}
!87 = !{i32 19, !46}
!88 = !{!"opencl_kernel_arg_type", !"int*", !"image3d_t"}
!89 = !{!"opencl_kernel_arg_base_type", !"int*", !"image3d_t"}
!90 = !{void (i32 addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_channel_data_type, !91}
!91 = !{!4, !92, !12, !75, !76, !77, !78, !79, !80}
!92 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !93, !11}
!93 = !{i32 21, !46}
!94 = !{void (i32 addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_channel_order, !95}
!95 = !{!4, !96, !12, !75, !76, !77, !78, !79, !80}
!96 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !97, !11}
!97 = !{i32 22, !46}
!98 = !{void (<2 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i32, i8*)* @test_image_dim, !99}
!99 = !{!4, !100, !101, !75, !76, !103, !104, !79, !80}
!100 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !45, !47, !11}
!101 = !{!"resource_alloc", !13, !14, !15, !102}
!102 = !{!"arg_allocs", !17, !18, !19, !19, !19, !19, !19, !19, !19, !20}
!103 = !{!"opencl_kernel_arg_type", !"int2*", !"image2d_t"}
!104 = !{!"opencl_kernel_arg_base_type", !"int2*", !"image2d_t"}
!105 = !{void (i32 addrspace(1)*, %opencl.image2d_array_t addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_image_array_size, !106}
!106 = !{!4, !107, !12, !75, !76, !109, !110, !79, !80}
!107 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !108, !11}
!108 = !{i32 23, !46}
!109 = !{!"opencl_kernel_arg_type", !"size_t*", !"image2d_array_t"}
!110 = !{!"opencl_kernel_arg_base_type", !"uint*", !"image2d_array_t"}
!111 = !{void (<4 x float> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_read_imagef_1, !112}
!112 = !{!4, !5, !113, !36, !37, !115, !116, !40, !41}
!113 = !{!"resource_alloc", !13, !14, !33, !114}
!114 = !{!"arg_allocs", !17, !18, !35, !19, !19, !19, !19, !19, !19, !20}
!115 = !{!"opencl_kernel_arg_type", !"float4*", !"image2d_t", !"sampler_t", !"int2"}
!116 = !{!"opencl_kernel_arg_base_type", !"float4*", !"image2d_t", !"sampler_t", !"int2"}
!117 = !{void (<4 x float> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x float>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_read_imagef_2, !118}
!118 = !{!4, !119, !32, !36, !37, !121, !122, !40, !41}
!119 = !{!"implicit_arg_desc", !6, !7, !8, !9, !10, !120, !11}
!120 = !{i32 26, !31}
!121 = !{!"opencl_kernel_arg_type", !"float4*", !"image2d_t", !"sampler_t", !"float2"}
!122 = !{!"opencl_kernel_arg_base_type", !"float4*", !"image2d_t", !"sampler_t", !"float2"}
!123 = !{void (<4 x float> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i8*)* @test_read_imagef_3, !124}
!124 = !{!4, !5, !12, !21, !22, !65, !66, !25, !26}
!125 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x i32>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i8*)* @test_read_imagei_1, !126}
!126 = !{!4, !29, !32, !36, !37, !127, !128, !40, !41}
!127 = !{!"opencl_kernel_arg_type", !"int4*", !"image2d_t", !"sampler_t", !"int2"}
!128 = !{!"opencl_kernel_arg_base_type", !"int4*", !"image2d_t", !"sampler_t", !"int2"}
!129 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x float>, <8 x i32>, <8 x i32>, i16, i16, i16, i32, i32, i32, i32, i8*)* @test_read_imagei_2, !130}
!130 = !{!4, !44, !49, !36, !37, !131, !132, !40, !41}
!131 = !{!"opencl_kernel_arg_type", !"int4*", !"image2d_t", !"sampler_t", !"float2"}
!132 = !{!"opencl_kernel_arg_base_type", !"int4*", !"image2d_t", !"sampler_t", !"float2"}
!133 = !{!"-opt-disable"}
!134 = !{!"-std=CL1.2"}
!135 = !{!"-kernel-arg-info"}


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
