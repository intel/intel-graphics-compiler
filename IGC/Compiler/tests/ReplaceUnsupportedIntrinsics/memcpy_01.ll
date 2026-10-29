;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-replace-unsupported-intrinsics -verify -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

target triple = "igil_32_GEN12"

%struct.data_t = type { [97 x <2 x i8>] }

; Function Attrs: nounwind
define void @test_kernel(%struct.data_t addrspace(1)* %p, %struct.data_t addrspace(1)* %q) #0 {
; CHECK: [[pIV:%[a-zA-Z0-9_]+]] = alloca i32
; CHECK-TYPED: [[vSrc:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to <8 x i32> addrspace(1)*
; CHECK-TYPED: [[vDst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to <8 x i32> addrspace(1)*
; CHECK: memcpy.body:
; CHECK-TYPED:   [[IV:%[a-zA-Z0-9_]+]] = load i32, i32* [[pIV]]
; CHECK-OPAQUE:   [[IV:%[a-zA-Z0-9_]+]] = load i32, ptr [[pIV]]
; CHECK-TYPED:   [[GEP0:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[vSrc]], i32 [[IV]]
; CHECK-OPAQUE:   [[GEP0:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, ptr addrspace(1) %q, i32 [[IV]]
; CHECK-TYPED:   [[GEP1:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[vDst]], i32 [[IV]]
; CHECK-OPAQUE:   [[GEP1:%[a-zA-Z0-9_]+]] = getelementptr <8 x i32>, ptr addrspace(1) %p, i32 [[IV]]
; CHECK-TYPED:   [[LD:%[a-zA-Z0-9_]+]] = load <8 x i32>, <8 x i32> addrspace(1)* [[GEP0]], align 2
; CHECK-OPAQUE:   [[LD:%[a-zA-Z0-9_]+]] = load <8 x i32>, ptr addrspace(1) [[GEP0]], align 2
; CHECK-TYPED:   store <8 x i32> [[LD]], <8 x i32> addrspace(1)* [[GEP1]], align 2
; CHECK-OPAQUE:   store <8 x i32> [[LD]], ptr addrspace(1) [[GEP1]], align 2
; CHECK:   [[INC0:%[a-zA-Z0-9_]+]] = add i32 [[IV]], 1
; CHECK-TYPED:   store i32 [[INC0]], i32* [[pIV]]
; CHECK-OPAQUE:   store i32 [[INC0]], ptr [[pIV]]
; CHECK:   [[CMP:%[a-zA-Z0-9_]+]] = icmp ult i32 [[INC0]], 12
; CHECK:   br i1 [[CMP]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.post:
; CHECK-TYPED:   [[memcpy_src:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %q to i8 addrspace(1)*
; CHECK-TYPED:   [[memcpy_dst:%[a-zA-Z0-9_]+]] = bitcast %struct.data_t addrspace(1)* %p to i8 addrspace(1)*
; CHECK-TYPED:   [[GEP2:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 384
; CHECK-OPAQUE:   [[GEP2:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %q, i32 384
; CHECK-TYPED:   [[GEP3:%[_a-zA-Z0-9_]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 384
; CHECK-OPAQUE:   [[GEP3:%[_a-zA-Z0-9_]+]] = getelementptr i8, ptr addrspace(1) %p, i32 384
; CHECK-TYPED:   [[PTR0:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP2]] to i32 addrspace(1)*
; CHECK-TYPED:   [[PTR1:%[_a-zA-Z0-9_]+]] = bitcast i8 addrspace(1)* [[GEP3]] to i32 addrspace(1)*
; CHECK-TYPED:   [[L0:%[_a-zA-Z0-9_]+]] = load i32, i32 addrspace(1)* [[PTR0]], align 2
; CHECK-OPAQUE:   [[L0:%[_a-zA-Z0-9_]+]] = load i32, ptr addrspace(1) [[GEP2]], align 2
; CHECK-TYPED:   store i32 [[L0]], i32 addrspace(1)* [[PTR1]], align 2
; CHECK-OPAQUE:   store i32 [[L0]], ptr addrspace(1) [[GEP3]], align 2
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
; CHECK-TYPED:    [[DST:%.*]] = bitcast [[STRUCT_DATA_T_2:%.*]] addrspace(1)* [[P:%.*]] to i8 addrspace(1)*
; CHECK-OPAQUE:    [[DST:%.*]] = bitcast ptr addrspace(1) [[P:%.*]] to ptr addrspace(1)
; CHECK-TYPED:    [[SRC:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[Q:%.*]] to i8 addrspace(1)*
; CHECK-OPAQUE:    [[SRC:%.*]] = bitcast ptr addrspace(1) [[Q:%.*]] to ptr addrspace(1)
; CHECK-TYPED:    [[MEMCPY_VSRC:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[Q]] to <8 x i32> addrspace(1)*
; CHECK-TYPED:    [[MEMCPY_VDST:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[P]] to <8 x i32> addrspace(1)*

; CHECK-TYPED:    [[TMP1:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 0
; CHECK-OPAQUE:    [[TMP1:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[Q]], i32 0
; CHECK-TYPED:    [[TMP2:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 0
; CHECK-OPAQUE:    [[TMP2:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[P]], i32 0
; CHECK-TYPED:    [[TMP3:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP1]], align 2
; CHECK-OPAQUE:    [[TMP3:%.*]] = load <8 x i32>, ptr addrspace(1) [[TMP1]], align 2
; CHECK-TYPED:    store <8 x i32> [[TMP3]], <8 x i32> addrspace(1)* [[TMP2]], align 2
; CHECK-OPAQUE:    store <8 x i32> [[TMP3]], ptr addrspace(1) [[TMP2]], align 2
; CHECK-TYPED:    [[TMP4:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 1
; CHECK-OPAQUE:    [[TMP4:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[Q]], i32 1
; CHECK-TYPED:    [[TMP5:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 1
; CHECK-OPAQUE:    [[TMP5:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[P]], i32 1
; CHECK-TYPED:    [[TMP6:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP4]], align 2
; CHECK-OPAQUE:    [[TMP6:%.*]] = load <8 x i32>, ptr addrspace(1) [[TMP4]], align 2
; CHECK-TYPED:    store <8 x i32> [[TMP6]], <8 x i32> addrspace(1)* [[TMP5]], align 2
; CHECK-OPAQUE:    store <8 x i32> [[TMP6]], ptr addrspace(1) [[TMP5]], align 2
; CHECK-TYPED:    [[TMP7:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 2
; CHECK-OPAQUE:    [[TMP7:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[Q]], i32 2
; CHECK-TYPED:    [[TMP8:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 2
; CHECK-OPAQUE:    [[TMP8:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[P]], i32 2
; CHECK-TYPED:    [[TMP9:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP7]], align 2
; CHECK-OPAQUE:    [[TMP9:%.*]] = load <8 x i32>, ptr addrspace(1) [[TMP7]], align 2
; CHECK-TYPED:    store <8 x i32> [[TMP9]], <8 x i32> addrspace(1)* [[TMP8]], align 2
; CHECK-OPAQUE:    store <8 x i32> [[TMP9]], ptr addrspace(1) [[TMP8]], align 2
; CHECK-TYPED:    [[TMP10:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 3
; CHECK-OPAQUE:    [[TMP10:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[Q]], i32 3
; CHECK-TYPED:    [[TMP11:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 3
; CHECK-OPAQUE:    [[TMP11:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[P]], i32 3
; CHECK-TYPED:    [[TMP12:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP10]], align 2
; CHECK-OPAQUE:    [[TMP12:%.*]] = load <8 x i32>, ptr addrspace(1) [[TMP10]], align 2
; CHECK-TYPED:    store <8 x i32> [[TMP12]], <8 x i32> addrspace(1)* [[TMP11]], align 2
; CHECK-OPAQUE:    store <8 x i32> [[TMP12]], ptr addrspace(1) [[TMP11]], align 2
; CHECK-TYPED:    [[TMP13:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 4
; CHECK-OPAQUE:    [[TMP13:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[Q]], i32 4
; CHECK-TYPED:    [[TMP14:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 4
; CHECK-OPAQUE:    [[TMP14:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[P]], i32 4
; CHECK-TYPED:    [[TMP15:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP13]], align 2
; CHECK-OPAQUE:    [[TMP15:%.*]] = load <8 x i32>, ptr addrspace(1) [[TMP13]], align 2
; CHECK-TYPED:    store <8 x i32> [[TMP15]], <8 x i32> addrspace(1)* [[TMP14]], align 2
; CHECK-OPAQUE:    store <8 x i32> [[TMP15]], ptr addrspace(1) [[TMP14]], align 2
; CHECK-TYPED:    [[TMP16:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VSRC]], i32 5
; CHECK-OPAQUE:    [[TMP16:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[Q]], i32 5
; CHECK-TYPED:    [[TMP17:%.*]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[MEMCPY_VDST]], i32 5
; CHECK-OPAQUE:    [[TMP17:%.*]] = getelementptr <8 x i32>, ptr addrspace(1) [[P]], i32 5
; CHECK-TYPED:    [[TMP18:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[TMP16]], align 2
; CHECK-OPAQUE:    [[TMP18:%.*]] = load <8 x i32>, ptr addrspace(1) [[TMP16]], align 2
; CHECK-TYPED:    store <8 x i32> [[TMP18]], <8 x i32> addrspace(1)* [[TMP17]], align 2
; CHECK-OPAQUE:    store <8 x i32> [[TMP18]], ptr addrspace(1) [[TMP17]], align 2

; CHECK-TYPED:    [[MEMCPY_SRC:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[Q]] to i8 addrspace(1)*
; CHECK-TYPED:    [[MEMCPY_DST:%.*]] = bitcast [[STRUCT_DATA_T_2]] addrspace(1)* [[P]] to i8 addrspace(1)*
; CHECK-TYPED:    [[TMP19:%.*]] = getelementptr i8, i8 addrspace(1)* [[MEMCPY_SRC]], i32 192
; CHECK-OPAQUE:    [[TMP19:%.*]] = getelementptr i8, ptr addrspace(1) [[Q]], i32 192
; CHECK-TYPED:    [[TMP20:%.*]] = getelementptr i8, i8 addrspace(1)* [[MEMCPY_DST]], i32 192
; CHECK-OPAQUE:    [[TMP20:%.*]] = getelementptr i8, ptr addrspace(1) [[P]], i32 192
; CHECK-TYPED:    [[MEMCPY_REM:%.*]] = bitcast i8 addrspace(1)* [[TMP19]] to i32 addrspace(1)*
; CHECK-TYPED:    [[MEMCPY_REM1:%.*]] = bitcast i8 addrspace(1)* [[TMP20]] to i32 addrspace(1)*
; CHECK-TYPED:    [[TMP21:%.*]] = load i32, i32 addrspace(1)* [[MEMCPY_REM]], align 2
; CHECK-OPAQUE:    [[TMP21:%.*]] = load i32, ptr addrspace(1) [[TMP19]], align 2
; CHECK-TYPED:    store i32 [[TMP21]], i32 addrspace(1)* [[MEMCPY_REM1]], align 2
; CHECK-OPAQUE:    store i32 [[TMP21]], ptr addrspace(1) [[TMP20]], align 2
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

