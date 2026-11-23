;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-clone-address-arithmetic --verify-each \
; RUN:   --regkey=RematRPELimit=0 --regkey=TotalGRFNum=16 --regkey=RematCallsOperand=1 | FileCheck %s

; Uniform clones must retain their dependency for the retry budget check. Without it the
; <32 x i32> add is counted as varying and the stage is rolled back despite fitting 16 GRFs.
; Cover both an operand chain cloned by greedy remat and a seed cloned by single-flow remat.

; CHECK-LABEL: define spir_kernel void @uniform_chain(
; CHECK: [[SUM:%.*]] = add <32 x i32> %a, zeroinitializer, !remat
; CHECK-NEXT: [[CMP:%.*]] = icmp eq <32 x i32> [[SUM]], %a, !remat
; CHECK-NEXT: store <32 x i1> [[CMP]], ptr addrspace(1) %p

define spir_kernel void @uniform_chain(ptr addrspace(1) %p, <32 x i32> %a) {
entry:
  %sum = add <32 x i32> %a, zeroinitializer
  %cmp = icmp eq <32 x i32> %sum, %a
  store <32 x i1> %cmp, ptr addrspace(1) %p
  ret void
}

; CHECK-LABEL: define spir_kernel void @uniform_seed(
; CHECK: [[SEED:%.*]] = inttoptr <32 x i64> %a to <32 x ptr addrspace(1)>, !remat
; CHECK-NEXT: call void @consume(<32 x ptr addrspace(1)> [[SEED]])

define spir_kernel void @uniform_seed(<32 x i64> %a) {
entry:
  %addr = inttoptr <32 x i64> %a to <32 x ptr addrspace(1)>
  call void @consume(<32 x ptr addrspace(1)> %addr)
  ret void
}

declare void @consume(<32 x ptr addrspace(1)>)

!igc.functions = !{!0, !3}
!0 = !{ptr @uniform_chain, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @uniform_seed, !1}
