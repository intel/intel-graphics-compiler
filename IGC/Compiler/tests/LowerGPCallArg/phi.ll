;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-lower-gp-arg | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"


; CHECK-LABEL: define void @bothSrcsInTheSameAddrspace
define void @bothSrcsInTheSameAddrspace(i32 addrspace(1)* %ptr0, i32 addrspace(1)* %ptr1) {
  %asc0 = addrspacecast i32 addrspace(1)* %ptr0 to i32 addrspace(4)*
  %asc1 = addrspacecast i32 addrspace(1)* %ptr1 to i32 addrspace(4)*

  ; CHECK: call void @callee0(i32 addrspace(1)* %ptr0, i32 addrspace(1)* %ptr1)
  call void @callee0(i32 addrspace(4)* %asc0, i32 addrspace(4)* %asc1)

  ret void
}

; CHECK: define void @callee0(i32 addrspace(1)* %src0, i32 addrspace(1)* %src1)
define void @callee0(i32 addrspace(4)* %src0, i32 addrspace(4)* %src1) {
bb0:
  ; CHECK: %[[P0:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src0, i32 0
  %p0 = getelementptr inbounds i32, i32 addrspace(4)* %src0, i32 0
  br label %phi_bb
bb1:
  ; CHECK: %[[P1:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src1, i32 0
  %p1 = getelementptr inbounds i32, i32 addrspace(4)* %src1, i32 0
  br label %phi_bb
phi_bb:
  ; CHECK: %[[ADDR:.*]] = phi i32 addrspace(1)* [ %[[P0]], %bb0 ], [ %[[P1]], %bb1 ]
  %addr = phi i32 addrspace(4)* [ %p0, %bb0 ], [ %p1, %bb1 ]
  ; CHECK: store i32 0, i32 addrspace(1)* %[[ADDR]], align 4
  store i32 0, i32 addrspace(4)* %addr, align 4
  ret void
}

; CHECK-LABEL: define void @allSrcsInDifferentAddrspace
define void @allSrcsInDifferentAddrspace(i32* %ptr0, i32 addrspace(1)* %ptr1, i32 addrspace(3)* %ptr2) {
  %asc0 = addrspacecast i32* %ptr0 to i32 addrspace(4)*
  %asc1 = addrspacecast i32 addrspace(1)* %ptr1 to i32 addrspace(4)*
  %asc2 = addrspacecast i32 addrspace(3)* %ptr2 to i32 addrspace(4)*

  ; CHECK: call void @callee1(i32* %ptr0, i32 addrspace(1)* %ptr1, i32 addrspace(3)* %ptr2)
  call void @callee1(i32 addrspace(4)* %asc0, i32 addrspace(4)* %asc1, i32 addrspace(4)* %asc2)

  ret void
}

; CHECK: define void @callee1(i32* %src0, i32 addrspace(1)* %src1, i32 addrspace(3)* %src2)
define void @callee1(i32 addrspace(4)* %src0, i32 addrspace(4)* %src1, i32 addrspace(4)* %src2) {
bb0:
  ; CHECK: %[[P0:.*]] = getelementptr inbounds i32, i32* %src0, i32 0
  ; CHECK: %[[AS0:.*]] = addrspacecast i32* %[[P0]] to i32 addrspace(4)*
  %p0 = getelementptr inbounds i32, i32 addrspace(4)* %src0, i32 0
  br label %phi_bb
bb1:
  ; CHECK: %[[P1:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src1, i32 0
  ; CHECK: %[[AS1:.*]] = addrspacecast i32 addrspace(1)* %[[P1]] to i32 addrspace(4)*
  %p1 = getelementptr inbounds i32, i32 addrspace(4)* %src1, i32 0
  br label %phi_bb
bb2:
  ; CHECK: %[[P2:.*]] = getelementptr inbounds i32, i32 addrspace(3)* %src2, i32 0
  ; CHECK: %[[AS2:.*]] = addrspacecast i32 addrspace(3)* %[[P2]] to i32 addrspace(4)*
  %p2 = getelementptr inbounds i32, i32 addrspace(4)* %src2, i32 0
  br label %phi_bb
phi_bb:
  ; CHECK: %[[ADDR:.*]] = phi i32 addrspace(4)* [ %[[AS0]], %bb0 ], [ %[[AS1]], %bb1 ], [ %[[AS2]], %bb2 ]
  %addr = phi i32 addrspace(4)* [ %p0, %bb0 ], [ %p1, %bb1 ], [ %p2, %bb2 ]
  ; CHECK: store i32 0, i32 addrspace(4)* %[[ADDR]], align 4
  store i32 0, i32 addrspace(4)* %addr, align 4
  ret void
}

; CHECK-LABEL: define void @partiallyLoweredSrcsInTheSameAddrspace
define void @partiallyLoweredSrcsInTheSameAddrspace(i32 addrspace(1)* %ptr0, i32 addrspace(1)* %ptr1) {
  %asc0 = addrspacecast i32 addrspace(1)* %ptr0 to i32 addrspace(4)*
  %asc1 = addrspacecast i32 addrspace(1)* %ptr1 to i32 addrspace(4)*

  ; CHECK: call void @callee2(i32 addrspace(1)* %ptr0, i32 addrspace(1)* %ptr1)
  call void @callee2(i32 addrspace(4)* %asc0, i32 addrspace(4)* %asc1)

  ret void
}

; CHECK: define void @callee2(i32 addrspace(1)* %src0, i32 addrspace(1)* %src1)
define void @callee2(i32 addrspace(4)* %src0, i32 addrspace(4)* %src1) {
bb0:
  ; CHECK: %[[P0:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src0, i32 0
  ; CHECK: %[[AS0:.*]] = addrspacecast i32 addrspace(1)* %[[P0]] to i32 addrspace(4)*
  %p0 = getelementptr inbounds i32, i32 addrspace(4)* %src0, i32 0
  br label %phi_bb
bb1:
  ; CHECK: %[[P1:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src1, i32 0
  ; CHECK: %[[AS1:.*]] = addrspacecast i32 addrspace(1)* %[[P1]] to i32 addrspace(4)*
  %p1 = getelementptr inbounds i32, i32 addrspace(4)* %src1, i32 0
  br label %phi_bb
bb2:
  %alloca = alloca i32, align 4
  %asc = addrspacecast i32* %alloca to i32 addrspace(4)*
  %notLowerable = getelementptr inbounds i32, i32 addrspace(4)* %asc, i32 0
  br label %phi_bb
phi_bb:
  ; CHECK: %[[ADDR:.*]] = phi i32 addrspace(4)* [ %[[AS0]], %bb0 ], [ %[[AS1]], %bb1 ], [ %notLowerable, %bb2 ]
  %addr = phi i32 addrspace(4)* [ %p0, %bb0 ], [ %p1, %bb1 ], [ %notLowerable, %bb2 ]
  ; CHECK: store i32 0, i32 addrspace(4)* %[[ADDR]], align 4
  store i32 0, i32 addrspace(4)* %addr, align 4
  ret void
}

; CHECK-LABEL: define void @partiallyLoweredSrcsInDifferentAddrspace
define void @partiallyLoweredSrcsInDifferentAddrspace(i32 addrspace(1)* %ptr0, i32 addrspace(3)* %ptr1) {
  %asc0 = addrspacecast i32 addrspace(1)* %ptr0 to i32 addrspace(4)*
  %asc1 = addrspacecast i32 addrspace(3)* %ptr1 to i32 addrspace(4)*

  ; CHECK: call void @callee3(i32 addrspace(1)* %ptr0, i32 addrspace(3)* %ptr1)
  call void @callee3(i32 addrspace(4)* %asc0, i32 addrspace(4)* %asc1)

  ret void
}

; CHECK: define void @callee3(i32 addrspace(1)* %src0, i32 addrspace(3)* %src1)
define void @callee3(i32 addrspace(4)* %src0, i32 addrspace(4)* %src1) {
bb0:
  ; CHECK: %[[P0:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src0, i32 0
  ; CHECK: %[[AS0:.*]] = addrspacecast i32 addrspace(1)* %[[P0]] to i32 addrspace(4)*
  %p0 = getelementptr inbounds i32, i32 addrspace(4)* %src0, i32 0
  br label %phi_bb
bb1:
  ; CHECK: %[[P1:.*]] = getelementptr inbounds i32, i32 addrspace(3)* %src1, i32 0
  ; CHECK: %[[AS1:.*]] = addrspacecast i32 addrspace(3)* %[[P1]] to i32 addrspace(4)*
  %p1 = getelementptr inbounds i32, i32 addrspace(4)* %src1, i32 0
  br label %phi_bb
bb2:
  %alloca = alloca i32, align 4
  %asc = addrspacecast i32* %alloca to i32 addrspace(4)*
  %notLowerable = getelementptr inbounds i32, i32 addrspace(4)* %asc, i32 0
  br label %phi_bb
phi_bb:
  ; CHECK: %[[ADDR:.*]] = phi i32 addrspace(4)* [ %[[AS0]], %bb0 ], [ %[[AS1]], %bb1 ], [ %notLowerable, %bb2 ]
  %addr = phi i32 addrspace(4)* [ %p0, %bb0 ], [ %p1, %bb1 ], [ %notLowerable, %bb2 ]
  ; CHECK: store i32 0, i32 addrspace(4)* %[[ADDR]], align 4
  store i32 0, i32 addrspace(4)* %addr, align 4
  ret void
}

; CHECK-LABEL: define void @consecutivePhis
define void @consecutivePhis(i32 addrspace(1)* %ptr0, i32 addrspace(3)* %ptr1, i32 addrspace(3)* %ptr2, i32 addrspace(1)* %ptr3) {
  %asc0 = addrspacecast i32 addrspace(1)* %ptr0 to i32 addrspace(4)*
  %asc1 = addrspacecast i32 addrspace(3)* %ptr1 to i32 addrspace(4)*
  %asc2 = addrspacecast i32 addrspace(3)* %ptr2 to i32 addrspace(4)*
  %asc3 = addrspacecast i32 addrspace(1)* %ptr3 to i32 addrspace(4)*

  ; CHECK: call void @callee4(i32 addrspace(1)* %ptr0, i32 addrspace(3)* %ptr1, i32 addrspace(3)* %ptr2, i32 addrspace(1)* %ptr3)
  call void @callee4(i32 addrspace(4)* %asc0, i32 addrspace(4)* %asc1, i32 addrspace(4)* %asc2, i32 addrspace(4)* %asc3)

  ret void
}

; CHECK: define void @callee4(i32 addrspace(1)* %src0, i32 addrspace(3)* %src1, i32 addrspace(3)* %src2, i32 addrspace(1)* %src3)
define void @callee4(i32 addrspace(4)* %src0, i32 addrspace(4)* %src1, i32 addrspace(4)* %src2, i32 addrspace(4)* %src3) {
bb0:
  ; CHECK: %[[A0:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src0, i32 0
  ; CHECK: %[[A0_GENERIC:.*]] = addrspacecast i32 addrspace(1)* %[[A0]] to i32 addrspace(4)*
  %a0 = getelementptr inbounds i32, i32 addrspace(4)* %src0, i32 0
  br label %phi1_bb
bb1:
  ; CHECK: %[[A1:.*]] = getelementptr inbounds i32, i32 addrspace(3)* %src1, i32 0
  ; CHECK: %[[A1_GENERIC:.*]] = addrspacecast i32 addrspace(3)* %[[A1]] to i32 addrspace(4)*
  %a1 = getelementptr inbounds i32, i32 addrspace(4)* %src1, i32 0
  br i1 0, label %phi0_bb, label %phi3_bb
bb2:
  ; CHECK: %[[A2:.*]] = getelementptr inbounds i32, i32 addrspace(3)* %src2, i32 0
  %a2 = getelementptr inbounds i32, i32 addrspace(4)* %src2, i32 0
  br label %phi0_bb
bb3:
  ; CHECK: %[[A3:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src3, i32 0
  ; CHECK: %[[A3_GENERIC:.*]] = addrspacecast i32 addrspace(1)* %[[A3]] to i32 addrspace(4)*
  %a3 = getelementptr inbounds i32, i32 addrspace(4)* %src3, i32 0
  br label %phi2_bb
phi0_bb:
  ; CHECK: %[[DST0:.*]] = phi i32 addrspace(3)* [ %[[A1]], %bb1 ], [ %[[A2]], %bb2 ]
  ; CHECK: %[[DST0_GENERIC:.*]] = addrspacecast i32 addrspace(3)* %dst0 to i32 addrspace(4)*
  %dst0 = phi i32 addrspace(4)* [ %a1, %bb1 ], [ %a2, %bb2 ]
  ; CHECK: store i32 0, i32 addrspace(3)* %[[DST0]], align 4
  store i32 0, i32 addrspace(4)* %dst0, align 4
  br label %phi1_bb
phi1_bb:
  ; CHECK: %[[DST1:.*]] = phi i32 addrspace(4)* [ %[[A0_GENERIC]], %bb0 ], [ %[[DST0_GENERIC]], %phi0_bb ]
  %dst1 = phi i32 addrspace(4)* [ %a0, %bb0 ], [ %dst0, %phi0_bb ]
  ; CHECK: store i32 1, i32 addrspace(4)* %[[DST1]], align 4
  store i32 1, i32 addrspace(4)* %dst1, align 4
  br label %phi2_bb
phi2_bb:
  ; CHECK: %[[DST2:.*]] = phi i32 addrspace(4)* [ %[[DST1]], %phi1_bb ], [ %[[A3_GENERIC]], %bb3 ]
  %dst2 = phi i32 addrspace(4)* [ %dst1, %phi1_bb ], [ %a3, %bb3 ]
  ; CHECK: store i32 2, i32 addrspace(4)* %[[DST2]], align 4
  store i32 2, i32 addrspace(4)* %dst2, align 4
  br label %phi3_bb
phi3_bb:
  ; CHECK: %[[DST3:.*]] = phi i32 addrspace(4)* [ %[[A1_GENERIC]], %bb1 ], [ %[[DST2]], %phi2_bb ]
  %dst3 = phi i32 addrspace(4)* [ %a1, %bb1 ], [ %dst2, %phi2_bb]
  ; CHECK: store i32 3, i32 addrspace(4)* %[[DST3]], align 4
  store i32 3, i32 addrspace(4)* %dst3, align 4

  ret void
}

!igc.functions = !{!0, !3, !4, !5, !6, !7, !8, !9, !10, !11}

!0 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @bothSrcsInTheSameAddrspace, !1}
!3 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @callee0, !1}
!4 = !{void (i32*, i32 addrspace(1)*, i32 addrspace(3)*)* @allSrcsInDifferentAddrspace, !1}
!5 = !{void (i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*)* @callee1, !1}
!6 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @partiallyLoweredSrcsInTheSameAddrspace, !1}
!7 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @callee2, !1}
!8 = !{void (i32 addrspace(1)*, i32 addrspace(3)*)* @partiallyLoweredSrcsInDifferentAddrspace, !1}
!9 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @callee3, !1}
!10 = !{void (i32 addrspace(1)*, i32 addrspace(3)*, i32 addrspace(3)*, i32 addrspace(1)*)* @consecutivePhis, !1}
!11 = !{void (i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*)* @callee4, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
