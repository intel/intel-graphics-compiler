;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-loop-canonicalization -S < %s | FileCheck %s
; ------------------------------------------------
; LoopCanonicalization
; ------------------------------------------------

define spir_kernel void @test_loop(i32* %a, i32* %b, i32 %c) {
; CHECK-LABEL: @test_loop(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[CMP_0:%.*]] = icmp eq i32* %a, %b
; CHECK-NEXT:    br i1 [[CMP_0]], label [[FOR_END:%.*]], label [[FOR_BODY_PREHEADER:%.*]]
; CHECK:       for.body.preheader:
; CHECK-NEXT:    br label [[FOR_BODY:%.*]]
; CHECK:       for.body:
; CHECK-NEXT:    [[P_0:%.*]] = phi i32* [ %a, [[FOR_BODY_PREHEADER]] ], [ [[P_0_BE:%.*]], [[FOR_BODY_BACKEDGE:%.*]] ]
; CHECK-NEXT:    [[P_1:%.*]] = phi i32 [ 42, [[FOR_BODY_PREHEADER]] ], [ [[P_1]], [[FOR_BODY_BACKEDGE]] ]
; CHECK-NEXT:    [[TMP0:%.*]] = load i32, i32* [[P_0]], align 4
; CHECK-NEXT:    [[CMP_1:%.*]] = icmp eq i32 [[TMP0]], [[P_1]]
; CHECK-NEXT:    br i1 [[CMP_1]], label [[FOR_IF:%.*]], label [[FOR_ELSE:%.*]]
; CHECK:       for.if:
; CHECK-NEXT:    br label [[FOR_BODY_BACKEDGE]]
; CHECK:       for.body.backedge:
; CHECK-NEXT:    [[P_0_BE]] = phi i32* [ [[INC_P:%.*]], [[FOR_ELSE]] ], [ %b, [[FOR_IF]] ]
; CHECK-NEXT:    br label [[FOR_BODY]]
; CHECK:       for.else:
; CHECK-NEXT:    [[INC_P]] = getelementptr inbounds i32, i32* [[P_0]], i64 1
; CHECK-NEXT:    [[CMP_2:%.*]] = icmp eq i32* [[INC_P]], %b
; CHECK-NEXT:    br i1 [[CMP_2]], label [[FOR_END]], label [[FOR_BODY_BACKEDGE]]
; CHECK:       for.end:
; CHECK-NEXT:    [[P_3:%.*]] = phi i32 [ %c, [[ENTRY:%.*]] ], [ [[TMP0]], [[FOR_ELSE]] ]
; CHECK-NEXT:    store i32 [[P_3]], i32* %a
; CHECK-NEXT:    ret void
;
entry:
  %cmp.0 = icmp eq i32* %a, %b
  br i1 %cmp.0, label %for.end, label %for.body

for.body:
  %p.0 = phi i32* [ %inc.p, %for.else ], [ %a, %entry ], [ %b, %for.if ]
  %p.1 = phi i32 [ 42, %entry ], [ %p.1, %for.else ], [ %p.1, %for.if ]
  %0 = load i32, i32* %p.0, align 4
  %cmp.1 = icmp eq i32 %0, %p.1
  br i1 %cmp.1, label %for.if, label %for.else

for.if:
  br label %for.body

for.else:
  %inc.p = getelementptr inbounds i32, i32* %p.0, i64 1
  %cmp.2 = icmp eq i32* %inc.p, %b
  br i1 %cmp.2, label %for.end, label %for.body

for.end:
  %p.3 = phi i32 [%c, %entry], [%0, %for.else]
  store i32 %p.3, i32* %a
  ret void
}

