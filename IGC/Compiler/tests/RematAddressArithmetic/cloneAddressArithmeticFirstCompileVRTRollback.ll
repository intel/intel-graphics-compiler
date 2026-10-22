;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: system-windows
; On CRI a kernel whose pressure is above the largest GRF mode cannot get more registers. The ten <64 x float> live
; accumulators keep the pressure over it however the two addresses are computed, so the first-compile remat is rolled
; back. The same remat is kept when it is not a first-compile run.

; RUN: igc_opt -platformCri --opaque-pointers %s -S -o - -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=1 --dce | FileCheck %s --check-prefix=KEEP
; RUN: igc_opt -platformCri --opaque-pointers %s -S -o - -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=1 -remat-first-compile --dce | FileCheck %s --check-prefix=ROLLBACK

; KEEP: !remat
; ROLLBACK-NOT: !remat
define spir_kernel void @main(ptr addrspace(3) %slm, ptr addrspace(1) %out, i32 %n, i32 %tid) {
entry:
  br label %loop

loop:
  %idx = phi i32 [ %tid, %entry ], [ %next, %loop ]
  %acc0 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc0, %loop ]
  %acc1 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc1, %loop ]
  %acc2 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc2, %loop ]
  %acc3 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc3, %loop ]
  %acc4 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc4, %loop ]
  %acc5 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc5, %loop ]
  %acc6 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc6, %loop ]
  %acc7 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc7, %loop ]
  %acc8 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc8, %loop ]
  %acc9 = phi <64 x float> [ zeroinitializer, %entry ], [ %nacc9, %loop ]
  %s = shl i32 %idx, 4
  %p0 = add i32 %s, 256
  %q0 = inttoptr i32 %p0 to ptr addrspace(3)
  %v0 = load <64 x float>, ptr addrspace(3) %q0, align 4
  %p1 = add i32 %s, 512
  %q1 = inttoptr i32 %p1 to ptr addrspace(3)
  %v1 = load <64 x float>, ptr addrspace(3) %q1, align 4
  %sum = fadd <64 x float> %v0, %v1
  %nacc0 = fadd <64 x float> %acc0, %sum
  %nacc1 = fadd <64 x float> %acc1, %sum
  %nacc2 = fadd <64 x float> %acc2, %sum
  %nacc3 = fadd <64 x float> %acc3, %sum
  %nacc4 = fadd <64 x float> %acc4, %sum
  %nacc5 = fadd <64 x float> %acc5, %sum
  %nacc6 = fadd <64 x float> %acc6, %sum
  %nacc7 = fadd <64 x float> %acc7, %sum
  %nacc8 = fadd <64 x float> %acc8, %sum
  %nacc9 = fadd <64 x float> %acc9, %sum
  %next = add i32 %idx, 1
  %cmp = icmp ult i32 %next, %n
  br i1 %cmp, label %loop, label %exit

exit:
  %e1 = fadd <64 x float> %nacc0, %nacc1
  %e2 = fadd <64 x float> %e1, %nacc2
  %e3 = fadd <64 x float> %e2, %nacc3
  %e4 = fadd <64 x float> %e3, %nacc4
  %e5 = fadd <64 x float> %e4, %nacc5
  %e6 = fadd <64 x float> %e5, %nacc6
  %e7 = fadd <64 x float> %e6, %nacc7
  %e8 = fadd <64 x float> %e7, %nacc8
  %e9 = fadd <64 x float> %e8, %nacc9
  store <64 x float> %e9, ptr addrspace(1) %out, align 4
  ret void
}
