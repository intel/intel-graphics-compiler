;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-sub-byte -S %s -o - | FileCheck %s

; Check icmp result is promoted to i8 for icmp user that is promoted to i8, i.e.
; the second element of type %structtype.
; Also check icmp result isn't promoted to i8 for icmp user that is condition
; of select or br instruction.

%structtype = type { i32, i1 }

define spir_kernel void @__omp_offloading_811_21536b6__ZN6openmc31process_surface_crossing_eventsEv_l323() {
  %cmpxchg.success = icmp eq i32 0, 0
  %1 = insertvalue %structtype zeroinitializer, i1 %cmpxchg.success, 1
  %cmp2.i1679.i = extractvalue %structtype %1, 1
  %2 = select i1 %cmpxchg.success, i32 0, i32 1
  %3 = select i1 %cmpxchg.success, i1 false, i1 true
  br i1 %cmpxchg.success, label %s, label %exit2

s:
  %4 = insertvalue %structtype zeroinitializer, i32 %2, 0
  %5 = insertvalue %structtype zeroinitializer, i1 %3, 1
  %not.cmp2.i1679.i = icmp ne i1 %cmp2.i1679.i, false
  br i1 %not.cmp2.i1679.i, label %exit1, label %exit2

exit1:
  br label %exit2

exit2:
  ret void
}

; CHECK:      [[CMP1:%[0-9]+]] = icmp eq i32 0, 0
; CHECK-NEXT: [[EXT1:%[0-9]+]] = zext i1 [[CMP1]] to i8
; CHECK-NEXT: [[INSERT:%[0-9]+]] = insertvalue %structtype zeroinitializer, i8 [[EXT1]], 1
; CHECK-NEXT: [[EXTRACT:%[0-9]+]] = extractvalue %structtype [[INSERT]], 1
; CHECK-NEXT: [[SELECT1:%[0-9]+]] = select i1 [[CMP1]], i32 0, i32 1
; CHECK-NEXT: [[SELECT2:%[0-9]+]] = select i1 [[CMP1]], i1 false, i1 true
; CHECK-NEXT: [[EXT2:%[0-9]+]] = zext i1 [[SELECT2]] to i8
; CHECK-NEXT: br i1 [[CMP1]], label %s, label %exit2

; CHECK:      s:
; CHECK-NEXT: insertvalue %structtype zeroinitializer, i32 [[SELECT1]], 0
; CHECK-NEXT: insertvalue %structtype zeroinitializer, i8 [[EXT2]], 1
; CHECK-NEXT: [[CMP2:%[0-9]+]] = icmp ne i8 [[EXTRACT]], 0
; CHECK-NEXT: br i1 [[CMP2]], label %exit1, label %exit2
