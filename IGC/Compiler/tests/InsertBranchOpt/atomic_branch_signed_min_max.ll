;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -igc-InsertBranchOpt -regkey EnableAtomicBranch=3072 -S %s | FileCheck %s
;
; EnableAtomicBranch=3072 enables SignMin (0x400) and SignMax (0x800).

; CHECK-LABEL: @atomicIMin(
; CHECK: [[SRC:%.*]] = add i32
; CHECK: [[READ:%.*]] = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed
; CHECK: [[READ_I32:%.*]] = bitcast float {{%.*}} to i32
; CHECK: [[CMP:%.*]] = icmp slt i32 [[SRC]], [[READ_I32]]
; CHECK: br i1 [[CMP]], label %[[ATOMIC_IF_TRUE:.*]], label %[[ATOMIC_IF_END:.*]]
; CHECK: [[ATOMIC_IF_TRUE]]:
; CHECK: call i32 @llvm.genx.GenISA.intatomicraw{{.*}}i32 11)
define i32 @atomicIMin(i32 %address, i32 %value) {
  %src = add i32 %value, 1
  %atomic = call i32 @llvm.genx.GenISA.intatomicraw.i32.p131073(ptr addrspace(131073) null, i32 %address, i32 %src, i32 11)
  ret i32 %atomic
}

; CHECK-LABEL: @atomicIMax(
; CHECK: [[SRC:%.*]] = add i32
; CHECK: [[CMP:%.*]] = icmp sgt i32 [[SRC]], {{%.*}}
; CHECK: br i1 [[CMP]], label %[[ATOMIC_IF_TRUE:.*]], label %[[ATOMIC_IF_END:.*]]
; CHECK: [[ATOMIC_IF_TRUE]]:
; CHECK: call i32 @llvm.genx.GenISA.intatomicraw{{.*}}i32 12)
define i32 @atomicIMax(i32 %address, i32 %value) {
  %src = add i32 %value, 1
  %atomic = call i32 @llvm.genx.GenISA.intatomicraw.i32.p131073(ptr addrspace(131073) null, i32 %address, i32 %src, i32 12)
  ret i32 %atomic
}

declare i32 @llvm.genx.GenISA.intatomicraw.i32.p131073(ptr addrspace(131073), i32, i32, i32)