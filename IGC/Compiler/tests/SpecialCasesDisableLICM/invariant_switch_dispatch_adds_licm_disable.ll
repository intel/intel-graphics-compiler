;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey EnableLICMInvariantSwitchDispatchDetection --igc-special-cases-disable-licm -S < %s | FileCheck %s
; ------------------------------------------------
; SpecialCasesDisableLICM : LoopHasInvariantSwitchDispatch
; ------------------------------------------------

; A loop containing many dispatch blocks (each a single icmp against the same
; loop-invariant value + conditional branch) should have LICM disabled.

; CHECK-LABEL: @test_invariant_switch_dispatch(
; CHECK: br i1 %exit_cond, label %exit, label %header, !llvm.loop [[LOOP0:![0-9]+]]


define spir_kernel void @test_invariant_switch_dispatch(i32 %dispatch_val, float %input, ptr addrspace(1) %output) {
preheader:
  br label %header

header:
  %counter = phi i32 [ 0, %preheader ], [ %next, %latch ]
  br label %dispatch0

dispatch0:
  %c0 = icmp eq i32 %dispatch_val, 0
  br i1 %c0, label %arm, label %dispatch1

dispatch1:
  %c1 = icmp eq i32 %dispatch_val, 1
  br i1 %c1, label %arm, label %dispatch2

dispatch2:
  %c2 = icmp eq i32 %dispatch_val, 2
  br i1 %c2, label %arm, label %dispatch3

dispatch3:
  %c3 = icmp eq i32 %dispatch_val, 3
  br i1 %c3, label %arm, label %dispatch4

dispatch4:
  %c4 = icmp eq i32 %dispatch_val, 4
  br i1 %c4, label %arm, label %dispatch5

dispatch5:
  %c5 = icmp eq i32 %dispatch_val, 5
  br i1 %c5, label %arm, label %dispatch6

dispatch6:
  %c6 = icmp eq i32 %dispatch_val, 6
  br i1 %c6, label %arm, label %dispatch7

dispatch7:
  %c7 = icmp eq i32 %dispatch_val, 7
  br i1 %c7, label %arm, label %dispatch8

dispatch8:
  %c8 = icmp eq i32 %dispatch_val, 8
  br i1 %c8, label %arm, label %dispatch9

dispatch9:
  %c9 = icmp eq i32 %dispatch_val, 9
  br i1 %c9, label %arm, label %dispatch10

dispatch10:
  %c10 = icmp eq i32 %dispatch_val, 10
  br i1 %c10, label %arm, label %dispatch11

dispatch11:
  %c11 = icmp eq i32 %dispatch_val, 11
  br i1 %c11, label %arm, label %dispatch12

dispatch12:
  %c12 = icmp eq i32 %dispatch_val, 12
  br i1 %c12, label %arm, label %dispatch13

dispatch13:
  %c13 = icmp eq i32 %dispatch_val, 13
  br i1 %c13, label %arm, label %dispatch14

dispatch14:
  %c14 = icmp eq i32 %dispatch_val, 14
  br i1 %c14, label %arm, label %latch

arm:
  %v0 = fmul float %input, 2.000000e+00
  %v1 = fmul float %v0, 2.000000e+00
  %v2 = fmul float %v1, 2.000000e+00
  %v3 = fmul float %v2, 2.000000e+00
  %v4 = fmul float %v3, 2.000000e+00
  %v5 = fmul float %v4, 2.000000e+00
  %v6 = fmul float %v5, 2.000000e+00
  %v7 = fmul float %v6, 2.000000e+00
  %v8 = fmul float %v7, 2.000000e+00
  %v9 = fmul float %v8, 2.000000e+00
  %v10 = fmul float %v9, 2.000000e+00
  %v11 = fmul float %v10, 2.000000e+00
  %v12 = fmul float %v11, 2.000000e+00
  %v13 = fmul float %v12, 2.000000e+00
  %v14 = fmul float %v13, 2.000000e+00
  %v15 = fmul float %v14, 2.000000e+00
  %v16 = fmul float %v15, 2.000000e+00
  %v17 = fmul float %v16, 2.000000e+00
  %v18 = fmul float %v17, 2.000000e+00
  %v19 = fmul float %v18, 2.000000e+00
  %v20 = fmul float %v19, 2.000000e+00
  %v21 = fmul float %v20, 2.000000e+00
  %v22 = fmul float %v21, 2.000000e+00
  %v23 = fmul float %v22, 2.000000e+00
  %v24 = fmul float %v23, 2.000000e+00
  %v25 = fmul float %v24, 2.000000e+00
  %v26 = fmul float %v25, 2.000000e+00
  %v27 = fmul float %v26, 2.000000e+00
  %v28 = fmul float %v27, 2.000000e+00
  %v29 = fmul float %v28, 2.000000e+00
  %v30 = fmul float %v29, 2.000000e+00
  %v31 = fmul float %v30, 2.000000e+00
  store float %v31, ptr addrspace(1) %output, align 4
  br label %latch

latch:
  %next = add i32 %counter, 1
  %exit_cond = icmp eq i32 %next, 100
  br i1 %exit_cond, label %exit, label %header

exit:
  ret void
}

; Tests for a default case which is only dominated by the header, which has more than 2 instructions.
; CHECK-LABEL: @test_header_root_pivot(
; CHECK: br i1 %exit_cond, label %exit, label %header, !llvm.loop [[LOOP1:![0-9]+]]
define spir_kernel void @test_header_root_pivot(i32 %dispatch_val, float %input, ptr addrspace(1) %output) {
preheader:
  br label %header

header:
  %counter = phi i32 [ 0, %preheader ], [ %next, %latch ]
  %variant = sitofp i32 %counter to float
  %root = icmp ult i32 %dispatch_val, 8
  br i1 %root, label %dispatch0, label %dispatch8

dispatch0:
  %c0 = icmp eq i32 %dispatch_val, 0
  br i1 %c0, label %case, label %dispatch1

dispatch1:
  %c1 = icmp eq i32 %dispatch_val, 1
  br i1 %c1, label %case, label %dispatch2

dispatch2:
  %c2 = icmp eq i32 %dispatch_val, 2
  br i1 %c2, label %case, label %dispatch3

dispatch3:
  %c3 = icmp eq i32 %dispatch_val, 3
  br i1 %c3, label %case, label %dispatch4

dispatch4:
  %c4 = icmp eq i32 %dispatch_val, 4
  br i1 %c4, label %case, label %dispatch5

dispatch5:
  %c5 = icmp eq i32 %dispatch_val, 5
  br i1 %c5, label %case, label %dispatch6

dispatch6:
  %c6 = icmp eq i32 %dispatch_val, 6
  br i1 %c6, label %case, label %dispatch7

dispatch7:
  %c7 = icmp eq i32 %dispatch_val, 7
  br i1 %c7, label %case, label %default

dispatch8:
  %c8 = icmp eq i32 %dispatch_val, 8
  br i1 %c8, label %case, label %dispatch9

dispatch9:
  %c9 = icmp eq i32 %dispatch_val, 9
  br i1 %c9, label %case, label %dispatch10

dispatch10:
  %c10 = icmp eq i32 %dispatch_val, 10
  br i1 %c10, label %case, label %dispatch11

dispatch11:
  %c11 = icmp eq i32 %dispatch_val, 11
  br i1 %c11, label %case, label %dispatch12

dispatch12:
  %c12 = icmp eq i32 %dispatch_val, 12
  br i1 %c12, label %case, label %dispatch13

dispatch13:
  %c13 = icmp eq i32 %dispatch_val, 13
  br i1 %c13, label %case, label %dispatch14

dispatch14:
  %c14 = icmp eq i32 %dispatch_val, 14
  br i1 %c14, label %case, label %dispatch15

dispatch15:
  %c15 = icmp eq i32 %dispatch_val, 15
  br i1 %c15, label %case, label %default

case:
  store float %variant, ptr addrspace(1) %output, align 4
  br label %latch

default:
  %v0 = fmul float %input, 2.000000e+00
  %v1 = fmul float %v0, 2.000000e+00
  %v2 = fmul float %v1, 2.000000e+00
  %v3 = fmul float %v2, 2.000000e+00
  %v4 = fmul float %v3, 2.000000e+00
  %v5 = fmul float %v4, 2.000000e+00
  %v6 = fmul float %v5, 2.000000e+00
  %v7 = fmul float %v6, 2.000000e+00
  %v8 = fmul float %v7, 2.000000e+00
  %v9 = fmul float %v8, 2.000000e+00
  %v10 = fmul float %v9, 2.000000e+00
  %v11 = fmul float %v10, 2.000000e+00
  %v12 = fmul float %v11, 2.000000e+00
  %v13 = fmul float %v12, 2.000000e+00
  %v14 = fmul float %v13, 2.000000e+00
  %v15 = fmul float %v14, 2.000000e+00
  %v16 = fmul float %v15, 2.000000e+00
  %v17 = fmul float %v16, 2.000000e+00
  %v18 = fmul float %v17, 2.000000e+00
  %v19 = fmul float %v18, 2.000000e+00
  %v20 = fmul float %v19, 2.000000e+00
  %v21 = fmul float %v20, 2.000000e+00
  %v22 = fmul float %v21, 2.000000e+00
  %v23 = fmul float %v22, 2.000000e+00
  %v24 = fmul float %v23, 2.000000e+00
  %v25 = fmul float %v24, 2.000000e+00
  %v26 = fmul float %v25, 2.000000e+00
  %v27 = fmul float %v26, 2.000000e+00
  %v28 = fmul float %v27, 2.000000e+00
  %v29 = fmul float %v28, 2.000000e+00
  %v30 = fmul float %v29, 2.000000e+00
  %v31 = fmul float %v30, 2.000000e+00
  store float %v31, ptr addrspace(1) %output, align 4
  br label %latch

latch:
  %next = add i32 %counter, 1
  %exit_cond = icmp eq i32 %next, 100
  br i1 %exit_cond, label %exit, label %header

exit:
  ret void
}
; CHECK: [[LOOP0]] = distinct !{[[LOOP0]], [[LICM_DISABLE:![0-9]+]]}
; CHECK: [[LICM_DISABLE]] = !{!"llvm.licm.disable"}
; CHECK: [[LOOP1]] = distinct !{[[LOOP1]], [[LICM_DISABLE]]}
!igc.functions = !{}
