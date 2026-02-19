;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-promote-constant-structs | FileCheck %s


%struct.S = type { i64, i64, i64 }

; Function Attrs: convergent nounwind
define void @f0() {
  %x = alloca %struct.S, align 8
  %y = alloca %struct.S*, align 8
  %xe0 = getelementptr inbounds %struct.S, %struct.S* %x, i64 0, i32 0
  store i64 0, i64* %xe0, align 8
  store %struct.S* %x, %struct.S** %y, align 8
  %z = load i64, i64* %xe0, align 8
  %1 = icmp eq i64 %z, 0
  ret void
}

; CHECK-LABEL:  define void @f0
; CHECK:        %x = alloca %struct.S, align 8
; CHECK:        %y = alloca %struct.S*, align 8
; CHECK:        %xe0 = getelementptr inbounds %struct.S, %struct.S* %x, i64 0, i32 0
; CHECK:        store i64 0, i64* %xe0, align 8
; CHECK:        store %struct.S* %x, %struct.S** %y, align 8
; CHECK:        %z = load i64, i64* %xe0, align 8
; CHECK:        %1 = icmp eq i64 %z, 0
; CHECK-NOT:    %1 = icmp eq i64 0, 0
; CHECK:        ret void


; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8 * nocapture, i8 * nocapture, i64, i1) #0

attributes #0 = { nounwind }
