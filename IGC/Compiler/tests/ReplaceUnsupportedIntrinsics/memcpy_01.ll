;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

target triple = "igil_32_GEN9"

%struct.data_t = type { [97 x <2 x i8>] }

; Function Attrs: nounwind
define void @test_kernel(%struct.data_t addrspace(1)* %p, %struct.data_t addrspace(1)* %q) #0 {
; CHECK: [[pIV:%[a-zA-Z0-9_]+]] = alloca i32
; CHECK: [[vSrc:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to <8 x i32> addrspace(1)*
; CHECK: [[vDst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to <8 x i32> addrspace(1)*
; CHECK: memcpy.body:
; CHECK:   [[IV:%[a-zA-Z0-9_]+]] = load i32, i32* [[pIV]]
; CHECK:   [[GEP0:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[vSrc]], i32 [[IV]]
; CHECK:   [[GEP1:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[vDst]], i32 [[IV]]
; CHECK:   [[LD:%[a-zA-Z0-9_]+]] = load <8 x i32>, <8 x i32> addrspace(1)* [[GEP0]], align 2
; CHECK:   store <8 x i32> [[LD]], <8 x i32> addrspace(1)* [[GEP1]], align 2
; CHECK:   [[INC0:%[a-zA-Z0-9_]+]] = add i32 [[IV]], 1
; CHECK:   store i32 [[INC0]], i32* [[pIV]]
; CHECK:   [[CMP:%[a-zA-Z0-9_]+]] = icmp ult i32 [[INC0]], 12
; CHECK:   br i1 [[CMP]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.post:
; CHECK:   [[memcpy_src:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to i8 addrspace(1)*
; CHECK:   [[memcpy_dst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to i8 addrspace(1)*
; CHECK:   [[GEP2:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 384
; CHECK:   [[GEP3:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 384
; CHECK:   [[PTR0:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP2]] to i32 addrspace(1)*
; CHECK:   [[PTR1:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP3]] to i32 addrspace(1)*
; CHECK:   [[L0:%[_a-zA-Z0-9_]+]] = load i32, i32 addrspace(1)* [[PTR0]], align 2
; CHECK:   store i32 [[L0]], i32 addrspace(1)* [[PTR1]], align 2
; CHECK:   ret void
  %Dst = bitcast %struct.data_t addrspace(1)* %p to i8 addrspace(1)*
  %Src = bitcast %struct.data_t addrspace(1)* %q to i8 addrspace(1)*
  call void @llvm.memcpy.p1i8.p1i8.i32(i8 addrspace(1)* %Dst, i8 addrspace(1)* %Src, i32 388, i32 2, i1 false)
  ret void
}

%struct.data_t_2 = type { [98 x <2 x i8>] }

; Function Attrs: nounwind
define void @test_kernel_unrolled(%struct.data_t_2 addrspace(1)* %p, %struct.data_t_2 addrspace(1)* %q) #0 {
; CHECK-LABEL: @test_kernel_unrolled(
; CHECK:    [[DST:%.*]] = bitcast [[STRUCT_DATA_T_2:%.*]] addrspace(1)* [[P:%.*]] to i8 addrspace(1)*
; CHECK:    [[SRC:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[Q:%.*]] to i8 addrspace(1)*
; CHECK:    [[MEMCPY_VSRC:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[Q]] to <8 x i32> addrspace(1)*
; CHECK:    [[MEMCPY_VDST:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[P]] to <8 x i32> addrspace(1)*

; CHECK:    [[TMP1:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 0
; CHECK:    [[TMP2:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 0
; CHECK:    [[TMP3:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP1]], align 2
; CHECK:    store <8 x i32> [[TMP3]], <8 x i32> addrspace(1)* [[TMP2]], align 2
; CHECK:    [[TMP4:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 1
; CHECK:    [[TMP5:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 1
; CHECK:    [[TMP6:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP4]], align 2
; CHECK:    store <8 x i32> [[TMP6]], <8 x i32> addrspace(1)* [[TMP5]], align 2
; CHECK:    [[TMP7:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 2
; CHECK:    [[TMP8:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 2
; CHECK:    [[TMP9:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP7]], align 2
; CHECK:    store <8 x i32> [[TMP9]], <8 x i32> addrspace(1)* [[TMP8]], align 2
; CHECK:    [[TMP10:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 3
; CHECK:    [[TMP11:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 3
; CHECK:    [[TMP12:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP10]], align 2
; CHECK:    store <8 x i32> [[TMP12]], <8 x i32> addrspace(1)* [[TMP11]], align 2
; CHECK:    [[TMP13:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 4
; CHECK:    [[TMP14:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 4
; CHECK:    [[TMP15:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP13]], align 2
; CHECK:    store <8 x i32> [[TMP15]], <8 x i32> addrspace(1)* [[TMP14]], align 2
; CHECK:    [[TMP16:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 5
; CHECK:    [[TMP17:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 5
; CHECK:    [[TMP18:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP16]], align 2
; CHECK:    store <8 x i32> [[TMP18]], <8 x i32> addrspace(1)* [[TMP17]], align 2

; CHECK:    [[MEMCPY_SRC:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[Q]] to i8 addrspace(1)*
; CHECK:    [[MEMCPY_DST:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[P]] to i8 addrspace(1)*
; CHECK:    [[TMP19:%.*]] = getelementptr i8, i8 addrspace(1)* [[MEMCPY_SRC]], i32 192
; CHECK:    [[TMP20:%.*]] = getelementptr i8, i8 addrspace(1)* [[MEMCPY_DST]], i32 192
; CHECK:    [[MEMCPY_REM:%.*]] = bitcast i8 addrspace(1)* [[TMP19]] to i32 addrspace(1)*
; CHECK:    [[MEMCPY_REM1:%.*]] = bitcast i8 addrspace(1)* [[TMP20]] to i32 addrspace(1)*
; CHECK:    [[TMP21:%.*]] = load i32, i32 addrspace(1)* [[MEMCPY_REM]], align 2
; CHECK:    store i32 [[TMP21]], i32 addrspace(1)* [[MEMCPY_REM1]], align 2
; CHECK:    ret void
;
  %Dst = bitcast %struct.data_t_2 addrspace(1)* %p to i8 addrspace(1)*
  %Src = bitcast %struct.data_t_2 addrspace(1)* %q to i8 addrspace(1)*
  call void @llvm.memcpy.p1i8.p1i8.i32(i8 addrspace(1)* %Dst, i8 addrspace(1)* %Src, i32 196, i32 2, i1 false)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p1i8.p1i8.i32(i8 addrspace(1)* nocapture, i8 addrspace(1)* nocapture, i32, i32, i1) #0

attributes #0 = { nounwind }

