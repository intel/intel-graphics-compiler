;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: igc_opt --opaque-pointers -igc-legalization -S < %s | FileCheck %s

; GenericAddressDynamicResolution runs after the last InstCombine and merges the
; bool vector loads of a generic pointer through a phi. The phi is legalized to
; bytes, so each extract becomes a byte extract + shift + mask + trunc.

define i1 @test_phi_bit24(ptr addrspace(4) %p, i1 %c) {
; CHECK-LABEL: define i1 @test_phi_bit24(
; CHECK-NOT:   <64 x i1>
; CHECK:     LocalBlock:
; CHECK:       [[LL:%.*]] = load <8 x i8>, ptr addrspace(3)
; CHECK:     GlobalBlock:
; CHECK:       [[GL:%.*]] = load <8 x i8>, ptr addrspace(1)
; CHECK:     Join:
; CHECK-NEXT:  [[PHI:%.*]] = phi <8 x i8> [ [[LL]], %LocalBlock ], [ [[GL]], %GlobalBlock ]
; CHECK:       [[E:%.*]] = extractelement <8 x i8> [[PHI]], i32 3
; CHECK:       [[A:%.*]] = and i8 [[E]], 1
; CHECK:       [[T:%.*]] = trunc i8 [[A]] to i1
; CHECK:       ret i1 [[T]]
; CHECK-NOT:   <64 x i1>
;
entry:
  br i1 %c, label %LocalBlock, label %GlobalBlock

LocalBlock:
  %lp = addrspacecast ptr addrspace(4) %p to ptr addrspace(3)
  %localLoad = load <64 x i1>, ptr addrspace(3) %lp, align 8
  br label %Join

GlobalBlock:
  %gp = addrspacecast ptr addrspace(4) %p to ptr addrspace(1)
  %globalLoad = load <64 x i1>, ptr addrspace(1) %gp, align 8
  br label %Join

Join:
  %v = phi <64 x i1> [ %localLoad, %LocalBlock ], [ %globalLoad, %GlobalBlock ]
  %b = extractelement <64 x i1> %v, i64 24
  ret i1 %b
}

define i1 @test_phi_bitcast_wide(<2 x i32> %w, ptr addrspace(1) %p, i1 %c) {
; CHECK-LABEL: define i1 @test_phi_bitcast_wide(
; CHECK-NOT:   <64 x i1>
; CHECK:     A:
; CHECK:       [[WB:%.*]] = bitcast <2 x i32> %w to <8 x i8>
; CHECK:     B:
; CHECK:       [[L:%.*]] = load <8 x i8>, ptr addrspace(1)
; CHECK:     Join:
; CHECK-NEXT:  [[PHI:%.*]] = phi <8 x i8> [ [[WB]], %A ], [ [[L]], %B ]
; CHECK:       [[E:%.*]] = extractelement <8 x i8> [[PHI]], i32 3
; CHECK-NOT:   <64 x i1>
;
entry:
  br i1 %c, label %A, label %B

A:
  %wb = bitcast <2 x i32> %w to <64 x i1>
  br label %Join

B:
  %l = load <64 x i1>, ptr addrspace(1) %p, align 8
  br label %Join

Join:
  %v = phi <64 x i1> [ %wb, %A ], [ %l, %B ]
  %b = extractelement <64 x i1> %v, i64 24
  ret i1 %b
}

define i1 @test_phi_loop_latch_load(ptr addrspace(1) %p, <8 x i8> %init, i32 %n) {
; CHECK-LABEL: define i1 @test_phi_loop_latch_load(
; CHECK-NOT:   <64 x i1>
; CHECK:     Header:
; CHECK-NEXT:  [[PHI:%.*]] = phi <8 x i8> [ %init, %entry ], [ [[L:%.*]], %Latch ]
; CHECK:     Latch:
; CHECK:       [[L]] = load <8 x i8>, ptr addrspace(1)
; CHECK-NOT:   load
; CHECK:     Exit:
; CHECK:       [[E:%.*]] = extractelement <8 x i8> [[PHI]], i32 3
; CHECK-NOT:   <64 x i1>
;
entry:
  %ib = bitcast <8 x i8> %init to <64 x i1>
  br label %Header

Header:
  %v = phi <64 x i1> [ %ib, %entry ], [ %l, %Latch ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %Latch ]
  %done = icmp eq i32 %i, %n
  br i1 %done, label %Exit, label %Latch

Latch:
  %l = load <64 x i1>, ptr addrspace(1) %p, align 8
  %i.next = add i32 %i, 1
  br label %Header

Exit:
  %b = extractelement <64 x i1> %v, i64 24
  ret i1 %b
}

define i1 @test_phi_loop_join(ptr addrspace(4) %p, <8 x i8> %init, i1 %c, i32 %n) {
; CHECK-LABEL: define i1 @test_phi_loop_join(
; CHECK-NOT:   <64 x i1>
; CHECK:     Header:
; CHECK-NEXT:  [[HPHI:%.*]] = phi <8 x i8> [ %init, %entry ], [ [[JPHI:%.*]], %Join ]
; CHECK:     Join:
; CHECK-NEXT:  [[JPHI]] = phi <8 x i8>
; CHECK:     Exit:
; CHECK:       extractelement <8 x i8> [[HPHI]], i32 3
; CHECK-NOT:   <64 x i1>
;
entry:
  %ib = bitcast <8 x i8> %init to <64 x i1>
  br label %Header

Header:
  %v = phi <64 x i1> [ %ib, %entry ], [ %join, %Join ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %Join ]
  %done = icmp eq i32 %i, %n
  br i1 %done, label %Exit, label %Body

Body:
  br i1 %c, label %LocalBlock, label %GlobalBlock

LocalBlock:
  %lp = addrspacecast ptr addrspace(4) %p to ptr addrspace(3)
  %localLoad = load <64 x i1>, ptr addrspace(3) %lp, align 8
  br label %Join

GlobalBlock:
  %gp = addrspacecast ptr addrspace(4) %p to ptr addrspace(1)
  %globalLoad = load <64 x i1>, ptr addrspace(1) %gp, align 8
  br label %Join

Join:
  %join = phi <64 x i1> [ %localLoad, %LocalBlock ], [ %globalLoad, %GlobalBlock ]
  %i.next = add i32 %i, 1
  br label %Header

Exit:
  %b = extractelement <64 x i1> %v, i64 24
  ret i1 %b
}

define i1 @test_phi_self(<8 x i8> %init, i32 %n) {
; CHECK-LABEL: define i1 @test_phi_self(
; CHECK-NOT:   <64 x i1>
; CHECK:     Header:
; CHECK-NEXT:  [[PHI:%.*]] = phi <8 x i8> [ %init, %entry ], [ [[PHI]], %Header ]
; CHECK:     Exit:
; CHECK:       extractelement <8 x i8> [[PHI]], i32 3
; CHECK-NOT:   <64 x i1>
;
entry:
  %ib = bitcast <8 x i8> %init to <64 x i1>
  br label %Header

Header:
  %v = phi <64 x i1> [ %ib, %entry ], [ %v, %Header ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %Header ]
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %Exit, label %Header

Exit:
  %b = extractelement <64 x i1> %v, i64 24
  ret i1 %b
}

!igc.functions = !{!0, !2, !3, !4, !5}

!0 = !{ptr @test_phi_bit24, !1}
!2 = !{ptr @test_phi_bitcast_wide, !1}
!3 = !{ptr @test_phi_loop_latch_load, !1}
!4 = !{ptr @test_phi_loop_join, !1}
!5 = !{ptr @test_phi_self, !1}
!1 = !{}
