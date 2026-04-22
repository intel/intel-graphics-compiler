;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-gas-resolve --opaque-pointers -S < %s | FileCheck %s

; Verify that GAS resolution propagates through GenISA.PredicatedLoad and
; GenISA.PredicatedStore, replacing addrspace(4) with the concrete address space.

define i16 @predicated_load_i16_global(ptr addrspace(1) %src, i1 %pred, i16 %passthru) {
; CHECK-LABEL: define i16 @predicated_load_i16_global(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]], i1 [[PRED:%.*]], i16 [[PASSTHRU:%.*]]) {
; CHECK-NEXT:    [[RES:%.*]] = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) [[SRC]], i64 1, i1 [[PRED]], i16 [[PASSTHRU]])
; CHECK-NEXT:    ret i16 [[RES]]
;
  %generic = addrspacecast ptr addrspace(1) %src to ptr addrspace(4)
  %res = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p4.i16(ptr addrspace(4) %generic, i64 1, i1 %pred, i16 %passthru)
  ret i16 %res
}

define i8 @predicated_load_i8_global(ptr addrspace(1) %src, i1 %pred, i8 %passthru) {
; CHECK-LABEL: define i8 @predicated_load_i8_global(
; CHECK-SAME: ptr addrspace(1) [[SRC:%.*]], i1 [[PRED:%.*]], i8 [[PASSTHRU:%.*]]) {
; CHECK-NEXT:    [[RES:%.*]] = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) [[SRC]], i64 1, i1 [[PRED]], i8 [[PASSTHRU]])
; CHECK-NEXT:    ret i8 [[RES]]
;
  %generic = addrspacecast ptr addrspace(1) %src to ptr addrspace(4)
  %res = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p4.i8(ptr addrspace(4) %generic, i64 1, i1 %pred, i8 %passthru)
  ret i8 %res
}

define void @predicated_store_i32_global(ptr addrspace(1) %dst, i32 %val, i1 %pred) {
; CHECK-LABEL: define void @predicated_store_i32_global(
; CHECK-SAME: ptr addrspace(1) [[DST:%.*]], i32 [[VAL:%.*]], i1 [[PRED:%.*]]) {
; CHECK-NEXT:    call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) [[DST]], i32 [[VAL]], i64 1, i1 [[PRED]])
; CHECK-NEXT:    ret void
;
  %generic = addrspacecast ptr addrspace(1) %dst to ptr addrspace(4)
  call void @llvm.genx.GenISA.PredicatedStore.p4.i32(ptr addrspace(4) %generic, i32 %val, i64 1, i1 %pred)
  ret void
}

define i32 @predicated_load_i32_local(ptr addrspace(3) %src, i1 %pred, i32 %passthru) {
; CHECK-LABEL: define i32 @predicated_load_i32_local(
; CHECK-SAME: ptr addrspace(3) [[SRC:%.*]], i1 [[PRED:%.*]], i32 [[PASSTHRU:%.*]]) {
; CHECK-NEXT:    [[RES:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p3.i32(ptr addrspace(3) [[SRC]], i64 1, i1 [[PRED]], i32 [[PASSTHRU]])
; CHECK-NEXT:    ret i32 [[RES]]
;
  %generic = addrspacecast ptr addrspace(3) %src to ptr addrspace(4)
  %res = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p4.i32(ptr addrspace(4) %generic, i64 1, i1 %pred, i32 %passthru)
  ret i32 %res
}

declare i16 @llvm.genx.GenISA.PredicatedLoad.i16.p4.i16(ptr addrspace(4), i64, i1, i16)
declare i8 @llvm.genx.GenISA.PredicatedLoad.i8.p4.i8(ptr addrspace(4), i64, i1, i8)
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p4.i32(ptr addrspace(4), i64, i1, i32)
declare void @llvm.genx.GenISA.PredicatedStore.p4.i32(ptr addrspace(4), i32, i64, i1)

!igc.functions = !{!0, !3, !4, !5}
!0 = !{ptr @predicated_load_i16_global, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @predicated_load_i8_global, !1}
!4 = !{ptr @predicated_store_i32_global, !1}
!5 = !{ptr @predicated_load_i32_local, !1}
