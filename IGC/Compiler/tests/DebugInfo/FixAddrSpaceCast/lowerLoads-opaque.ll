;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt  --opaque-pointers --igc-addrspacecast-fix -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll
; RUN: FileCheck %s --input-file=%t.ll --check-prefix=REMOVED
; ------------------------------------------------
; FixAddrSpaceCast
; ------------------------------------------------
; This test checks that loads are correctly lowered from blocks with phi nodes into incoming blocks,
; to avoid extra address space casts.
; ------------------------------------------------

; CHECK-LABEL: cond.true3442:
; CHECK: [[T0:%.*]] = load i32, ptr addrspace(4) %gep.res
; CHECK: br label %cond.end3465

; CHECK-LABEL: cond.false3459
; CHECK:  [[T1:%.*]] = load i32, ptr addrspace(3) %arrayidx3464
; CHECK:  br label %cond.end3465

; CHECK-LABEL: cond.end3465:
; CHECK: [[T3:%.*]] = phi i32 [ [[T0]], %cond.true3442 ], [ [[T1]], %cond.false3459 ]
; CHECK: icmp sgt i32 [[T3]], -1

; REMOVED-LABEL: cond.false3459
; REMOVED-NOT: addrspacecast

define i1 @testFn(ptr addrspace(3) %arrayidx3464, ptr addrspace(1) %globalArgs, i64 %idx.ext.i12415) #0 {

entry:
  %test = icmp sgt i64 %idx.ext.i12415, -1
  br i1 %test, label %cond.true3442, label %cond.false3459

cond.true3442:
  %global.ptr = load ptr addrspace(4), ptr addrspace(1) %globalArgs, align 16
  %gep.res = getelementptr inbounds i8, ptr addrspace(4) %global.ptr, i64 %idx.ext.i12415
  br label %cond.end3465

cond.false3459:
  %addr.res = addrspacecast ptr addrspace(3) %arrayidx3464 to ptr addrspace(4)
  br label %cond.end3465

cond.end3465:
  %cond3466.in = phi ptr addrspace(4) [ %gep.res, %cond.true3442 ], [ %addr.res, %cond.false3459 ]
  %cond3466 = load i32, ptr addrspace(4) %cond3466.in, align 4
  %test2 = icmp sgt i32 %cond3466, -1
  br i1 %test2, label %while.cond3471.preheader.exitStub, label %if.end3568.exitStub

while.cond3471.preheader.exitStub:
  ret i1 true

if.end3568.exitStub:
  ret i1 false
}

attributes #0 = { noinline nounwind }