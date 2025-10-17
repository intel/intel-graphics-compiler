;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

; For opaque pointers we need to be able to deduce the type from more complex type like e.g. struct
; and we can find it by investigating GEP instruction

; CHECK: [[GEP0:%.*]] = getelementptr <16 x i16>, ptr addrspace(1) [[GEP:%.*]], i32 0
; CHECK: store <16 x i16> zeroinitializer, ptr addrspace(1) [[GEP0]], align 16
; CHECK: [[GEP1:%.*]] = getelementptr <16 x i16>, ptr addrspace(1) [[GEP]], i32 1
; CHECK: store <16 x i16> zeroinitializer, ptr addrspace(1) [[GEP1]], align 16
; CHECK: [[GEP2:%.*]] = getelementptr <16 x i16>, ptr addrspace(1) [[GEP]], i32 2
; CHECK: store <16 x i16> zeroinitializer, ptr addrspace(1) [[GEP2]], align 16
; CHECK: [[GEP3:%.*]] = getelementptr <16 x i16>, ptr addrspace(1) [[GEP]], i32 3
; CHECK: store <16 x i16> zeroinitializer, ptr addrspace(1) [[GEP3]], align 16

%"class.sycl::_V1::vec.1284" = type { %"class.sycl::_V1::detail::vec_base.1320" }
%"class.sycl::_V1::detail::vec_base.1320" = type { [8 x i16] }

define void @_foo(ptr addrspace(1) align 16 %0, ptr addrspace(3) align 16 %1, ptr addrspace(1) align 1 %2,<8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %globalSize, <3 x i32> %localSize, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY) #0 {
  %4 = extractelement <3 x i32> %localSize, i64 1
  %5 = extractelement <3 x i32> %globalSize, i64 0
  %6 = extractelement <3 x i32> %globalOffset, i64 0
  %7 = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %8 = extractelement <3 x i32> %enqueuedLocalSize, i64 1
  %9 = extractelement <8 x i32> %r0, i64 1
  %10 = extractelement <8 x i32> %r0, i64 6
  %11 = zext i32 %10 to i64
  %12 = zext i32 %8 to i64
  %13 = mul nuw i64 %12, %11
  %14 = zext i16 %localIdY to i64
  %15 = add nuw i64 %13, %14
  %16 = zext i32 %9 to i64
  %17 = zext i32 %7 to i64
  %18 = mul nuw i64 %17, %16
  %19 = zext i16 %localIdX to i64
  %20 = add nuw i64 %18, %19
  %21 = zext i32 %6 to i64
  %22 = add nuw i64 %20, %21
  %23 = zext i32 %5 to i64
  %24 = mul i64 %15, %23
  %25 = add i64 %24, %22
  %26 = sub i64 %25, %21
  %27 = mul i64 %26, 10
  %28 = getelementptr inbounds %"class.sycl::_V1::vec.1284", ptr addrspace(1) %0, i64 %27
  call void @llvm.memset.p1.i64(ptr addrspace(1) align 16 %28, i8 0, i64 160, i1 false)
  ret void
}

declare void @llvm.memset.p1.i64(ptr addrspace(1) nocapture, i8, i64, i1) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind }
