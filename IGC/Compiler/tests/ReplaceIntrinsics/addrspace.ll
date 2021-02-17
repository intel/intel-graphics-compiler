;=========================== begin_copyright_notice ============================
;
; Copyright (c) 2014-2021 Intel Corporation
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"),
; to deal in the Software without restriction, including without limitation
; the rights to use, copy, modify, merge, publish, distribute, sublicense,
; and/or sell copies of the Software, and to permit persons to whom
; the Software is furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included
; in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
; IN THE SOFTWARE.
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

target triple = "igil_32_GEN8"

define void @A(i8 addrspace(1)* %Src, i8 addrspace(3)* %Dst, i32 %S, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
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
; CHECK:   [[GEP0:%[a-zA-Z0-9]+]] = getelementptr i8, i8 addrspace(1)* %Src, i32 %IV
; CHECK:   [[GEP1:%[a-zA-Z0-9]+]] = getelementptr i8, i8 addrspace(3)* %Dst, i32 %IV
; CHECK:   [[LD:%[a-zA-Z0-9]+]] = load i8, i8 addrspace(1)* [[GEP0]], align 1
; CHECK:   store i8 [[LD]], i8 addrspace(3)* [[GEP1]], align 1
; CHECK:   [[INC0:%[a-zA-Z0-9]+]] = add i32 %IV, 1
; CHECK:   store i32 [[INC0]], i32* %pIV
; CHECK:   [[CMP1:%[a-zA-Z0-9]+]] = icmp ult i32 [[INC0]], %umax
; CHECK:   br i1 [[CMP1]], label %memcpy.body, label %memcpy.post
; CHECK: memcpy.post:
; CHECK: ret
  %0 = icmp ugt i32 %S, 1
  %umax = select i1 %0, i32 %S, i32 1
  call void @llvm.memcpy.p0i8.p0i8.i32(i8 addrspace(3)* %Dst, i8 addrspace(1)* %Src, i32 %umax, i32 1, i1 false)
  ret void
}

declare void @llvm.memcpy.p0i8.p0i8.i32(i8 addrspace(3)* nocapture, i8 addrspace(1)* nocapture, i32, i32, i1) #0

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind }
