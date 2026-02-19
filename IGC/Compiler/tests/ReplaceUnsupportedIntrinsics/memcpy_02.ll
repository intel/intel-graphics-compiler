;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
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

