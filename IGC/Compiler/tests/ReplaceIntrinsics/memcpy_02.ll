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

target triple = "igil_32_GEN9"

%struct.Material = type { [4 x %struct.ColorPower] }
%struct.ColorPower = type { half }
%aggregation = type { [4 x %struct.Material] }

; Function Attrs: nounwind
define void @test_kernel(%aggregation addrspace(1)* %p, %aggregation addrspace(1)* %q) #0 {
; CHECK:   [[vSrc:%[a-zA-Z0-9_]+]] = bitcast %aggregation addrspace(1)* %q to <16 x i16> addrspace(1)*
; CHECK:   [[vDst:%[a-zA-Z0-9_]+]] = bitcast %aggregation addrspace(1)* %p to <16 x i16> addrspace(1)*
; CHECK:   [[GEP0:%[a-zA-Z0-9_]+]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[vSrc]], i32 0
; CHECK:   [[GEP1:%[a-zA-Z0-9_]+]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[vDst]], i32 0
; CHECK:   [[LD:%[a-zA-Z0-9_]+]] = load <16 x i16>, <16 x i16> addrspace(1)* [[GEP0]]
; CHECK:   store <16 x i16> [[LD]], <16 x i16> addrspace(1)* [[GEP1]]
; CHECK:   ret void
  %Dst = bitcast %aggregation addrspace(1)* %p to i8 addrspace(1)*
  %Src = bitcast %aggregation addrspace(1)* %q to i8 addrspace(1)*
  call void @llvm.memcpy.p1i8.p1i8.i64(i8 addrspace(1)* %Dst, i8 addrspace(1)* %Src, i64 32, i1 false)
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p1i8.p1i8.i64(i8 addrspace(1)* nocapture, i8 addrspace(1)* nocapture, i64, i1) #0

attributes #0 = { nounwind }

