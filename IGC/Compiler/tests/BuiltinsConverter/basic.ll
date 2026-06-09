;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-conv-ocl-to-common -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; BuiltinsConverter
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

%opencl.image2d_t.read_only = type opaque
%opencl.image2d_t.write_only = type opaque

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_read_imagef(<4 x float> addrspace(1)* %dst, %opencl.image2d_t.read_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test_read_imagef(
; CHECK:  entry:
; CHECK:    [[COORD_ADDR:%.*]] = alloca <2 x i32>, align 8
; CHECK:    [[TMP1:%.*]] = load <2 x i32>, <2 x i32>* [[COORD_ADDR]], align 8
; CHECK:    [[COORDX:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[COORDY:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[CALL21_I1:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
; CHECK:    [[CALL21_I2:%.*]] = call <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p196608f32.p196608f32(i32 [[COORDX]], i32 [[COORDY]], i32 0, i32 0, float addrspace(196608)* undef, float addrspace(196608)* [[CALL21_I1]], i32 0, i32 0, i32 0)
; CHECK:    [[ASTYPE_I54_I:%.*]] = bitcast <4 x float> [[CALL21_I2]] to <4 x i32>
;
entry:
  %dst.addr = alloca <4 x float> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x float> addrspace(1)* %dst, <4 x float> addrspace(1)** %dst.addr, align 8
  store %opencl.image2d_t.read_only addrspace(1)* %img, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = load %opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8
  %ImageArgVal = ptrtoint %opencl.image2d_t.read_only addrspace(1)* %0 to i64
  %2 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i = trunc i64 %ImageArgVal to i32
  %call21.i = call spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32 %conv.i, <2 x i32> %1, i32 0) #4
  %astype.i54.i = bitcast <4 x float> %call21.i to <4 x i32>
  %and.i55.i = and <4 x i32> %astype.i54.i, <i32 -2139095041, i32 -2139095041, i32 -2139095041, i32 -2139095041>
  %cmp.i56.i = icmp eq <4 x i32> %and.i55.i, %astype.i54.i
  %and5.i57.i = and <4 x i32> %astype.i54.i, <i32 -2147483648, i32 -2147483648, i32 -2147483648, i32 -2147483648>
  %3 = select <4 x i1> %cmp.i56.i, <4 x i32> %and5.i57.i, <4 x i32> %astype.i54.i
  %4 = bitcast <4 x i32> %3 to <4 x float>
  %5 = load <4 x float> addrspace(1)*, <4 x float> addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %5, i64 0
  store <4 x float> %4, <4 x float> addrspace(1)* %arrayidx, align 16
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_read_imagei(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t.read_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test_read_imagei(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca <4 x i32> addrspace(1)*, align 8
; CHECK:    [[COORD_ADDR:%.*]] = alloca <2 x i32>, align 8
; CHECK:    [[TMP1:%.*]] = load <2 x i32>, <2 x i32>* [[COORD_ADDR]], align 8
; CHECK:    [[COORDX:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[COORDY:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[CALL15_I1:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
; CHECK:    [[CALL15_I2:%.*]] = call <4 x i32> @llvm.genx.GenISA.ldptr.v4i32.p196608f32.p196608f32(i32 [[COORDX]], i32 [[COORDY]], i32 0, i32 0, float addrspace(196608)* undef, float addrspace(196608)* [[CALL15_I1]], i32 0, i32 0, i32 0)
; CHECK:    [[TMP3:%.*]] = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* [[TMP3]], i64 0
; CHECK:    store <4 x i32> [[CALL15_I2]], <4 x i32> addrspace(1)* [[ARRAYIDX]], align 16
;
entry:
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 8
  store %opencl.image2d_t.read_only addrspace(1)* %img, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = load %opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8
  %ImageArgVal = ptrtoint %opencl.image2d_t.read_only addrspace(1)* %0 to i64
  %2 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i = trunc i64 %ImageArgVal to i32
  %call15.i = call spir_func <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %conv.i, <2 x i32> %1, i32 0) #4
  %3 = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %3, i64 0
  store <4 x i32> %call15.i, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_read_imageui(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t.read_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test_read_imageui(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca <4 x i32> addrspace(1)*, align 8
; CHECK:    [[COORD_ADDR:%.*]] = alloca <2 x i32>, align 8
; CHECK:    [[TMP1:%.*]] = load <2 x i32>, <2 x i32>* [[COORD_ADDR]], align 8
; CHECK:    [[COORDX:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[COORDY:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[CALL15_I1:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
; CHECK:    [[CALL15_I2:%.*]] = call <4 x i32> @llvm.genx.GenISA.ldptr.v4i32.p196608f32.p196608f32(i32 [[COORDX]], i32 [[COORDY]], i32 0, i32 0, float addrspace(196608)* undef, float addrspace(196608)* [[CALL15_I1]], i32 0, i32 0, i32 0)
; CHECK:    [[TMP3:%.*]] = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* [[TMP3]], i64 0
; CHECK:    store <4 x i32> [[CALL15_I2]], <4 x i32> addrspace(1)* [[ARRAYIDX]], align 16
; CHECK:    ret void
;
entry:
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 8
  store %opencl.image2d_t.read_only addrspace(1)* %img, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = load %opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8
  %ImageArgVal = ptrtoint %opencl.image2d_t.read_only addrspace(1)* %0 to i64
  %2 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i = trunc i64 %ImageArgVal to i32
  %call15.i = call spir_func <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %conv.i, <2 x i32> %1, i32 0) #4
  %3 = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %3, i64 0
  store <4 x i32> %call15.i, <4 x i32> addrspace(1)* %arrayidx, align 16
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_write_imagef(<4 x float> addrspace(1)* %src, %opencl.image2d_t.write_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test_write_imagef(
; CHECK-NEXT:  entry:
; CHECK:    [[SRC_ADDR:%.*]] = alloca <4 x float> addrspace(1)*, align 8
; CHECK:    [[COORD_ADDR:%.*]] = alloca <2 x i32>, align 8
; CHECK:    [[TMP1:%.*]] = load <2 x i32>, <2 x i32>* [[COORD_ADDR]], align 8
; CHECK:    [[TMP2:%.*]] = load <4 x float> addrspace(1)*, <4 x float> addrspace(1)** [[SRC_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* [[TMP2]], i64 0
; CHECK:    [[TMP3:%.*]] = load <4 x float>, <4 x float> addrspace(1)* [[ARRAYIDX]], align 16
; CHECK:    [[ASTYPE_I:%.*]] = bitcast <4 x float> [[TMP3]] to <4 x i32>
; CHECK:    [[TMP5:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
; CHECK:    [[COORDX:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[COORDY:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[FLOATCOLOR:%.*]] = bitcast <4 x i32> [[ASTYPE_I]] to <4 x float>
; CHECK:    [[COLORX:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 0
; CHECK:    [[COLORY:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 1
; CHECK:    [[COLORZ:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 2
; CHECK:    [[COLORW:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 3
; CHECK:    call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[TMP5]], i32 [[COORDX]], i32 [[COORDY]], i32 0, i32 0, float [[COLORX]], float [[COLORY]], float [[COLORZ]], float [[COLORW]])
; CHECK:    ret void
;
entry:
  %src.addr = alloca <4 x float> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.write_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x float> addrspace(1)* %src, <4 x float> addrspace(1)** %src.addr, align 8
  store %opencl.image2d_t.write_only addrspace(1)* %img, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = load %opencl.image2d_t.write_only addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8
  %2 = load <4 x float> addrspace(1)*, <4 x float> addrspace(1)** %src.addr, align 8
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %2, i64 0
  %3 = load <4 x float>, <4 x float> addrspace(1)* %arrayidx, align 16
  %ImageArgVal = ptrtoint %opencl.image2d_t.write_only addrspace(1)* %0 to i64
  %4 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %astype.i = bitcast <4 x float> %3 to <4 x i32>
  %conv.i.i.i = trunc i64 %ImageArgVal to i32
  call spir_func void @__builtin_IB_write_2d_u4i(i32 %conv.i.i.i, <2 x i32> %1, <4 x i32> %astype.i, i32 0) #4
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_write_imagei(<4 x i32> addrspace(1)* %src, %opencl.image2d_t.write_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test_write_imagei(
; CHECK:  entry:
; CHECK:    [[SRC_ADDR:%.*]] = alloca <4 x i32> addrspace(1)*, align 8
; CHECK:    [[COORD_ADDR:%.*]] = alloca <2 x i32>, align 8
; CHECK:    [[TMP1:%.*]] = load <2 x i32>, <2 x i32>* [[COORD_ADDR]], align 8
; CHECK:    [[TMP2:%.*]] = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** [[SRC_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* [[TMP2]], i64 0
; CHECK:    [[TMP3:%.*]] = load <4 x i32>, <4 x i32> addrspace(1)* [[ARRAYIDX]], align 16
; CHECK:    [[TMP5:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
; CHECK:    [[COORDX:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[COORDY:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[FLOATCOLOR:%.*]] = bitcast <4 x i32> [[TMP3]] to <4 x float>
; CHECK:    [[COLORX:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 0
; CHECK:    [[COLORY:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 1
; CHECK:    [[COLORZ:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 2
; CHECK:    [[COLORW:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 3
; CHECK:    call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[TMP5]], i32 [[COORDX]], i32 [[COORDY]], i32 0, i32 0, float [[COLORX]], float [[COLORY]], float [[COLORZ]], float [[COLORW]])
; CHECK:    ret void
;
entry:
  %src.addr = alloca <4 x i32> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.write_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x i32> addrspace(1)* %src, <4 x i32> addrspace(1)** %src.addr, align 8
  store %opencl.image2d_t.write_only addrspace(1)* %img, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = load %opencl.image2d_t.write_only addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8
  %2 = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** %src.addr, align 8
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %2, i64 0
  %3 = load <4 x i32>, <4 x i32> addrspace(1)* %arrayidx, align 16
  %ImageArgVal = ptrtoint %opencl.image2d_t.write_only addrspace(1)* %0 to i64
  %4 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i.i = trunc i64 %ImageArgVal to i32
  call spir_func void @__builtin_IB_write_2d_u4i(i32 %conv.i.i, <2 x i32> %1, <4 x i32> %3, i32 0) #4
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_write_imageui(<4 x i32> addrspace(1)* %src, %opencl.image2d_t.write_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test_write_imageui(
; CHECK:  entry:
; CHECK:    [[SRC_ADDR:%.*]] = alloca <4 x i32> addrspace(1)*, align 8
; CHECK:    [[COORD_ADDR:%.*]] = alloca <2 x i32>, align 8
; CHECK:    [[TMP1:%.*]] = load <2 x i32>, <2 x i32>* [[COORD_ADDR]], align 8
; CHECK:    [[TMP2:%.*]] = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** [[SRC_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* [[TMP2]], i64 0
; CHECK:    [[TMP3:%.*]] = load <4 x i32>, <4 x i32> addrspace(1)* [[ARRAYIDX]], align 16
; CHECK:    [[TMP4:%.*]] = shufflevector <2 x i32> [[TMP1]], <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
; CHECK:    [[TMP5:%.*]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2)
; CHECK:    [[COORDX:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[COORDY:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[FLOATCOLOR:%.*]] = bitcast <4 x i32> [[TMP3]] to <4 x float>
; CHECK:    [[COLORX:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 0
; CHECK:    [[COLORY:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 1
; CHECK:    [[COLORZ:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 2
; CHECK:    [[COLORW:%.*]] = extractelement <4 x float> [[FLOATCOLOR]], i32 3
; CHECK:    call void @llvm.genx.GenISA.typedwrite.p196608f32(float addrspace(196608)* [[TMP5]], i32 [[COORDX]], i32 [[COORDY]], i32 0, i32 0, float [[COLORX]], float [[COLORY]], float [[COLORZ]], float [[COLORW]])
; CHECK:    ret void
;
entry:
  %src.addr = alloca <4 x i32> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.write_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x i32> addrspace(1)* %src, <4 x i32> addrspace(1)** %src.addr, align 8
  store %opencl.image2d_t.write_only addrspace(1)* %img, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = load %opencl.image2d_t.write_only addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8
  %2 = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** %src.addr, align 8
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %2, i64 0
  %3 = load <4 x i32>, <4 x i32> addrspace(1)* %arrayidx, align 16
  %ImageArgVal = ptrtoint %opencl.image2d_t.write_only addrspace(1)* %0 to i64
  %4 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i.i = trunc i64 %ImageArgVal to i32
  call spir_func void @__builtin_IB_write_2d_u4i(i32 %conv.i.i, <2 x i32> %1, <4 x i32> %3, i32 0) #4
  ret void
}

declare spir_func <4 x i32> @__builtin_IB_OCL_2d_ldui(i32, <2 x i32>, i32) local_unnamed_addr #2
declare spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32, <2 x i32>, i32) local_unnamed_addr #2
declare spir_func void @__builtin_IB_write_2d_u4i(i32, <2 x i32>, <4 x i32>, i32) local_unnamed_addr #2
declare void @llvm.genx.GenISA.CatchAllDebugLine() #3

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #2 = { convergent "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }
attributes #4 = { convergent nounwind }

!llvm.module.flags = !{!0, !1, !2}
!IGCMetadata = !{!3}
!igc.functions = !{!37, !46, !47, !48, !49, !50}
!opencl.ocl.version = !{!51, !51, !51, !51, !51}
!opencl.spir.version = !{!51, !51, !51, !51, !51}
!llvm.ident = !{!52, !52, !52, !52, !52}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !113, !114, !115, !116, !117, !118, !119, !120, !121, !122, !123, !124}
!5 = distinct !{!"FuncMDMap[0]", void (<4 x float> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagef}
!6 = !{!"FuncMDValue[0]", !7, !8, !9}
!7 = !{!"funcArgs"}
!8 = !{!"functionType", !"KernelFunction"}
!9 = !{!"resAllocMD", !10}
!10 = !{!"argAllocMDList", !11, !15, !18, !21, !22, !24, !25, !26}
!11 = !{!"argAllocMDListVec[0]", !12, !13, !14}
!12 = !{!"type", i32 1}
!13 = !{!"extensionType", i32 -1}
!14 = !{!"indexType", i32 0}
!15 = !{!"argAllocMDListVec[1]", !16, !17, !14}
!16 = !{!"type", i32 2}
!17 = !{!"extensionType", i32 0}
!18 = !{!"argAllocMDListVec[2]", !19, !13, !20}
!19 = !{!"type", i32 0}
!20 = !{!"indexType", i32 -1}
!21 = !{!"argAllocMDListVec[3]", !19, !13, !20}
!22 = !{!"argAllocMDListVec[4]", !12, !13, !23}
!23 = !{!"indexType", i32 1}
!24 = !{!"argAllocMDListVec[5]", !19, !13, !20}
!25 = !{!"argAllocMDListVec[6]", !19, !13, !20}
!26 = !{!"argAllocMDListVec[7]", !19, !13, !20}
!27 = distinct !{!"FuncMDMap[1]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagei}
!28 = !{!"FuncMDValue[1]", !7, !8, !9}
!29 = distinct !{!"FuncMDMap[2]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imageui}
!30 = !{!"FuncMDValue[2]", !7, !8, !9}
!31 = distinct !{!"FuncMDMap[3]", void (<4 x float> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagef}
!32 = !{!"FuncMDValue[3]", !7, !8, !9}
!33 = distinct !{!"FuncMDMap[4]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagei}
!34 = !{!"FuncMDValue[4]", !7, !8, !9}
!35 = distinct !{!"FuncMDMap[5]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imageui}
!36 = !{!"FuncMDValue[5]", !7, !8, !9}
!37 = !{void (<4 x float> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagef, !38}
!38 = !{!39}
!39 = !{!"function_type", i32 0}
!46 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagei, !38}
!47 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imageui, !38}
!48 = !{void (<4 x float> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagef, !38}
!49 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagei, !38}
!50 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imageui, !38}
!51 = !{i32 2, i32 0}
!52 = !{!"clang version 10.0.0"}
!53 = !{!"argId", i32 0}
!54 = !{!"implicitArgInfoListVec[0]", !53}
!55 = !{!"argId", i32 1}
!56 = !{!"implicitArgInfoListVec[1]", !55}
!57 = !{!"argId", i32 13}
!58 = !{!"implicitArgInfoListVec[2]", !57}
!59 = !{!"argId", i32 15}
!60 = !{!"explicitArgNum", i32 0}
!61 = !{!"implicitArgInfoListVec[3]", !59, !60}
!62 = !{!"implicitArgInfoList", !54, !56, !58, !61}
!63 = !{!"argId", i32 0}
!64 = !{!"implicitArgInfoListVec[0]", !63}
!65 = !{!"argId", i32 1}
!66 = !{!"implicitArgInfoListVec[1]", !65}
!67 = !{!"argId", i32 13}
!68 = !{!"implicitArgInfoListVec[2]", !67}
!69 = !{!"argId", i32 15}
!70 = !{!"explicitArgNum", i32 0}
!71 = !{!"implicitArgInfoListVec[3]", !69, !70}
!72 = !{!"implicitArgInfoList", !64, !66, !68, !71}
!73 = !{!"argId", i32 0}
!74 = !{!"implicitArgInfoListVec[0]", !73}
!75 = !{!"argId", i32 1}
!76 = !{!"implicitArgInfoListVec[1]", !75}
!77 = !{!"argId", i32 13}
!78 = !{!"implicitArgInfoListVec[2]", !77}
!79 = !{!"argId", i32 15}
!80 = !{!"explicitArgNum", i32 0}
!81 = !{!"implicitArgInfoListVec[3]", !79, !80}
!82 = !{!"implicitArgInfoList", !74, !76, !78, !81}
!83 = !{!"argId", i32 0}
!84 = !{!"implicitArgInfoListVec[0]", !83}
!85 = !{!"argId", i32 1}
!86 = !{!"implicitArgInfoListVec[1]", !85}
!87 = !{!"argId", i32 13}
!88 = !{!"implicitArgInfoListVec[2]", !87}
!89 = !{!"argId", i32 15}
!90 = !{!"explicitArgNum", i32 0}
!91 = !{!"implicitArgInfoListVec[3]", !89, !90}
!92 = !{!"implicitArgInfoList", !84, !86, !88, !91}
!93 = !{!"argId", i32 0}
!94 = !{!"implicitArgInfoListVec[0]", !93}
!95 = !{!"argId", i32 1}
!96 = !{!"implicitArgInfoListVec[1]", !95}
!97 = !{!"argId", i32 13}
!98 = !{!"implicitArgInfoListVec[2]", !97}
!99 = !{!"argId", i32 15}
!100 = !{!"explicitArgNum", i32 0}
!101 = !{!"implicitArgInfoListVec[3]", !99, !100}
!102 = !{!"implicitArgInfoList", !94, !96, !98, !101}
!103 = !{!"argId", i32 0}
!104 = !{!"implicitArgInfoListVec[0]", !103}
!105 = !{!"argId", i32 1}
!106 = !{!"implicitArgInfoListVec[1]", !105}
!107 = !{!"argId", i32 13}
!108 = !{!"implicitArgInfoListVec[2]", !107}
!109 = !{!"argId", i32 15}
!110 = !{!"explicitArgNum", i32 0}
!111 = !{!"implicitArgInfoListVec[3]", !109, !110}
!112 = !{!"implicitArgInfoList", !104, !106, !108, !111}
!113 = !{!"FuncMDMap[6]", void (<4 x float> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagef}
!114 = !{!"FuncMDValue[6]", !62}
!115 = !{!"FuncMDMap[7]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagei}
!116 = !{!"FuncMDValue[7]", !72}
!117 = !{!"FuncMDMap[8]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imageui}
!118 = !{!"FuncMDValue[8]", !82}
!119 = !{!"FuncMDMap[9]", void (<4 x float> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagef}
!120 = !{!"FuncMDValue[9]", !92}
!121 = !{!"FuncMDMap[10]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagei}
!122 = !{!"FuncMDValue[10]", !102}
!123 = !{!"FuncMDMap[11]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imageui}
!124 = !{!"FuncMDValue[11]", !112}
