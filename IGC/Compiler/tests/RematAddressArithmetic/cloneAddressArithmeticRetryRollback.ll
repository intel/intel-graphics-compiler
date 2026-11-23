;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys

; RUN: split-file %s %t
; RUN: igc_opt --opaque-pointers %t/greedy.ll -S -o - -igc-clone-address-arithmetic --regkey=RematFlowThreshold=1 \
; RUN:   --regkey=RematRPELimit=0 --dce | FileCheck %s --check-prefix=KEEP
; RUN: igc_opt --opaque-pointers %t/greedy.ll -S -o - -igc-clone-address-arithmetic --regkey=RematFlowThreshold=1 \
; RUN:   --regkey=RematRPELimit=0 --regkey=TotalGRFNum=8 --dce | FileCheck %s --check-prefix=ROLLBACK
; RUN: igc_opt --opaque-pointers %t/single-flow.ll -S -o - -igc-clone-address-arithmetic --regkey=RematRPELimit=0 \
; RUN:   --regkey=TotalGRFNum=8 | FileCheck %s --check-prefix=SF-ROLLBACK
; RUN: igc_opt --opaque-pointers %t/single-flow.ll -S -o - -igc-clone-address-arithmetic --regkey=RematRPELimit=0 \
; RUN:   --regkey=TotalGRFNum=8 --regkey=RematRetryRollback=0 | FileCheck %s --check-prefix=SF-KEEP

; Cloning %sext increases register pressure beyond the 8 GRF budget, which should trigger a rollback.
; KEEP: getelementptr i32, ptr addrspace(1) %p0, i64 %remat
; ROLLBACK-NOT: !remat
; ROLLBACK: %g0 = getelementptr i32, ptr addrspace(1) %p0, i64 %sext
; ROLLBACK-NOT: !remat

; The single flow stage clones each add next to its store, which keeps both PHIs live instead of the GEP: 21 -> 28 GRF.
; SF-ROLLBACK-NOT: add i64 %l0, %m0, !remat
; SF-KEEP: add i64 %l0, %m0, !remat

;--- greedy.ll
define spir_kernel void @test(ptr addrspace(1) %p0, ptr addrspace(1) %p1, ptr addrspace(1) %p2, ptr addrspace(1) %p3,
                              i32 %n, i32 %v, <8 x i32> %r0) {
entry:
  %tid = extractelement <8 x i32> %r0, i64 1
  %idx = mul i32 %tid, %n
  %sext = sext i32 %idx to i64
  %g0 = getelementptr i32, ptr addrspace(1) %p0, i64 %sext
  %g1 = getelementptr i32, ptr addrspace(1) %p1, i64 %sext
  %g2 = getelementptr i32, ptr addrspace(1) %p2, i64 %sext
  %g3 = getelementptr i32, ptr addrspace(1) %p3, i64 %sext
  store i32 %v, ptr addrspace(1) %g0, align 4
  store i32 %v, ptr addrspace(1) %g1, align 4
  store i32 %v, ptr addrspace(1) %g2, align 4
  store i32 %v, ptr addrspace(1) %g3, align 4
  ret void
}

;--- single-flow.ll
define spir_kernel void @single_flow(ptr addrspace(1) %p, ptr addrspace(1) %q, i1 %c) {
entry:
  %x0 = load i64, ptr addrspace(1) %q, align 8
  %y0 = load i64, ptr addrspace(1) %p, align 8
  %x1 = load i64, ptr addrspace(1) %q, align 8
  %y1 = load i64, ptr addrspace(1) %p, align 8
  %x2 = load i64, ptr addrspace(1) %q, align 8
  %y2 = load i64, ptr addrspace(1) %p, align 8
  %x3 = load i64, ptr addrspace(1) %q, align 8
  %y3 = load i64, ptr addrspace(1) %p, align 8
  br i1 %c, label %t, label %j
t:
  br label %j
j:
  %l0 = phi i64 [ %x0, %entry ], [ %y0, %t ]
  %m0 = phi i64 [ %y0, %entry ], [ %x0, %t ]
  %l1 = phi i64 [ %x1, %entry ], [ %y1, %t ]
  %m1 = phi i64 [ %y1, %entry ], [ %x1, %t ]
  %l2 = phi i64 [ %x2, %entry ], [ %y2, %t ]
  %m2 = phi i64 [ %y2, %entry ], [ %x2, %t ]
  %l3 = phi i64 [ %x3, %entry ], [ %y3, %t ]
  %m3 = phi i64 [ %y3, %entry ], [ %x3, %t ]
  %a0 = add i64 %l0, %m0
  %g0 = getelementptr i64, ptr addrspace(1) %p, i64 %a0
  %a1 = add i64 %l1, %m1
  %g1 = getelementptr i64, ptr addrspace(1) %p, i64 %a1
  %a2 = add i64 %l2, %m2
  %g2 = getelementptr i64, ptr addrspace(1) %p, i64 %a2
  %a3 = add i64 %l3, %m3
  %g3 = getelementptr i64, ptr addrspace(1) %p, i64 %a3
  %w0 = load i64, ptr addrspace(1) %q, align 8
  %w1 = load i64, ptr addrspace(1) %q, align 8
  %w2 = load i64, ptr addrspace(1) %q, align 8
  %w3 = load i64, ptr addrspace(1) %q, align 8
  store i64 %w0, ptr addrspace(1) %g0, align 8
  store i64 %w1, ptr addrspace(1) %g1, align 8
  store i64 %w2, ptr addrspace(1) %g2, align 8
  store i64 %w3, ptr addrspace(1) %g3, align 8
  store i64 %w0, ptr addrspace(1) %q, align 8
  store i64 %w1, ptr addrspace(1) %q, align 8
  store i64 %w2, ptr addrspace(1) %q, align 8
  store i64 %w3, ptr addrspace(1) %q, align 8
  ret void
}
