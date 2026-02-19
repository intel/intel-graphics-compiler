;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-promote-constant-structs | FileCheck %s


%struct.S = type { i64, i64, i64 }

; Function Attrs: convergent nounwind
define void @f0(i32 %src1) {
  %s1 = alloca %struct.S, align 8
  %s2 = alloca %struct.S, align 8
  %s1e0 = getelementptr inbounds %struct.S, %struct.S* %s1, i64 0, i32 0
  store i64 0, i64* %s1e0, align 8
  %s1e2 = getelementptr inbounds %struct.S, %struct.S* %s1, i64 0, i32 2
  store i64 0, i64* %s1e2, align 8
  %s1e1 = getelementptr inbounds %struct.S, %struct.S* %s1, i64 0, i32 1
  store i64 0, i64* %s1e1, align 8
  %1 = icmp eq i32 %src1, 0
  br i1 %1, label %bb1, label %bb2

bb1: ; preds = %0
  store i64 1, i64* %s1e1, align 8
  br label %bb2

bb2: ; preds = %0
  %2 = load i64, i64* %s1e2, align 8
  %3 = load i64, i64* %s1e2, align 8
  %4 = load i64, i64* %s1e1, align 8
  br label %bb3

bb3: ; preds = %bb2, %bb3
  %5 = phi i64 [ 4, %bb2 ], [ %7, %bb3 ]
  %6 = load i64, i64* %s1e2, align 8
  %7 = sub i64 %5, 1
  store i64 %7, i64* %s1e2, align 8
  %8 = load i64, i64* %s1e2, align 8
  %9 = icmp eq i64 %7, 0
  br i1 %9, label %bb3, label %bb4

bb4:
  %10 = bitcast %struct.S* %s1 to i8*
  %11 = bitcast %struct.S* %s2 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %10, i8* %11, i64 8, i1 false)
  %12 = load i64, i64* %s1e0, align 8
  %13 = icmp eq i64 %12, 0
  ret void
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8 * nocapture, i8 * nocapture, i64, i1) #0

attributes #0 = { nounwind }

 ; CHECK-LABEL: define void @f0
 ; CHECK:  %s1 = alloca %struct.S, align 8
 ; CHECK:  %s2 = alloca %struct.S, align 8
 ; CHECK:  %s1e0 = getelementptr inbounds %struct.S, %struct.S* %s1, i64 0, i32 0
 ; CHECK:  store i64 0, i64* %s1e0, align 8
 ; CHECK:  %s1e2 = getelementptr inbounds %struct.S, %struct.S* %s1, i64 0, i32 2
 ; CHECK:  store i64 0, i64* %s1e2, align 8
 ; CHECK:  %s1e1 = getelementptr inbounds %struct.S, %struct.S* %s1, i64 0, i32 1
 ; CHECK:  store i64 0, i64* %s1e1, align 8
 ; CHECK:  %1 = icmp eq i32 %src1, 0
 ; CHECK:  br i1 %1, label %bb1, label %bb2

 ; CHECK:  store i64 1, i64* %s1e1, align 8
 ; CHECK:  br label %bb2

 ; CHECK:  %2 = load i64, i64* %s1e1, align 8
 ; CHECK:  br label %bb3

 ; CHECK:  %3 = phi i64 [ 4, %bb2 ], [ %4, %bb3 ]
 ; CHECK:  %4 = sub i64 %3, 1
 ; CHECK:  store i64 %4, i64* %s1e2, align 8
 ; CHECK:  %5 = load i64, i64* %s1e2, align 8
 ; CHECK:  %6 = icmp eq i64 %4, 0
 ; CHECK:  br i1 %6, label %bb3, label %bb4

 ; CHECK:  %7 = bitcast %struct.S* %s1 to i8*
 ; CHECK:  %8 = bitcast %struct.S* %s2 to i8*
 ; CHECK:  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %7, i8* %8, i64 8, i1 false)
 ; CHECK:  %9 = load i64, i64* %s1e0, align 8
 ; CHECK:  %10 = icmp eq i64 %9, 0
 ; CHECK:  ret void
