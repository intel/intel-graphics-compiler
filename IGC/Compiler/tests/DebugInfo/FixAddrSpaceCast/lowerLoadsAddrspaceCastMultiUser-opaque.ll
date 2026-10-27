;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-addrspacecast-fix -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll
; RUN: FileCheck %s --input-file=%t.ll --check-prefix=NOT-REMOVED
; ------------------------------------------------
; FixAddrSpaceCast
; ------------------------------------------------
; Test checks if in case that addrspace cast has multiple users, it is not removed
; during when lowering loads from phi nodes into incoming blocks.
; ------------------------------------------------

; CHECK-LABEL: cond.true3442:
; CHECK: [[T2:%.*]] = load i32, ptr addrspace(4) %gep.res, align 4
; CHECK: br label %cond.end3465

; CHECK-LABEL: cond.false3459
; CHECK:  [[T4:%.*]] = load i32, ptr addrspace(3) %arrayidx3464, align 4
; CHECK:  br label %cond.end3465

; CHECK-LABEL: cond.end3465:
; CHECK: [[T5:%.*]] = phi i32 [ [[T2]], %cond.true3442 ], [ [[T4]], %cond.false3459 ]
; CHECK: icmp sgt i32 [[T5]], -1

; NOT-REMOVED-LABEL: cond.false3459
; NOT-REMOVED: addrspacecast

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
  store i8 0, ptr addrspace(4) %addr.res, align 8
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
