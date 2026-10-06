;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey EnableLICMInvariantSwitchDispatchDetection --igc-special-cases-disable-licm -S < %s | FileCheck %s --implicit-check-not=llvm.licm.disable
;
; CHECK-LABEL: @test_invariant_switch_dispatch_debug_calls(
; CHECK: br i1 %exit_cond, label %exit, label %header{{$}}

; Debug intrinsics, lifetime markers and llvm.assume generate no code and must
; not count as hoistable work. The arm has 15 real hoistable instructions
define spir_kernel void @test_invariant_switch_dispatch_debug_calls(i32 %dispatch_val, float %input, ptr addrspace(1) %output) {
preheader:
  %tmp = alloca float, align 4
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
  call void @llvm.dbg.value(metadata float %v0, metadata !{}, metadata !DIExpression())
  %v1 = fmul float %v0, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v1, metadata !{}, metadata !DIExpression())
  %v2 = fmul float %v1, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v2, metadata !{}, metadata !DIExpression())
  %v3 = fmul float %v2, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v3, metadata !{}, metadata !DIExpression())
  %v4 = fmul float %v3, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v4, metadata !{}, metadata !DIExpression())
  %v5 = fmul float %v4, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v5, metadata !{}, metadata !DIExpression())
  %v6 = fmul float %v5, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v6, metadata !{}, metadata !DIExpression())
  %v7 = fmul float %v6, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v7, metadata !{}, metadata !DIExpression())
  %v8 = fmul float %v7, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v8, metadata !{}, metadata !DIExpression())
  %v9 = fmul float %v8, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v9, metadata !{}, metadata !DIExpression())
  %v10 = fmul float %v9, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v10, metadata !{}, metadata !DIExpression())
  %v11 = fmul float %v10, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v11, metadata !{}, metadata !DIExpression())
  %v12 = fmul float %v11, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v12, metadata !{}, metadata !DIExpression())
  %v13 = fmul float %v12, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v13, metadata !{}, metadata !DIExpression())
  %v14 = fmul float %v13, 2.000000e+00
  call void @llvm.dbg.value(metadata float %v14, metadata !{}, metadata !DIExpression())
  call void @llvm.lifetime.start.p0(i64 4, ptr %tmp)
  call void @llvm.lifetime.end.p0(i64 4, ptr %tmp)
  %cond = fcmp oge float %input, 0.000000e+00
  call void @llvm.assume(i1 %cond)
  store float %v14, ptr addrspace(1) %output, align 4
  br label %latch

latch:
  %next = add i32 %counter, 1
  %exit_cond = icmp eq i32 %next, 100
  br i1 %exit_cond, label %exit, label %header

exit:
  ret void
}

declare void @llvm.dbg.value(metadata, metadata, metadata)
declare void @llvm.lifetime.start.p0(i64, ptr nocapture)
declare void @llvm.lifetime.end.p0(i64, ptr nocapture)
declare void @llvm.assume(i1)

!igc.functions = !{}
