;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-custom-safe-opt -S < %s | FileCheck %s
;
; Test if first insertelement's source is replaced with poison value.
; Similar to LLVM instcombine optimization, but works on larger vectors.

;
; i8 tests
;

define spir_kernel void @test_v2i8(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v2i8
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <2 x i8> [ zeroinitializer, %.preheader ], [ %i1, %.loop ]
  %input = load <64 x i8>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <64 x i8> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <2 x i8> poison, i8 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <64 x i8> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <2 x i8> %i0, i8 %e1, i32 1
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <64 x i8> %input, i32 0
  %i0 = insertelement <2 x i8> %old, i8 %e0, i32 0
  %e1 = extractelement <64 x i8> %input, i32 1
  %i1 = insertelement <2 x i8> %i0, i8 %e1, i32 1
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <2 x i8> [ %i1, %.loop ]
  %output = phi <2 x i8> [ %i1, %.loop ]
  store <2 x i8> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v4i8(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v4i8
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <4 x i8> [ zeroinitializer, %.preheader ], [ %i3, %.loop ]
  %input = load <64 x i8>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <64 x i8> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <4 x i8> poison, i8 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <64 x i8> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <4 x i8> %i0, i8 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <64 x i8> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <4 x i8> %i1, i8 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <64 x i8> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <4 x i8> %i2, i8 %e3, i32 3
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <64 x i8> %input, i32 0
  %i0 = insertelement <4 x i8> %old, i8 %e0, i32 0
  %e1 = extractelement <64 x i8> %input, i32 1
  %i1 = insertelement <4 x i8> %i0, i8 %e1, i32 1
  %e2 = extractelement <64 x i8> %input, i32 2
  %i2 = insertelement <4 x i8> %i1, i8 %e2, i32 2
  %e3 = extractelement <64 x i8> %input, i32 3
  %i3 = insertelement <4 x i8> %i2, i8 %e3, i32 3
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <4 x i8> [ %i3, %.loop ]
  %output = phi <4 x i8> [ %i3, %.loop ]
  store <4 x i8> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v8i8(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v8i8
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <8 x i8> [ zeroinitializer, %.preheader ], [ %i7, %.loop ]
  %input = load <64 x i8>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <64 x i8> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <8 x i8> poison, i8 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <64 x i8> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <8 x i8> %i0, i8 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <64 x i8> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <8 x i8> %i1, i8 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <64 x i8> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <8 x i8> %i2, i8 %e3, i32 3
; CHECK-NEXT:  %e4 = extractelement <64 x i8> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <8 x i8> %i3, i8 %e4, i32 4
; CHECK-NEXT:  %e5 = extractelement <64 x i8> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <8 x i8> %i4, i8 %e5, i32 5
; CHECK-NEXT:  %e6 = extractelement <64 x i8> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <8 x i8> %i5, i8 %e6, i32 6
; CHECK-NEXT:  %e7 = extractelement <64 x i8> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <8 x i8> %i6, i8 %e7, i32 7
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <64 x i8> %input, i32 0
  %i0 = insertelement <8 x i8> %old, i8 %e0, i32 0
  %e1 = extractelement <64 x i8> %input, i32 1
  %i1 = insertelement <8 x i8> %i0, i8 %e1, i32 1
  %e2 = extractelement <64 x i8> %input, i32 2
  %i2 = insertelement <8 x i8> %i1, i8 %e2, i32 2
  %e3 = extractelement <64 x i8> %input, i32 3
  %i3 = insertelement <8 x i8> %i2, i8 %e3, i32 3
  %e4 = extractelement <64 x i8> %input, i32 4
  %i4 = insertelement <8 x i8> %i3, i8 %e4, i32 4
  %e5 = extractelement <64 x i8> %input, i32 5
  %i5 = insertelement <8 x i8> %i4, i8 %e5, i32 5
  %e6 = extractelement <64 x i8> %input, i32 6
  %i6 = insertelement <8 x i8> %i5, i8 %e6, i32 6
  %e7 = extractelement <64 x i8> %input, i32 7
  %i7 = insertelement <8 x i8> %i6, i8 %e7, i32 7
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <8 x i8> [ %i7, %.loop ]
  %output = phi <8 x i8> [ %i7, %.loop ]
  store <8 x i8> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v16i8(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v16i8
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <16 x i8> [ zeroinitializer, %.preheader ], [ %i15, %.loop ]
  %input = load <64 x i8>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <64 x i8> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <16 x i8> poison, i8 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <64 x i8> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <16 x i8> %i0, i8 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <64 x i8> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <16 x i8> %i1, i8 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <64 x i8> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <16 x i8> %i2, i8 %e3, i32 3
; CHECK-NEXT:  %e4 = extractelement <64 x i8> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <16 x i8> %i3, i8 %e4, i32 4
; CHECK-NEXT:  %e5 = extractelement <64 x i8> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <16 x i8> %i4, i8 %e5, i32 5
; CHECK-NEXT:  %e6 = extractelement <64 x i8> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <16 x i8> %i5, i8 %e6, i32 6
; CHECK-NEXT:  %e7 = extractelement <64 x i8> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <16 x i8> %i6, i8 %e7, i32 7
; CHECK-NEXT:  %e8 = extractelement <64 x i8> %input, i32 8
; CHECK-NEXT:  %i8 = insertelement <16 x i8> %i7, i8 %e8, i32 8
; CHECK-NEXT:  %e9 = extractelement <64 x i8> %input, i32 9
; CHECK-NEXT:  %i9 = insertelement <16 x i8> %i8, i8 %e9, i32 9
; CHECK-NEXT:  %e10 = extractelement <64 x i8> %input, i32 10
; CHECK-NEXT:  %i10 = insertelement <16 x i8> %i9, i8 %e10, i32 10
; CHECK-NEXT:  %e11 = extractelement <64 x i8> %input, i32 11
; CHECK-NEXT:  %i11 = insertelement <16 x i8> %i10, i8 %e11, i32 11
; CHECK-NEXT:  %e12 = extractelement <64 x i8> %input, i32 12
; CHECK-NEXT:  %i12 = insertelement <16 x i8> %i11, i8 %e12, i32 12
; CHECK-NEXT:  %e13 = extractelement <64 x i8> %input, i32 13
; CHECK-NEXT:  %i13 = insertelement <16 x i8> %i12, i8 %e13, i32 13
; CHECK-NEXT:  %e14 = extractelement <64 x i8> %input, i32 14
; CHECK-NEXT:  %i14 = insertelement <16 x i8> %i13, i8 %e14, i32 14
; CHECK-NEXT:  %e15 = extractelement <64 x i8> %input, i32 15
; CHECK-NEXT:  %i15 = insertelement <16 x i8> %i14, i8 %e15, i32 15
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <64 x i8> %input, i32 0
  %i0 = insertelement <16 x i8> %old, i8 %e0, i32 0
  %e1 = extractelement <64 x i8> %input, i32 1
  %i1 = insertelement <16 x i8> %i0, i8 %e1, i32 1
  %e2 = extractelement <64 x i8> %input, i32 2
  %i2 = insertelement <16 x i8> %i1, i8 %e2, i32 2
  %e3 = extractelement <64 x i8> %input, i32 3
  %i3 = insertelement <16 x i8> %i2, i8 %e3, i32 3
  %e4 = extractelement <64 x i8> %input, i32 4
  %i4 = insertelement <16 x i8> %i3, i8 %e4, i32 4
  %e5 = extractelement <64 x i8> %input, i32 5
  %i5 = insertelement <16 x i8> %i4, i8 %e5, i32 5
  %e6 = extractelement <64 x i8> %input, i32 6
  %i6 = insertelement <16 x i8> %i5, i8 %e6, i32 6
  %e7 = extractelement <64 x i8> %input, i32 7
  %i7 = insertelement <16 x i8> %i6, i8 %e7, i32 7
  %e8 = extractelement <64 x i8> %input, i32 8
  %i8 = insertelement <16 x i8> %i7, i8 %e8, i32 8
  %e9 = extractelement <64 x i8> %input, i32 9
  %i9 = insertelement <16 x i8> %i8, i8 %e9, i32 9
  %e10 = extractelement <64 x i8> %input, i32 10
  %i10 = insertelement <16 x i8> %i9, i8 %e10, i32 10
  %e11 = extractelement <64 x i8> %input, i32 11
  %i11 = insertelement <16 x i8> %i10, i8 %e11, i32 11
  %e12 = extractelement <64 x i8> %input, i32 12
  %i12 = insertelement <16 x i8> %i11, i8 %e12, i32 12
  %e13 = extractelement <64 x i8> %input, i32 13
  %i13 = insertelement <16 x i8> %i12, i8 %e13, i32 13
  %e14 = extractelement <64 x i8> %input, i32 14
  %i14 = insertelement <16 x i8> %i13, i8 %e14, i32 14
  %e15 = extractelement <64 x i8> %input, i32 15
  %i15 = insertelement <16 x i8> %i14, i8 %e15, i32 15
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <16 x i8> [ %i15, %.loop ]
  %output = phi <16 x i8> [ %i15, %.loop ]
  store <16 x i8> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v32i8(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v32i8
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <32 x i8> [ zeroinitializer, %.preheader ], [ %i31, %.loop ]
  %input = load <64 x i8>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <64 x i8> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <32 x i8> poison, i8 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <64 x i8> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <32 x i8> %i0, i8 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <64 x i8> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <32 x i8> %i1, i8 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <64 x i8> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <32 x i8> %i2, i8 %e3, i32 3
; CHECK-NEXT:  %e4 = extractelement <64 x i8> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <32 x i8> %i3, i8 %e4, i32 4
; CHECK-NEXT:  %e5 = extractelement <64 x i8> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <32 x i8> %i4, i8 %e5, i32 5
; CHECK-NEXT:  %e6 = extractelement <64 x i8> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <32 x i8> %i5, i8 %e6, i32 6
; CHECK-NEXT:  %e7 = extractelement <64 x i8> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <32 x i8> %i6, i8 %e7, i32 7
; CHECK-NEXT:  %e8 = extractelement <64 x i8> %input, i32 8
; CHECK-NEXT:  %i8 = insertelement <32 x i8> %i7, i8 %e8, i32 8
; CHECK-NEXT:  %e9 = extractelement <64 x i8> %input, i32 9
; CHECK-NEXT:  %i9 = insertelement <32 x i8> %i8, i8 %e9, i32 9
; CHECK-NEXT:  %e10 = extractelement <64 x i8> %input, i32 10
; CHECK-NEXT:  %i10 = insertelement <32 x i8> %i9, i8 %e10, i32 10
; CHECK-NEXT:  %e11 = extractelement <64 x i8> %input, i32 11
; CHECK-NEXT:  %i11 = insertelement <32 x i8> %i10, i8 %e11, i32 11
; CHECK-NEXT:  %e12 = extractelement <64 x i8> %input, i32 12
; CHECK-NEXT:  %i12 = insertelement <32 x i8> %i11, i8 %e12, i32 12
; CHECK-NEXT:  %e13 = extractelement <64 x i8> %input, i32 13
; CHECK-NEXT:  %i13 = insertelement <32 x i8> %i12, i8 %e13, i32 13
; CHECK-NEXT:  %e14 = extractelement <64 x i8> %input, i32 14
; CHECK-NEXT:  %i14 = insertelement <32 x i8> %i13, i8 %e14, i32 14
; CHECK-NEXT:  %e15 = extractelement <64 x i8> %input, i32 15
; CHECK-NEXT:  %i15 = insertelement <32 x i8> %i14, i8 %e15, i32 15
; CHECK-NEXT:  %e16 = extractelement <64 x i8> %input, i32 16
; CHECK-NEXT:  %i16 = insertelement <32 x i8> %i15, i8 %e16, i32 16
; CHECK-NEXT:  %e17 = extractelement <64 x i8> %input, i32 17
; CHECK-NEXT:  %i17 = insertelement <32 x i8> %i16, i8 %e17, i32 17
; CHECK-NEXT:  %e18 = extractelement <64 x i8> %input, i32 18
; CHECK-NEXT:  %i18 = insertelement <32 x i8> %i17, i8 %e18, i32 18
; CHECK-NEXT:  %e19 = extractelement <64 x i8> %input, i32 19
; CHECK-NEXT:  %i19 = insertelement <32 x i8> %i18, i8 %e19, i32 19
; CHECK-NEXT:  %e20 = extractelement <64 x i8> %input, i32 20
; CHECK-NEXT:  %i20 = insertelement <32 x i8> %i19, i8 %e20, i32 20
; CHECK-NEXT:  %e21 = extractelement <64 x i8> %input, i32 21
; CHECK-NEXT:  %i21 = insertelement <32 x i8> %i20, i8 %e21, i32 21
; CHECK-NEXT:  %e22 = extractelement <64 x i8> %input, i32 22
; CHECK-NEXT:  %i22 = insertelement <32 x i8> %i21, i8 %e22, i32 22
; CHECK-NEXT:  %e23 = extractelement <64 x i8> %input, i32 23
; CHECK-NEXT:  %i23 = insertelement <32 x i8> %i22, i8 %e23, i32 23
; CHECK-NEXT:  %e24 = extractelement <64 x i8> %input, i32 24
; CHECK-NEXT:  %i24 = insertelement <32 x i8> %i23, i8 %e24, i32 24
; CHECK-NEXT:  %e25 = extractelement <64 x i8> %input, i32 25
; CHECK-NEXT:  %i25 = insertelement <32 x i8> %i24, i8 %e25, i32 25
; CHECK-NEXT:  %e26 = extractelement <64 x i8> %input, i32 26
; CHECK-NEXT:  %i26 = insertelement <32 x i8> %i25, i8 %e26, i32 26
; CHECK-NEXT:  %e27 = extractelement <64 x i8> %input, i32 27
; CHECK-NEXT:  %i27 = insertelement <32 x i8> %i26, i8 %e27, i32 27
; CHECK-NEXT:  %e28 = extractelement <64 x i8> %input, i32 28
; CHECK-NEXT:  %i28 = insertelement <32 x i8> %i27, i8 %e28, i32 28
; CHECK-NEXT:  %e29 = extractelement <64 x i8> %input, i32 29
; CHECK-NEXT:  %i29 = insertelement <32 x i8> %i28, i8 %e29, i32 29
; CHECK-NEXT:  %e30 = extractelement <64 x i8> %input, i32 30
; CHECK-NEXT:  %i30 = insertelement <32 x i8> %i29, i8 %e30, i32 30
; CHECK-NEXT:  %e31 = extractelement <64 x i8> %input, i32 31
; CHECK-NEXT:  %i31 = insertelement <32 x i8> %i30, i8 %e31, i32 31
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <64 x i8> %input, i32 0
  %i0 = insertelement <32 x i8> %old, i8 %e0, i32 0
  %e1 = extractelement <64 x i8> %input, i32 1
  %i1 = insertelement <32 x i8> %i0, i8 %e1, i32 1
  %e2 = extractelement <64 x i8> %input, i32 2
  %i2 = insertelement <32 x i8> %i1, i8 %e2, i32 2
  %e3 = extractelement <64 x i8> %input, i32 3
  %i3 = insertelement <32 x i8> %i2, i8 %e3, i32 3
  %e4 = extractelement <64 x i8> %input, i32 4
  %i4 = insertelement <32 x i8> %i3, i8 %e4, i32 4
  %e5 = extractelement <64 x i8> %input, i32 5
  %i5 = insertelement <32 x i8> %i4, i8 %e5, i32 5
  %e6 = extractelement <64 x i8> %input, i32 6
  %i6 = insertelement <32 x i8> %i5, i8 %e6, i32 6
  %e7 = extractelement <64 x i8> %input, i32 7
  %i7 = insertelement <32 x i8> %i6, i8 %e7, i32 7
  %e8 = extractelement <64 x i8> %input, i32 8
  %i8 = insertelement <32 x i8> %i7, i8 %e8, i32 8
  %e9 = extractelement <64 x i8> %input, i32 9
  %i9 = insertelement <32 x i8> %i8, i8 %e9, i32 9
  %e10 = extractelement <64 x i8> %input, i32 10
  %i10 = insertelement <32 x i8> %i9, i8 %e10, i32 10
  %e11 = extractelement <64 x i8> %input, i32 11
  %i11 = insertelement <32 x i8> %i10, i8 %e11, i32 11
  %e12 = extractelement <64 x i8> %input, i32 12
  %i12 = insertelement <32 x i8> %i11, i8 %e12, i32 12
  %e13 = extractelement <64 x i8> %input, i32 13
  %i13 = insertelement <32 x i8> %i12, i8 %e13, i32 13
  %e14 = extractelement <64 x i8> %input, i32 14
  %i14 = insertelement <32 x i8> %i13, i8 %e14, i32 14
  %e15 = extractelement <64 x i8> %input, i32 15
  %i15 = insertelement <32 x i8> %i14, i8 %e15, i32 15
  %e16 = extractelement <64 x i8> %input, i32 16
  %i16 = insertelement <32 x i8> %i15, i8 %e16, i32 16
  %e17 = extractelement <64 x i8> %input, i32 17
  %i17 = insertelement <32 x i8> %i16, i8 %e17, i32 17
  %e18 = extractelement <64 x i8> %input, i32 18
  %i18 = insertelement <32 x i8> %i17, i8 %e18, i32 18
  %e19 = extractelement <64 x i8> %input, i32 19
  %i19 = insertelement <32 x i8> %i18, i8 %e19, i32 19
  %e20 = extractelement <64 x i8> %input, i32 20
  %i20 = insertelement <32 x i8> %i19, i8 %e20, i32 20
  %e21 = extractelement <64 x i8> %input, i32 21
  %i21 = insertelement <32 x i8> %i20, i8 %e21, i32 21
  %e22 = extractelement <64 x i8> %input, i32 22
  %i22 = insertelement <32 x i8> %i21, i8 %e22, i32 22
  %e23 = extractelement <64 x i8> %input, i32 23
  %i23 = insertelement <32 x i8> %i22, i8 %e23, i32 23
  %e24 = extractelement <64 x i8> %input, i32 24
  %i24 = insertelement <32 x i8> %i23, i8 %e24, i32 24
  %e25 = extractelement <64 x i8> %input, i32 25
  %i25 = insertelement <32 x i8> %i24, i8 %e25, i32 25
  %e26 = extractelement <64 x i8> %input, i32 26
  %i26 = insertelement <32 x i8> %i25, i8 %e26, i32 26
  %e27 = extractelement <64 x i8> %input, i32 27
  %i27 = insertelement <32 x i8> %i26, i8 %e27, i32 27
  %e28 = extractelement <64 x i8> %input, i32 28
  %i28 = insertelement <32 x i8> %i27, i8 %e28, i32 28
  %e29 = extractelement <64 x i8> %input, i32 29
  %i29 = insertelement <32 x i8> %i28, i8 %e29, i32 29
  %e30 = extractelement <64 x i8> %input, i32 30
  %i30 = insertelement <32 x i8> %i29, i8 %e30, i32 30
  %e31 = extractelement <64 x i8> %input, i32 31
  %i31 = insertelement <32 x i8> %i30, i8 %e31, i32 31
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <32 x i8> [ %i31, %.loop ]
  %output = phi <32 x i8> [ %i31, %.loop ]
  store <32 x i8> %output, ptr addrspace(1) %dst, align 4
  ret void
}

;
; i16 tests
;

define spir_kernel void @test_v2i16(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v2i16
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <2 x i16> [ zeroinitializer, %.preheader ], [ %i1, %.loop ]
  %input = load <32 x i16>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <32 x i16> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <2 x i16> poison, i16 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <32 x i16> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <2 x i16> %i0, i16 %e1, i32 1
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <32 x i16> %input, i32 0
  %i0 = insertelement <2 x i16> %old, i16 %e0, i32 0
  %e1 = extractelement <32 x i16> %input, i32 1
  %i1 = insertelement <2 x i16> %i0, i16 %e1, i32 1
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <2 x i16> [ %i1, %.loop ]
  %output = phi <2 x i16> [ %i1, %.loop ]
  store <2 x i16> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v4i16(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v4i16
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <4 x i16> [ zeroinitializer, %.preheader ], [ %i3, %.loop ]
  %input = load <32 x i16>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <32 x i16> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <4 x i16> poison, i16 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <32 x i16> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <4 x i16> %i0, i16 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <32 x i16> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <4 x i16> %i1, i16 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <32 x i16> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <4 x i16> %i2, i16 %e3, i32 3
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <32 x i16> %input, i32 0
  %i0 = insertelement <4 x i16> %old, i16 %e0, i32 0
  %e1 = extractelement <32 x i16> %input, i32 1
  %i1 = insertelement <4 x i16> %i0, i16 %e1, i32 1
  %e2 = extractelement <32 x i16> %input, i32 2
  %i2 = insertelement <4 x i16> %i1, i16 %e2, i32 2
  %e3 = extractelement <32 x i16> %input, i32 3
  %i3 = insertelement <4 x i16> %i2, i16 %e3, i32 3
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <4 x i16> [ %i3, %.loop ]
  %output = phi <4 x i16> [ %i3, %.loop ]
  store <4 x i16> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v8i16(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v8i16
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <8 x i16> [ zeroinitializer, %.preheader ], [ %i7, %.loop ]
  %input = load <32 x i16>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <32 x i16> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <8 x i16> poison, i16 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <32 x i16> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <8 x i16> %i0, i16 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <32 x i16> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <8 x i16> %i1, i16 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <32 x i16> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <8 x i16> %i2, i16 %e3, i32 3
; CHECK-NEXT:  %e4 = extractelement <32 x i16> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <8 x i16> %i3, i16 %e4, i32 4
; CHECK-NEXT:  %e5 = extractelement <32 x i16> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <8 x i16> %i4, i16 %e5, i32 5
; CHECK-NEXT:  %e6 = extractelement <32 x i16> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <8 x i16> %i5, i16 %e6, i32 6
; CHECK-NEXT:  %e7 = extractelement <32 x i16> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <8 x i16> %i6, i16 %e7, i32 7
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <32 x i16> %input, i32 0
  %i0 = insertelement <8 x i16> %old, i16 %e0, i32 0
  %e1 = extractelement <32 x i16> %input, i32 1
  %i1 = insertelement <8 x i16> %i0, i16 %e1, i32 1
  %e2 = extractelement <32 x i16> %input, i32 2
  %i2 = insertelement <8 x i16> %i1, i16 %e2, i32 2
  %e3 = extractelement <32 x i16> %input, i32 3
  %i3 = insertelement <8 x i16> %i2, i16 %e3, i32 3
  %e4 = extractelement <32 x i16> %input, i32 4
  %i4 = insertelement <8 x i16> %i3, i16 %e4, i32 4
  %e5 = extractelement <32 x i16> %input, i32 5
  %i5 = insertelement <8 x i16> %i4, i16 %e5, i32 5
  %e6 = extractelement <32 x i16> %input, i32 6
  %i6 = insertelement <8 x i16> %i5, i16 %e6, i32 6
  %e7 = extractelement <32 x i16> %input, i32 7
  %i7 = insertelement <8 x i16> %i6, i16 %e7, i32 7
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <8 x i16> [ %i7, %.loop ]
  %output = phi <8 x i16> [ %i7, %.loop ]
  store <8 x i16> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v16i16(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v16i16
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <16 x i16> [ zeroinitializer, %.preheader ], [ %i15, %.loop ]
  %input = load <32 x i16>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <32 x i16> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <16 x i16> poison, i16 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <32 x i16> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <16 x i16> %i0, i16 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <32 x i16> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <16 x i16> %i1, i16 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <32 x i16> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <16 x i16> %i2, i16 %e3, i32 3
; CHECK-NEXT:  %e4 = extractelement <32 x i16> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <16 x i16> %i3, i16 %e4, i32 4
; CHECK-NEXT:  %e5 = extractelement <32 x i16> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <16 x i16> %i4, i16 %e5, i32 5
; CHECK-NEXT:  %e6 = extractelement <32 x i16> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <16 x i16> %i5, i16 %e6, i32 6
; CHECK-NEXT:  %e7 = extractelement <32 x i16> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <16 x i16> %i6, i16 %e7, i32 7
; CHECK-NEXT:  %e8 = extractelement <32 x i16> %input, i32 8
; CHECK-NEXT:  %i8 = insertelement <16 x i16> %i7, i16 %e8, i32 8
; CHECK-NEXT:  %e9 = extractelement <32 x i16> %input, i32 9
; CHECK-NEXT:  %i9 = insertelement <16 x i16> %i8, i16 %e9, i32 9
; CHECK-NEXT:  %e10 = extractelement <32 x i16> %input, i32 10
; CHECK-NEXT:  %i10 = insertelement <16 x i16> %i9, i16 %e10, i32 10
; CHECK-NEXT:  %e11 = extractelement <32 x i16> %input, i32 11
; CHECK-NEXT:  %i11 = insertelement <16 x i16> %i10, i16 %e11, i32 11
; CHECK-NEXT:  %e12 = extractelement <32 x i16> %input, i32 12
; CHECK-NEXT:  %i12 = insertelement <16 x i16> %i11, i16 %e12, i32 12
; CHECK-NEXT:  %e13 = extractelement <32 x i16> %input, i32 13
; CHECK-NEXT:  %i13 = insertelement <16 x i16> %i12, i16 %e13, i32 13
; CHECK-NEXT:  %e14 = extractelement <32 x i16> %input, i32 14
; CHECK-NEXT:  %i14 = insertelement <16 x i16> %i13, i16 %e14, i32 14
; CHECK-NEXT:  %e15 = extractelement <32 x i16> %input, i32 15
; CHECK-NEXT:  %i15 = insertelement <16 x i16> %i14, i16 %e15, i32 15
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <32 x i16> %input, i32 0
  %i0 = insertelement <16 x i16> %old, i16 %e0, i32 0
  %e1 = extractelement <32 x i16> %input, i32 1
  %i1 = insertelement <16 x i16> %i0, i16 %e1, i32 1
  %e2 = extractelement <32 x i16> %input, i32 2
  %i2 = insertelement <16 x i16> %i1, i16 %e2, i32 2
  %e3 = extractelement <32 x i16> %input, i32 3
  %i3 = insertelement <16 x i16> %i2, i16 %e3, i32 3
  %e4 = extractelement <32 x i16> %input, i32 4
  %i4 = insertelement <16 x i16> %i3, i16 %e4, i32 4
  %e5 = extractelement <32 x i16> %input, i32 5
  %i5 = insertelement <16 x i16> %i4, i16 %e5, i32 5
  %e6 = extractelement <32 x i16> %input, i32 6
  %i6 = insertelement <16 x i16> %i5, i16 %e6, i32 6
  %e7 = extractelement <32 x i16> %input, i32 7
  %i7 = insertelement <16 x i16> %i6, i16 %e7, i32 7
  %e8 = extractelement <32 x i16> %input, i32 8
  %i8 = insertelement <16 x i16> %i7, i16 %e8, i32 8
  %e9 = extractelement <32 x i16> %input, i32 9
  %i9 = insertelement <16 x i16> %i8, i16 %e9, i32 9
  %e10 = extractelement <32 x i16> %input, i32 10
  %i10 = insertelement <16 x i16> %i9, i16 %e10, i32 10
  %e11 = extractelement <32 x i16> %input, i32 11
  %i11 = insertelement <16 x i16> %i10, i16 %e11, i32 11
  %e12 = extractelement <32 x i16> %input, i32 12
  %i12 = insertelement <16 x i16> %i11, i16 %e12, i32 12
  %e13 = extractelement <32 x i16> %input, i32 13
  %i13 = insertelement <16 x i16> %i12, i16 %e13, i32 13
  %e14 = extractelement <32 x i16> %input, i32 14
  %i14 = insertelement <16 x i16> %i13, i16 %e14, i32 14
  %e15 = extractelement <32 x i16> %input, i32 15
  %i15 = insertelement <16 x i16> %i14, i16 %e15, i32 15
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <16 x i16> [ %i15, %.loop ]
  %output = phi <16 x i16> [ %i15, %.loop ]
  store <16 x i16> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v16i16_reverse_order(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v16i16_reverse_order
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <16 x i16> [ zeroinitializer, %.preheader ], [ %i0, %.loop ]
  %input = load <32 x i16>, ptr addrspace(1) %src, align 4
; CHECK:       %e15 = extractelement <32 x i16> %input, i32 15
; CHECK-NEXT:  %i15 = insertelement <16 x i16> poison, i16 %e15, i32 15
; CHECK-NEXT:  %e14 = extractelement <32 x i16> %input, i32 14
; CHECK-NEXT:  %i14 = insertelement <16 x i16> %i15, i16 %e14, i32 14
; CHECK-NEXT:  %e13 = extractelement <32 x i16> %input, i32 13
; CHECK-NEXT:  %i13 = insertelement <16 x i16> %i14, i16 %e13, i32 13
; CHECK-NEXT:  %e12 = extractelement <32 x i16> %input, i32 12
; CHECK-NEXT:  %i12 = insertelement <16 x i16> %i13, i16 %e12, i32 12
; CHECK-NEXT:  %e11 = extractelement <32 x i16> %input, i32 11
; CHECK-NEXT:  %i11 = insertelement <16 x i16> %i12, i16 %e11, i32 11
; CHECK-NEXT:  %e10 = extractelement <32 x i16> %input, i32 10
; CHECK-NEXT:  %i10 = insertelement <16 x i16> %i11, i16 %e10, i32 10
; CHECK-NEXT:  %e9 = extractelement <32 x i16> %input, i32 9
; CHECK-NEXT:  %i9 = insertelement <16 x i16> %i10, i16 %e9, i32 9
; CHECK-NEXT:  %e8 = extractelement <32 x i16> %input, i32 8
; CHECK-NEXT:  %i8 = insertelement <16 x i16> %i9, i16 %e8, i32 8
; CHECK-NEXT:  %e7 = extractelement <32 x i16> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <16 x i16> %i8, i16 %e7, i32 7
; CHECK-NEXT:  %e6 = extractelement <32 x i16> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <16 x i16> %i7, i16 %e6, i32 6
; CHECK-NEXT:  %e5 = extractelement <32 x i16> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <16 x i16> %i6, i16 %e5, i32 5
; CHECK-NEXT:  %e4 = extractelement <32 x i16> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <16 x i16> %i5, i16 %e4, i32 4
; CHECK-NEXT:  %e3 = extractelement <32 x i16> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <16 x i16> %i4, i16 %e3, i32 3
; CHECK-NEXT:  %e2 = extractelement <32 x i16> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <16 x i16> %i3, i16 %e2, i32 2
; CHECK-NEXT:  %e1 = extractelement <32 x i16> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <16 x i16> %i2, i16 %e1, i32 1
; CHECK-NEXT:  %e0 = extractelement <32 x i16> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <16 x i16> %i1, i16 %e0, i32 0
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e15 = extractelement <32 x i16> %input, i32 15
  %i15 = insertelement <16 x i16> %old, i16 %e15, i32 15
  %e14 = extractelement <32 x i16> %input, i32 14
  %i14 = insertelement <16 x i16> %i15, i16 %e14, i32 14
  %e13 = extractelement <32 x i16> %input, i32 13
  %i13 = insertelement <16 x i16> %i14, i16 %e13, i32 13
  %e12 = extractelement <32 x i16> %input, i32 12
  %i12 = insertelement <16 x i16> %i13, i16 %e12, i32 12
  %e11 = extractelement <32 x i16> %input, i32 11
  %i11 = insertelement <16 x i16> %i12, i16 %e11, i32 11
  %e10 = extractelement <32 x i16> %input, i32 10
  %i10 = insertelement <16 x i16> %i11, i16 %e10, i32 10
  %e9 = extractelement <32 x i16> %input, i32 9
  %i9 = insertelement <16 x i16> %i10, i16 %e9, i32 9
  %e8 = extractelement <32 x i16> %input, i32 8
  %i8 = insertelement <16 x i16> %i9, i16 %e8, i32 8
  %e7 = extractelement <32 x i16> %input, i32 7
  %i7 = insertelement <16 x i16> %i8, i16 %e7, i32 7
  %e6 = extractelement <32 x i16> %input, i32 6
  %i6 = insertelement <16 x i16> %i7, i16 %e6, i32 6
  %e5 = extractelement <32 x i16> %input, i32 5
  %i5 = insertelement <16 x i16> %i6, i16 %e5, i32 5
  %e4 = extractelement <32 x i16> %input, i32 4
  %i4 = insertelement <16 x i16> %i5, i16 %e4, i32 4
  %e3 = extractelement <32 x i16> %input, i32 3
  %i3 = insertelement <16 x i16> %i4, i16 %e3, i32 3
  %e2 = extractelement <32 x i16> %input, i32 2
  %i2 = insertelement <16 x i16> %i3, i16 %e2, i32 2
  %e1 = extractelement <32 x i16> %input, i32 1
  %i1 = insertelement <16 x i16> %i2, i16 %e1, i32 1
  %e0 = extractelement <32 x i16> %input, i32 0
  %i0 = insertelement <16 x i16> %i1, i16 %e0, i32 0
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <16 x i16> [ %i0, %.loop ]
  %output = phi <16 x i16> [ %i0, %.loop ]
  store <16 x i16> %output, ptr addrspace(1) %dst, align 4
  ret void
}

;
; i32 tests
;

define spir_kernel void @test_v2i32(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v2i32
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <2 x i32> [ zeroinitializer, %.preheader ], [ %i1, %.loop ]
  %input = load <16 x i32>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <16 x i32> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <2 x i32> poison, i32 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <16 x i32> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <2 x i32> %i0, i32 %e1, i32 1
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <16 x i32> %input, i32 0
  %i0 = insertelement <2 x i32> %old, i32 %e0, i32 0
  %e1 = extractelement <16 x i32> %input, i32 1
  %i1 = insertelement <2 x i32> %i0, i32 %e1, i32 1
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <2 x i32> [ %i1, %.loop ]
  %output = phi <2 x i32> [ %i1, %.loop ]
  store <2 x i32> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v4i32(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v4i32
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <4 x i32> [ zeroinitializer, %.preheader ], [ %i3, %.loop ]
  %input = load <16 x i32>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <16 x i32> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <4 x i32> poison, i32 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <16 x i32> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <4 x i32> %i0, i32 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <16 x i32> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <4 x i32> %i1, i32 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <16 x i32> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <4 x i32> %i2, i32 %e3, i32 3
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <16 x i32> %input, i32 0
  %i0 = insertelement <4 x i32> %old, i32 %e0, i32 0
  %e1 = extractelement <16 x i32> %input, i32 1
  %i1 = insertelement <4 x i32> %i0, i32 %e1, i32 1
  %e2 = extractelement <16 x i32> %input, i32 2
  %i2 = insertelement <4 x i32> %i1, i32 %e2, i32 2
  %e3 = extractelement <16 x i32> %input, i32 3
  %i3 = insertelement <4 x i32> %i2, i32 %e3, i32 3
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <4 x i32> [ %i3, %.loop ]
  %output = phi <4 x i32> [ %i3, %.loop ]
  store <4 x i32> %output, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @test_v8i32(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v8i32
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <8 x i32> [ zeroinitializer, %.preheader ], [ %i7, %.loop ]
  %input = load <16 x i32>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <16 x i32> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <8 x i32> poison, i32 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <16 x i32> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <8 x i32> %i0, i32 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <16 x i32> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <8 x i32> %i1, i32 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <16 x i32> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <8 x i32> %i2, i32 %e3, i32 3
; CHECK-NEXT:  %e4 = extractelement <16 x i32> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <8 x i32> %i3, i32 %e4, i32 4
; CHECK-NEXT:  %e5 = extractelement <16 x i32> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <8 x i32> %i4, i32 %e5, i32 5
; CHECK-NEXT:  %e6 = extractelement <16 x i32> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <8 x i32> %i5, i32 %e6, i32 6
; CHECK-NEXT:  %e7 = extractelement <16 x i32> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <8 x i32> %i6, i32 %e7, i32 7
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
; CHECK-NEXT:  %cmp = icmp slt i32 %index1, 4
; CHECK-NEXT:  br i1 %cmp, label %.loop, label %.exit
  %e0 = extractelement <16 x i32> %input, i32 0
  %i0 = insertelement <8 x i32> %old, i32 %e0, i32 0
  %e1 = extractelement <16 x i32> %input, i32 1
  %i1 = insertelement <8 x i32> %i0, i32 %e1, i32 1
  %e2 = extractelement <16 x i32> %input, i32 2
  %i2 = insertelement <8 x i32> %i1, i32 %e2, i32 2
  %e3 = extractelement <16 x i32> %input, i32 3
  %i3 = insertelement <8 x i32> %i2, i32 %e3, i32 3
  %e4 = extractelement <16 x i32> %input, i32 4
  %i4 = insertelement <8 x i32> %i3, i32 %e4, i32 4
  %e5 = extractelement <16 x i32> %input, i32 5
  %i5 = insertelement <8 x i32> %i4, i32 %e5, i32 5
  %e6 = extractelement <16 x i32> %input, i32 6
  %i6 = insertelement <8 x i32> %i5, i32 %e6, i32 6
  %e7 = extractelement <16 x i32> %input, i32 7
  %i7 = insertelement <8 x i32> %i6, i32 %e7, i32 7
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
; CHECK:  %output = phi <8 x i32> [ %i7, %.loop ]
  %output = phi <8 x i32> [ %i7, %.loop ]
  store <8 x i32> %output, ptr addrspace(1) %dst, align 4
  ret void
}

;
; non-opt cases
;

; Not all indices are inserted, can't optimize.
define spir_kernel void @test_v16i16_incomplete(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v16i16_incomplete
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <16 x i16> [ zeroinitializer, %.preheader ], [ %i7, %.loop ]
  %input = load <32 x i16>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <32 x i16> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <16 x i16> %old, i16 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <32 x i16> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <16 x i16> %i0, i16 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <32 x i16> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <16 x i16> %i1, i16 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <32 x i16> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <16 x i16> %i2, i16 %e3, i32 3
; CHECK-NEXT:  %e4 = extractelement <32 x i16> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <16 x i16> %i3, i16 %e4, i32 4
; CHECK-NEXT:  %e5 = extractelement <32 x i16> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <16 x i16> %i4, i16 %e5, i32 5
; CHECK-NEXT:  %e6 = extractelement <32 x i16> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <16 x i16> %i5, i16 %e6, i32 6
; CHECK-NEXT:  %e7 = extractelement <32 x i16> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <16 x i16> %i6, i16 %e7, i32 7
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
  %e0 = extractelement <32 x i16> %input, i32 0
  %i0 = insertelement <16 x i16> %old, i16 %e0, i32 0
  %e1 = extractelement <32 x i16> %input, i32 1
  %i1 = insertelement <16 x i16> %i0, i16 %e1, i32 1
  %e2 = extractelement <32 x i16> %input, i32 2
  %i2 = insertelement <16 x i16> %i1, i16 %e2, i32 2
  %e3 = extractelement <32 x i16> %input, i32 3
  %i3 = insertelement <16 x i16> %i2, i16 %e3, i32 3
  %e4 = extractelement <32 x i16> %input, i32 4
  %i4 = insertelement <16 x i16> %i3, i16 %e4, i32 4
  %e5 = extractelement <32 x i16> %input, i32 5
  %i5 = insertelement <16 x i16> %i4, i16 %e5, i32 5
  %e6 = extractelement <32 x i16> %input, i32 6
  %i6 = insertelement <16 x i16> %i5, i16 %e6, i32 6
  %e7 = extractelement <32 x i16> %input, i32 7
  %i7 = insertelement <16 x i16> %i6, i16 %e7, i32 7
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
  %output = phi <16 x i16> [ %i7, %.loop ]
  store <16 x i16> %output, ptr addrspace(1) %dst, align 4
  ret void
}

; Insertelement at index 7 has other use, can't optimize.
define spir_kernel void @test_v16i16_insert_other_use(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v16i16_insert_other_use
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <16 x i16> [ zeroinitializer, %.preheader ], [ %i7, %.loop ]
  %input = load <32 x i16>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <32 x i16> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <16 x i16> %old, i16 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <32 x i16> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <16 x i16> %i0, i16 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <32 x i16> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <16 x i16> %i1, i16 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <32 x i16> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <16 x i16> %i2, i16 %e3, i32 3
; CHECK-NEXT:  %e4 = extractelement <32 x i16> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <16 x i16> %i3, i16 %e4, i32 4
; CHECK-NEXT:  %e5 = extractelement <32 x i16> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <16 x i16> %i4, i16 %e5, i32 5
; CHECK-NEXT:  %e6 = extractelement <32 x i16> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <16 x i16> %i5, i16 %e6, i32 6
; CHECK-NEXT:  %e7 = extractelement <32 x i16> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <16 x i16> %i6, i16 %e7, i32 7
; CHECK-NEXT:  %e8 = extractelement <32 x i16> %input, i32 8
; CHECK-NEXT:  %i8 = insertelement <16 x i16> %i7, i16 %e8, i32 8
; CHECK-NEXT:  %e9 = extractelement <32 x i16> %input, i32 9
; CHECK-NEXT:  %i9 = insertelement <16 x i16> %i8, i16 %e9, i32 9
; CHECK-NEXT:  %e10 = extractelement <32 x i16> %input, i32 10
; CHECK-NEXT:  %i10 = insertelement <16 x i16> %i9, i16 %e10, i32 10
; CHECK-NEXT:  %e11 = extractelement <32 x i16> %input, i32 11
; CHECK-NEXT:  %i11 = insertelement <16 x i16> %i10, i16 %e11, i32 11
; CHECK-NEXT:  %e12 = extractelement <32 x i16> %input, i32 12
; CHECK-NEXT:  %i12 = insertelement <16 x i16> %i11, i16 %e12, i32 12
; CHECK-NEXT:  %e13 = extractelement <32 x i16> %input, i32 13
; CHECK-NEXT:  %i13 = insertelement <16 x i16> %i12, i16 %e13, i32 13
; CHECK-NEXT:  %e14 = extractelement <32 x i16> %input, i32 14
; CHECK-NEXT:  %i14 = insertelement <16 x i16> %i13, i16 %e14, i32 14
; CHECK-NEXT:  %e15 = extractelement <32 x i16> %input, i32 15
; CHECK-NEXT:  %i15 = insertelement <16 x i16> %i14, i16 %e15, i32 15
  %e0 = extractelement <32 x i16> %input, i32 0
  %i0 = insertelement <16 x i16> %old, i16 %e0, i32 0
  %e1 = extractelement <32 x i16> %input, i32 1
  %i1 = insertelement <16 x i16> %i0, i16 %e1, i32 1
  %e2 = extractelement <32 x i16> %input, i32 2
  %i2 = insertelement <16 x i16> %i1, i16 %e2, i32 2
  %e3 = extractelement <32 x i16> %input, i32 3
  %i3 = insertelement <16 x i16> %i2, i16 %e3, i32 3
  %e4 = extractelement <32 x i16> %input, i32 4
  %i4 = insertelement <16 x i16> %i3, i16 %e4, i32 4
  %e5 = extractelement <32 x i16> %input, i32 5
  %i5 = insertelement <16 x i16> %i4, i16 %e5, i32 5
  %e6 = extractelement <32 x i16> %input, i32 6
  %i6 = insertelement <16 x i16> %i5, i16 %e6, i32 6
  %e7 = extractelement <32 x i16> %input, i32 7
  %i7 = insertelement <16 x i16> %i6, i16 %e7, i32 7
  %e8 = extractelement <32 x i16> %input, i32 8
  %i8 = insertelement <16 x i16> %i7, i16 %e8, i32 8
  %e9 = extractelement <32 x i16> %input, i32 9
  %i9 = insertelement <16 x i16> %i8, i16 %e9, i32 9
  %e10 = extractelement <32 x i16> %input, i32 10
  %i10 = insertelement <16 x i16> %i9, i16 %e10, i32 10
  %e11 = extractelement <32 x i16> %input, i32 11
  %i11 = insertelement <16 x i16> %i10, i16 %e11, i32 11
  %e12 = extractelement <32 x i16> %input, i32 12
  %i12 = insertelement <16 x i16> %i11, i16 %e12, i32 12
  %e13 = extractelement <32 x i16> %input, i32 13
  %i13 = insertelement <16 x i16> %i12, i16 %e13, i32 13
  %e14 = extractelement <32 x i16> %input, i32 14
  %i14 = insertelement <16 x i16> %i13, i16 %e14, i32 14
  %e15 = extractelement <32 x i16> %input, i32 15
  %i15 = insertelement <16 x i16> %i14, i16 %e15, i32 15
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
  %output = phi <16 x i16> [ %i15, %.loop ]
  store <16 x i16> %output, ptr addrspace(1) %dst, align 4
  ret void
}

; Unsupported type, don't optimize.
define spir_kernel void @test_v12i16_invalid_type(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: @test_v12i16_invalid_type
 br label %.preheader

.preheader:
  br label %.loop

.loop:
  %index = phi i32 [ 0, %.preheader ], [ %index1, %.loop ]
  %old = phi <12 x i16> [ zeroinitializer, %.preheader ], [ %i11, %.loop ]
  %input = load <32 x i16>, ptr addrspace(1) %src, align 4
; CHECK:       %e0 = extractelement <32 x i16> %input, i32 0
; CHECK-NEXT:  %i0 = insertelement <12 x i16> %old, i16 %e0, i32 0
; CHECK-NEXT:  %e1 = extractelement <32 x i16> %input, i32 1
; CHECK-NEXT:  %i1 = insertelement <12 x i16> %i0, i16 %e1, i32 1
; CHECK-NEXT:  %e2 = extractelement <32 x i16> %input, i32 2
; CHECK-NEXT:  %i2 = insertelement <12 x i16> %i1, i16 %e2, i32 2
; CHECK-NEXT:  %e3 = extractelement <32 x i16> %input, i32 3
; CHECK-NEXT:  %i3 = insertelement <12 x i16> %i2, i16 %e3, i32 3
; CHECK-NEXT:  %e4 = extractelement <32 x i16> %input, i32 4
; CHECK-NEXT:  %i4 = insertelement <12 x i16> %i3, i16 %e4, i32 4
; CHECK-NEXT:  %e5 = extractelement <32 x i16> %input, i32 5
; CHECK-NEXT:  %i5 = insertelement <12 x i16> %i4, i16 %e5, i32 5
; CHECK-NEXT:  %e6 = extractelement <32 x i16> %input, i32 6
; CHECK-NEXT:  %i6 = insertelement <12 x i16> %i5, i16 %e6, i32 6
; CHECK-NEXT:  %e7 = extractelement <32 x i16> %input, i32 7
; CHECK-NEXT:  %i7 = insertelement <12 x i16> %i6, i16 %e7, i32 7
; CHECK-NEXT:  %e8 = extractelement <32 x i16> %input, i32 8
; CHECK-NEXT:  %i8 = insertelement <12 x i16> %i7, i16 %e8, i32 8
; CHECK-NEXT:  %e9 = extractelement <32 x i16> %input, i32 9
; CHECK-NEXT:  %i9 = insertelement <12 x i16> %i8, i16 %e9, i32 9
; CHECK-NEXT:  %e10 = extractelement <32 x i16> %input, i32 10
; CHECK-NEXT:  %i10 = insertelement <12 x i16> %i9, i16 %e10, i32 10
; CHECK-NEXT:  %e11 = extractelement <32 x i16> %input, i32 11
; CHECK-NEXT:  %i11 = insertelement <12 x i16> %i10, i16 %e11, i32 11
; CHECK-NEXT:  %index1 = add nuw nsw i32 %index, 1
  %e0 = extractelement <32 x i16> %input, i32 0
  %i0 = insertelement <12 x i16> %old, i16 %e0, i32 0
  %e1 = extractelement <32 x i16> %input, i32 1
  %i1 = insertelement <12 x i16> %i0, i16 %e1, i32 1
  %e2 = extractelement <32 x i16> %input, i32 2
  %i2 = insertelement <12 x i16> %i1, i16 %e2, i32 2
  %e3 = extractelement <32 x i16> %input, i32 3
  %i3 = insertelement <12 x i16> %i2, i16 %e3, i32 3
  %e4 = extractelement <32 x i16> %input, i32 4
  %i4 = insertelement <12 x i16> %i3, i16 %e4, i32 4
  %e5 = extractelement <32 x i16> %input, i32 5
  %i5 = insertelement <12 x i16> %i4, i16 %e5, i32 5
  %e6 = extractelement <32 x i16> %input, i32 6
  %i6 = insertelement <12 x i16> %i5, i16 %e6, i32 6
  %e7 = extractelement <32 x i16> %input, i32 7
  %i7 = insertelement <12 x i16> %i6, i16 %e7, i32 7
  %e8 = extractelement <32 x i16> %input, i32 8
  %i8 = insertelement <12 x i16> %i7, i16 %e8, i32 8
  %e9 = extractelement <32 x i16> %input, i32 9
  %i9 = insertelement <12 x i16> %i8, i16 %e9, i32 9
  %e10 = extractelement <32 x i16> %input, i32 10
  %i10 = insertelement <12 x i16> %i9, i16 %e10, i32 10
  %e11 = extractelement <32 x i16> %input, i32 11
  %i11 = insertelement <12 x i16> %i10, i16 %e11, i32 11
  %index1 = add nuw nsw i32 %index, 1
  %cmp = icmp slt i32 %index1, 4
  br i1 %cmp, label %.loop, label %.exit

.exit:
  %output = phi <12 x i16> [ %i11, %.loop ]
  store <12 x i16> %output, ptr addrspace(1) %dst, align 4
  ret void
}
