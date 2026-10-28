;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - -igc-lower-gp-arg | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - -igc-lower-gp-arg | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"


; CHECK-LABEL: define void @bothSrcsInTheSameAddrspace
define void @bothSrcsInTheSameAddrspace(i32 addrspace(1)* %ptr0, i32 addrspace(1)* %ptr1) {
  %asc0 = addrspacecast i32 addrspace(1)* %ptr0 to i32 addrspace(4)*
  %asc1 = addrspacecast i32 addrspace(1)* %ptr1 to i32 addrspace(4)*

  ; CHECK-TYPED: call void @callee0(i32 addrspace(1)* %ptr0, i32 addrspace(1)* %ptr1)
  ; CHECK-OPAQUE: call void @callee0(ptr addrspace(1) %ptr0, ptr addrspace(1) %ptr1)
  call void @callee0(i32 addrspace(4)* %asc0, i32 addrspace(4)* %asc1)

  ret void
}

; CHECK-TYPED: define void @callee0(i32 addrspace(1)* %src0, i32 addrspace(1)* %src1)
; CHECK-OPAQUE: define void @callee0(ptr addrspace(1) %src0, ptr addrspace(1) %src1)
define void @callee0(i32 addrspace(4)* %src0, i32 addrspace(4)* %src1) {
  ; CHECK-TYPED: %[[P0:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src0, i32 0
  ; CHECK-OPAQUE: %[[P0:.*]] = getelementptr inbounds i32, ptr addrspace(1) %src0, i32 0
  %p0 = getelementptr inbounds i32, i32 addrspace(4)* %src0, i32 0
  ; CHECK-TYPED: %[[P1:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src1, i32 0
  ; CHECK-OPAQUE: %[[P1:.*]] = getelementptr inbounds i32, ptr addrspace(1) %src1, i32 0
  %p1 = getelementptr inbounds i32, i32 addrspace(4)* %src1, i32 0
  ; CHECK-TYPED: %[[ADDR:.*]] = select i1 false, i32 addrspace(1)* %[[P0]], i32 addrspace(1)* %[[P1]]
  ; CHECK-OPAQUE: %[[ADDR:.*]] = select i1 false, ptr addrspace(1) %[[P0]], ptr addrspace(1) %[[P1]]
  %addr = select i1 0, i32 addrspace(4)* %p0, i32 addrspace(4)* %p1
  ; CHECK-TYPED: store i32 0, i32 addrspace(1)* %[[ADDR]], align 4
  ; CHECK-OPAQUE: store i32 0, ptr addrspace(1) %[[ADDR]], align 4
  store i32 0, i32 addrspace(4)* %addr, align 4
  ret void
}

; CHECK-LABEL: define void @allSrcsInDifferentAddrspace
define void @allSrcsInDifferentAddrspace(i32* %ptr0, i32 addrspace(1)* %ptr1) {
  %asc0 = addrspacecast i32* %ptr0 to i32 addrspace(4)*
  %asc1 = addrspacecast i32 addrspace(1)* %ptr1 to i32 addrspace(4)*

  ; CHECK-TYPED: call void @callee1(i32* %ptr0, i32 addrspace(1)* %ptr1)
  ; CHECK-OPAQUE: call void @callee1(ptr %ptr0, ptr addrspace(1) %ptr1)
  call void @callee1(i32 addrspace(4)* %asc0, i32 addrspace(4)* %asc1)

  ret void
}

; CHECK-TYPED: define void @callee1(i32* %src0, i32 addrspace(1)* %src1)
; CHECK-OPAQUE: define void @callee1(ptr %src0, ptr addrspace(1) %src1)
define void @callee1(i32 addrspace(4)* %src0, i32 addrspace(4)* %src1) {
  ; CHECK-TYPED: %[[P0:.*]] = getelementptr inbounds i32, i32* %src0, i32 0
  ; CHECK-OPAQUE: %[[P0:.*]] = getelementptr inbounds i32, ptr %src0, i32 0
  ; CHECK-TYPED: %[[AS0:.*]] = addrspacecast i32* %[[P0]] to i32 addrspace(4)*
  ; CHECK-OPAQUE: %[[AS0:.*]] = addrspacecast ptr %[[P0]] to ptr addrspace(4)
  %p0 = getelementptr inbounds i32, i32 addrspace(4)* %src0, i32 0
  ; CHECK-TYPED: %[[P1:.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src1, i32 0
  ; CHECK-OPAQUE: %[[P1:.*]] = getelementptr inbounds i32, ptr addrspace(1) %src1, i32 0
  ; CHECK-TYPED: %[[AS1:.*]] = addrspacecast i32 addrspace(1)* %p1 to i32 addrspace(4)*
  ; CHECK-OPAQUE: %[[AS1:.*]] = addrspacecast ptr addrspace(1) %p1 to ptr addrspace(4)
  %p1 = getelementptr inbounds i32, i32 addrspace(4)* %src1, i32 0
  ; CHECK-TYPED: %[[ADDR:.*]] = select i1 false, i32 addrspace(4)* %[[AS0]], i32 addrspace(4)* %[[AS1]]
  ; CHECK-OPAQUE: %[[ADDR:.*]] = select i1 false, ptr addrspace(4) %[[AS0]], ptr addrspace(4) %[[AS1]]
  %addr = select i1 0, i32 addrspace(4)* %p0, i32 addrspace(4)* %p1
  ; CHECK-TYPED: store i32 0, i32 addrspace(4)* %[[ADDR]], align 4
  ; CHECK-OPAQUE: store i32 0, ptr addrspace(4) %[[ADDR]], align 4
  store i32 0, i32 addrspace(4)* %addr, align 4
  ret void
}

!igc.functions = !{!0, !3, !4, !5}

!0 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @bothSrcsInTheSameAddrspace, !1}
!3 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @callee0, !1}
!4 = !{void (i32*, i32 addrspace(1)*)* @allSrcsInDifferentAddrspace, !1}
!5 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @callee1, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
