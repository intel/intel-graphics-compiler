;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-addrspacecast-fix -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll
; RUN: FileCheck %s --input-file=%t.ll --check-prefix=REMOVED
; ------------------------------------------------
; FixAddrSpaceCast
; ------------------------------------------------
; Test checks if, in case that incoming Phi value has different parent then incoming block, it is correctly
; handled and new Phi is created correctly.
; ------------------------------------------------

; CHECK-LABEL: cond.true3442:
; CHECK: [[T1:%.*]] = bitcast i8 addrspace(4)* %gep.res to i32 addrspace(4)*
; CHECK: [[T2:%.*]] = load i32, i32 addrspace(4)* [[T1]], align 4
; CHECK: br label %cond.end3465

; CHECK-LABEL: cond.false3459
; CHECK:  [[T3:%.*]] = bitcast i8 addrspace(3)* %arrayidx3464 to i32 addrspace(3)*
; CHECK:  [[T4:%.*]] = load i32, i32 addrspace(3)* [[T3]], align 4
; CHECK:  br label %cond.between

; CHECK-LABEL: cond.end3465:
; CHECK: [[T5:%.*]] = phi i32 [ [[T2]], %cond.true3442 ], [ [[T4]], %cond.between ]
; CHECK: icmp sgt i32 [[T5]], -1

; REMOVED-LABEL: cond.false3459
; REMOVED-NOT: addrspacecast

define i1 @testFn(i8 addrspace(3)* %arrayidx3464, i8 addrspace(1)* %globalArgs, i64 %idx.ext.i12415) #0 {

entry:
  %test = icmp sgt i64 %idx.ext.i12415, -1
  br i1 %test, label %cond.true3442, label %cond.false3459

cond.true3442:
  %arrayidx3450 = getelementptr inbounds i8, i8 addrspace(1)* %globalArgs, i64 896
  %tmp1 = bitcast i8 addrspace(1)* %arrayidx3450 to i8 addrspace(4)* addrspace(1)*
  %tmp2 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(1)* %tmp1, align 16
  %gep.res = getelementptr inbounds i8, i8 addrspace(4)* %tmp2, i64 %idx.ext.i12415
  br label %cond.end3465

cond.false3459:
  %addr.res = addrspacecast i8 addrspace(3)* %arrayidx3464 to i8 addrspace(4)*
  br label %cond.between

cond.between:
  br label %cond.end3465

cond.end3465:
  %cond3466.in = phi i8 addrspace(4)* [ %gep.res, %cond.true3442 ], [ %addr.res, %cond.between ]
  %cast1 = bitcast i8 addrspace(4)* %cond3466.in to i32 addrspace(4)*
  %cond3466 = load i32, i32 addrspace(4)* %cast1, align 4
  %test2 = icmp sgt i32 %cond3466, -1
  br i1 %test2, label %while.cond3471.preheader.exitStub, label %if.end3568.exitStub

while.cond3471.preheader.exitStub:
  ret i1 true

if.end3568.exitStub:
  ret i1 false
}

attributes #0 = { noinline nounwind }