;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: igc_opt --opaque-pointers -igc-legalization -S < %s | FileCheck %s --implicit-check-not='phi <4 x i1>'

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

; A bool phi narrower than a byte splits into one i1 phi per lane.
define i1 @test_phi_sub_byte(i1 %a0, i1 %a1, i1 %a2, i1 %a3, i1 %b0, i1 %b1, i1 %b2, i1 %b3, i1 %c) {
; CHECK-LABEL: define i1 @test_phi_sub_byte(
; CHECK:     A:
; CHECK-DAG:   [[A0:%.*]] = sext i1 %a0 to i32
; CHECK-DAG:   [[A1:%.*]] = sext i1 %a1 to i32
; CHECK-DAG:   [[A2:%.*]] = sext i1 %a2 to i32
; CHECK-DAG:   [[A3:%.*]] = sext i1 %a3 to i32
; CHECK:     B:
; CHECK-DAG:   [[B0:%.*]] = sext i1 %b0 to i32
; CHECK-DAG:   [[B1:%.*]] = sext i1 %b1 to i32
; CHECK-DAG:   [[B2:%.*]] = sext i1 %b2 to i32
; CHECK-DAG:   [[B3:%.*]] = sext i1 %b3 to i32
; CHECK:     Join:
; CHECK-NEXT:  phi i32 [ [[A0]], %A ], [ [[B0]], %B ]
; CHECK-NEXT:  phi i32 [ [[A1]], %A ], [ [[B1]], %B ]
; CHECK-NEXT:  [[PHI2:%.*]] = phi i32 [ [[A2]], %A ], [ [[B2]], %B ]
; CHECK-NEXT:  phi i32 [ [[A3]], %A ], [ [[B3]], %B ]
; CHECK:       [[L2:%.*]] = icmp ne i32 [[PHI2]], 0
; CHECK:       [[S2:%.*]] = sext i1 [[L2]] to i8
; CHECK-NEXT:  insertelement <4 x i8> {{%.*}}, i8 [[S2]], i32 2
; CHECK:       [[E:%.*]] = extractelement <4 x i8> {{%.*}}, i64 2
; CHECK-NEXT:  [[EM:%.*]] = and i8 [[E]], 1
; CHECK-NEXT:  [[T:%.*]] = icmp ne i8 [[EM]], 0
; CHECK:       ret i1 [[T]]
;
entry:
  br i1 %c, label %A, label %B

A:
  %va0 = insertelement <4 x i1> poison, i1 %a0, i64 0
  %va1 = insertelement <4 x i1> %va0, i1 %a1, i64 1
  %va2 = insertelement <4 x i1> %va1, i1 %a2, i64 2
  %va = insertelement <4 x i1> %va2, i1 %a3, i64 3
  br label %Join

B:
  %vb0 = insertelement <4 x i1> poison, i1 %b0, i64 0
  %vb1 = insertelement <4 x i1> %vb0, i1 %b1, i64 1
  %vb2 = insertelement <4 x i1> %vb1, i1 %b2, i64 2
  %vb = insertelement <4 x i1> %vb2, i1 %b3, i64 3
  br label %Join

Join:
  %v = phi <4 x i1> [ %va, %A ], [ %vb, %B ]
  %b = extractelement <4 x i1> %v, i64 2
  ret i1 %b
}

; Constant incoming vectors give constant lanes.
define i1 @test_phi_sub_byte_const(i1 %x, i1 %c) {
; CHECK-LABEL: define i1 @test_phi_sub_byte_const(
; CHECK:     A:
; CHECK-NEXT:  [[X:%.*]] = sext i1 %x to i32
; CHECK:     J:
; CHECK-NEXT:  phi i32 [ 0, %A ], [ 0, %B ]
; CHECK-NEXT:  phi i32 [ [[X]], %A ], [ 0, %B ]
;
entry:
  %v = insertelement <4 x i1> zeroinitializer, i1 %x, i32 1
  br i1 %c, label %A, label %B

A:
  br label %J

B:
  br label %J

J:
  %p = phi <4 x i1> [ %v, %A ], [ zeroinitializer, %B ]
  %r = extractelement <4 x i1> %p, i32 1
  ret i1 %r
}

; Duplicate entries for one predecessor must share one lane value.
define i1 @test_phi_sub_byte_dup_pred(<4 x i1> %v, i32 %s) {
; CHECK-LABEL: define i1 @test_phi_sub_byte_dup_pred(
; CHECK:       [[E1:%.*]] = extractelement <4 x i1> %v, i32 1
; CHECK:       [[S1:%.*]] = sext i1 [[E1]] to i32
; CHECK:     Join:
; CHECK-NEXT:  phi i32
; CHECK-NEXT:  phi i32 [ [[S1]], %entry ], [ [[S1]], %entry ]
;
entry:
  switch i32 %s, label %Join [ i32 0, label %Join ]

Join:
  %p = phi <4 x i1> [ %v, %entry ], [ %v, %entry ]
  %r = extractelement <4 x i1> %p, i32 1
  ret i1 %r
}

; A loop-carried sub-byte phi takes its latch lanes, defined after the phi, in the latch.
define i1 @test_phi_sub_byte_loop(i1 %a, i32 %n) {
; CHECK-LABEL: define i1 @test_phi_sub_byte_loop(
; CHECK:     loop:
; CHECK-NEXT:  phi i32 [ {{%.*}}, %entry ], [ [[LATCH0:%.*]], %loop ]
; CHECK:       [[LATCH0]] = sext i8 {{%.*}} to i32
; CHECK:       br i1 %done, label %exit, label %loop
;
entry:
  %init = insertelement <4 x i1> zeroinitializer, i1 %a, i64 0
  br label %loop

loop:
  %lanes = phi <4 x i1> [ %init, %entry ], [ %next.lanes, %loop ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %lane0 = extractelement <4 x i1> %lanes, i64 0
  %next.lanes = insertelement <4 x i1> %lanes, i1 %lane0, i64 3
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  %r = extractelement <4 x i1> %next.lanes, i64 2
  ret i1 %r
}

; Lanes the latch insert leaves untouched must keep their value when the phi is the block's last.
define i1 @test_phi_sub_byte_loop_last_phi(i1 %stop) {
; CHECK-LABEL: define i1 @test_phi_sub_byte_loop_last_phi(
; CHECK:     loop:
; CHECK-NEXT:  phi i32 [ 0, %entry ]
; CHECK-NEXT:  phi i32 [ 0, %entry ]
; CHECK-NEXT:  [[PHI2:%.*]] = phi i32 [ -1, %entry ]
; CHECK:       [[L2:%.*]] = icmp ne i32 [[PHI2]], 0
; CHECK:       [[S2:%.*]] = sext i1 [[L2]] to i8
; CHECK-NEXT:  insertelement <4 x i8> {{%.*}}, i8 [[S2]], i32 2
;
entry:
  br label %loop

loop:
  %lanes = phi <4 x i1> [ <i1 false, i1 false, i1 true, i1 false>, %entry ], [ %next.lanes, %loop ]
  %lane0 = extractelement <4 x i1> %lanes, i32 0
  %next.lanes = insertelement <4 x i1> %lanes, i1 %lane0, i32 3
  br i1 %stop, label %exit, label %loop

exit:
  %r = extractelement <4 x i1> %next.lanes, i32 2
  ret i1 %r
}

!igc.functions = !{!0, !2, !3, !4, !5, !6, !7, !8, !9, !10}

!0 = !{ptr @test_phi_bit24, !1}
!2 = !{ptr @test_phi_bitcast_wide, !1}
!3 = !{ptr @test_phi_loop_latch_load, !1}
!4 = !{ptr @test_phi_loop_join, !1}
!5 = !{ptr @test_phi_self, !1}
!6 = !{ptr @test_phi_sub_byte, !1}
!7 = !{ptr @test_phi_sub_byte_const, !1}
!8 = !{ptr @test_phi_sub_byte_dup_pred, !1}
!9 = !{ptr @test_phi_sub_byte_loop, !1}
!10 = !{ptr @test_phi_sub_byte_loop_last_phi, !1}
!1 = !{}
