;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - -igc-promote-constant-structs | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - -igc-promote-constant-structs | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}


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
; CHECK-TYPED:        %y = alloca %struct.S*, align 8
; CHECK-OPAQUE:        %y = alloca ptr, align 8
; CHECK-TYPED:        %xe0 = getelementptr inbounds %struct.S, %struct.S* %x, i64 0, i32 0
; CHECK-OPAQUE:        %xe0 = getelementptr inbounds %struct.S, ptr %x, i64 0, i32 0
; CHECK-TYPED:        store i64 0, i64* %xe0, align 8
; CHECK-OPAQUE:        store i64 0, ptr %xe0, align 8
; CHECK-TYPED:        store %struct.S* %x, %struct.S** %y, align 8
; CHECK-OPAQUE:        store ptr %x, ptr %y, align 8
; CHECK-TYPED:        %z = load i64, i64* %xe0, align 8
; CHECK-OPAQUE:        %z = load i64, ptr %xe0, align 8
; CHECK:        %1 = icmp eq i64 %z, 0
; CHECK-NOT:    %1 = icmp eq i64 0, 0
; CHECK:        ret void


; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8 * nocapture, i8 * nocapture, i64, i1) #0

attributes #0 = { nounwind }
