;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

target triple = "igil_32_GEN8"

define void @A(i8 * %Src, i8 * %Dst, i32 %S, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
entry:
; CHECK-LABEL: A
; CHECK:  %pIV = alloca i32
; CHECK:  %0 = icmp ugt i32 %S, 1
; CHECK:  %umax = select i1 %0, i32 %S, i32 1
; CHECK:  store i32 0, i32* %pIV
; CHECK:  [[CMP0:%[a-zA-Z0-9]+]] = icmp ult i32 0, %umax
; CHECK:  br i1 [[CMP0]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.body:
; CHECK:   %IV = load i32, i32* %pIV
; CHECK:   [[GEP0:%[a-zA-Z0-9]+]] = getelementptr i8, i8* %Src, i32 %IV
; CHECK:   [[GEP1:%[a-zA-Z0-9]+]] = getelementptr i8, i8* %Dst, i32 %IV
; CHECK:   [[LD:%[a-zA-Z0-9]+]] = load i8, i8* [[GEP0]], align 1
; CHECK:   store i8 [[LD]], i8* [[GEP1]], align 1
; CHECK:   [[INC0:%[a-zA-Z0-9]+]] = add i32 %IV, 1
; CHECK:   store i32 [[INC0]], i32* %pIV
; CHECK:   [[CMP1:%[a-zA-Z0-9]+]] = icmp ult i32 [[INC0]], %umax
; CHECK:   br i1 [[CMP1]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.post:
; CHECK: ret
  %0 = icmp ugt i32 %S, 1
  %umax = select i1 %0, i32 %S, i32 1
  call void @llvm.memcpy.p0i8.p0i8.i32(i8 * %Dst, i8 * %Src, i32 %umax, i32 1, i1 false)
  ret void
}

define void @B(i8 * %Src, i8 * %Dst, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
entry:
; CHECK-LABEL: B
; CHECK: ret
  call void @llvm.memcpy.p0i8.p0i8.i32(i8 * %Dst, i8 * %Src, i32 4096, i32 1, i1 false)
  ret void
}

define void @C(i8 * %Src, i8 * %Dst, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
entry:
; CHECK-LABEL: C
; CHECK: ret
  call void @llvm.memcpy.p0i8.p0i8.i32(i8 * %Dst, i8 * %Src, i32 1, i32 1, i1 false)
  ret void
}

@.str = private addrspace(2) constant [4 x i8] c"ocl\00", align 1
@.str1 = private addrspace(2) constant [4 x i8] c"c99\00", align 1
@.str2 = private addrspace(2) constant [4 x i8] c"gcc\00", align 1
@ocl_test_kernel.args = private unnamed_addr constant [3 x i8 addrspace(2)*] [i8 addrspace(2)* bitcast ([4 x i8] addrspace(2)* @.str to i8 addrspace(2)*), i8 addrspace(2)* bitcast ([4 x i8] addrspace(2)* @.str1 to i8 addrspace(2)*), i8 addrspace(2)* bitcast ([4 x i8] addrspace(2)* @.str2 to i8 addrspace(2)*)], align 4

define void @string_array(i32 addrspace(1)* %ocl_test_results, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
entry:
; CHECK-LABEL: string_array
; CHECK:      entry:
; CHECK-NEXT:  %args = alloca [3 x i8 addrspace(2)*], align 4
; CHECK-NEXT:  %0 = bitcast [3 x i8 addrspace(2)*]* %args to i8*
; CHECK-NEXT:  %1 = bitcast [3 x i8 addrspace(2)*]* @ocl_test_kernel.args to i8*
; CHECK-NEXT:  [[memcpy_rem:%[a-zA-Z0-9_]+]] = bitcast [3 x i8 addrspace(2)*]* %args to i8*
; CHECK-NEXT:  [[memcpy_rem1:%[a-zA-Z0-9_]+]] = bitcast [3 x i8 addrspace(2)*]* %args to <3 x i32>*
; CHECK-NEXT:  [[load1:%[a-zA-Z0-9_]+]] = load <3 x i32>, <3 x i32>* bitcast ([3 x i8 addrspace(2)*]* @ocl_test_kernel.args to <3 x i32>*), align 4
; CHECK-NEXT:  store <3 x i32> [[load1]], <3 x i32>* [[memcpy_rem1]], align 4
; CHECK-NEXT:  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %ocl_test_results, i32 0
; CHECK:      ret
  %args = alloca [3 x i8 addrspace(2)*], align 4
  %0 = bitcast [3 x i8 addrspace(2)*]* %args to i8*
  %1 = bitcast [3 x i8 addrspace(2)*]* @ocl_test_kernel.args to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %0, i8* %1, i32 12, i32 4, i1 false)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %ocl_test_results, i32 0
  store i32 1, i32 addrspace(1)* %arrayidx, align 4
  %arraydecay = getelementptr inbounds [3 x i8 addrspace(2)*], [3 x i8 addrspace(2)*]* %args, i32 0, i32 0
  %arrayidx.i = getelementptr inbounds i32, i32 addrspace(1)* %ocl_test_results, i32 1
  %2 = load i32, i32 addrspace(1)* %arrayidx.i, align 4
  %3 = load i32, i32 addrspace(1)* %ocl_test_results, align 4
  %arrayidx2.i = getelementptr inbounds i8 addrspace(2)*, i8 addrspace(2)** %arraydecay, i32 %3
  %4 = load i8 addrspace(2)*, i8 addrspace(2)** %arrayidx2.i, align 4
  %arrayidx3.i = getelementptr inbounds i8, i8 addrspace(2)* %4, i32 %2
  %5 = load i8, i8 addrspace(2)* %arrayidx3.i, align 1
  %conv.i = sext i8 %5 to i32
  store i32 %conv.i, i32 addrspace(1)* %ocl_test_results, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %ocl_test_results, i32 3
  store i32 undef, i32 addrspace(1)* %arrayidx1, align 4
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %ocl_test_results, i32 0
  store i32 2, i32 addrspace(1)* %arrayidx2, align 4
  ret void
}

declare void @llvm.memcpy.p0i8.p0i8.i32(i8* nocapture, i8* nocapture, i32, i32, i1) #0

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind }
