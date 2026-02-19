;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-memopt -instcombine | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @f0(i32* %dst, i32* %src) {
entry:
  %0 = load i32, i32* %src, align 4, !nontemporal !5
  %arrayidx1 = getelementptr inbounds i32, i32* %src, i64 1
  %1 = load i32, i32* %arrayidx1, align 4, !nontemporal !5
  %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 2
  %2 = load i32, i32* %arrayidx2, align 4, !nontemporal !5
  store i32 %0, i32* %dst, align 4, !nontemporal !5
  %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 1
  store i32 %1, i32* %arrayidx4, align 4, !nontemporal !5
  %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
  store i32 %2, i32* %arrayidx5, align 4, !nontemporal !5
  ret void
}

; CHECK-LABEL: @f0(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = bitcast i32* [[SRC:%.*]] to <3 x i32>*
; CHECK-NEXT:    [[TMP1:%.*]] = load <3 x i32>, <3 x i32>* [[TMP0]], align 4
; CHECK-SAME:    !nontemporal ![[NONTEMPORAL:[0-9]+]]
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast i32* [[DST:%.*]] to <3 x i32>*
; CHECK-NEXT:    store <3 x i32> [[TMP1]], <3 x i32>* [[TMP2]], align 4
; CHECK-SAME:    !nontemporal ![[NONTEMPORAL:[0-9]+]]
; CHECK-NEXT:    ret void


define void @f1(i32* %dst, i32* %src) {
entry:
  %0 = load i32, i32* %src, align 4
  %arrayidx1 = getelementptr inbounds i32, i32* %src, i64 1
  %1 = load i32, i32* %arrayidx1, align 4, !nontemporal !5
  %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 2
  %2 = load i32, i32* %arrayidx2, align 4
  store i32 %0, i32* %dst, align 4
  %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 1
  store i32 %1, i32* %arrayidx4, align 4
  %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
  store i32 %2, i32* %arrayidx5, align 4, !nontemporal !5
  ret void
}

; CHECK-LABEL: @f1(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = bitcast i32* [[SRC:%.*]] to <3 x i32>*
; CHECK-NEXT:    [[TMP1:%.*]] = load <3 x i32>, <3 x i32>* [[TMP0]], align 4
; CHECK-SAME:    !nontemporal ![[NONTEMPORAL:[0-9]+]]
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast i32* [[DST:%.*]] to <3 x i32>*
; CHECK-NEXT:    store <3 x i32> [[TMP1]], <3 x i32>* [[TMP2]], align 4
; CHECK-SAME:    !nontemporal ![[NONTEMPORAL:[0-9]+]]
; CHECK-NEXT:    ret void

define void @f2(i32* %dst, i32* %src) {
entry:
  %0 = load i32, i32* %src, align 4
  %arrayidx1 = getelementptr inbounds i32, i32* %src, i64 1
  %1 = load i32, i32* %arrayidx1, align 4
  %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 2
  %2 = load i32, i32* %arrayidx2, align 4
  store i32 %0, i32* %dst, align 4
  %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 1
  store i32 %1, i32* %arrayidx4, align 4
  %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
  store i32 %2, i32* %arrayidx5, align 4
  ret void
}

; CHECK-LABEL: @f2(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = bitcast i32* [[SRC:%.*]] to <3 x i32>*
; CHECK-NEXT:    [[TMP1:%.*]] = load <3 x i32>, <3 x i32>* [[TMP0]], align 4
; CHECK-NOT:     !nontemporal ![[NONTEMPORAL:[0-9]+]]
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast i32* [[DST:%.*]] to <3 x i32>*
; CHECK-NEXT:    store <3 x i32> [[TMP1]], <3 x i32>* [[TMP2]], align 4
; CHECK-NOT:     !nontemporal ![[NONTEMPORAL:[0-9]+]]
; CHECK-NEXT:    ret void

; CHECK: ![[NONTEMPORAL]] = !{i32 1}

!igc.functions = !{!0, !3, !4}

!0 = !{void (i32*, i32*)* @f0, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i32*, i32*)* @f1, !1}
!4 = !{void (i32*, i32*)* @f2, !1}
!5 = !{i32 1}
