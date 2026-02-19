;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -indirect-call-optimization -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; IndirectCallOptimization
; ------------------------------------------------

; Test checks that indirect calls are replaced with direct ones

@a = internal global i32 0, align 8
@fptr = internal global i32 (i32)* @f1, align 8

; One callee case:

define i32 @one_callee(i32 %s1) {
; CHECK-LABEL: @one_callee(
; CHECK:    [[TMP1:%.*]] = call i32 @f1(i32 %s1)
; CHECK:    ret i32 [[TMP1]]
;
entry:
  %0 = load i32 (i32)*, i32 (i32)** @fptr, align 8
  %call = call i32 %0(i32 %s1), !callees !0
  ret i32 %call
}

define internal i32 @f1(i32 %s1) "referenced-indirectly" {
entry:
  %sub = sub i32 %s1, 1
  ret i32 %sub
}

; Multiple callee case:

define i32 @two_callee(i32 %s1) {
; CHECK-LABEL: @two_callee(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = load i32 (i32)*, i32 (i32)** @fptr, align 8
; CHECK-NEXT:    [[TMP1:%.*]] = ptrtoint i32 (i32)* [[TMP0]] to i64
; CHECK-NEXT:    [[TMP2:%.*]] = icmp eq i64 [[TMP1]], ptrtoint (i32 (i32)* @f1 to i64)
; CHECK-NEXT:    br i1 [[TMP2]], label %[[FUNCTHEN:.*]], label %[[FUNCELSE:.*]]
; CHECK:       [[FUNCTHEN]]:
; CHECK-NEXT:    [[TMP3:%.*]] = call i32 @f1(i32 %s1)
; CHECK-NEXT:    br label %[[ENDINDIRECTCALLBB:.*]]
; CHECK:       [[FUNCELSE]]:
; CHECK-NEXT:    [[TMP4:%.*]] = icmp eq i64 [[TMP1]], ptrtoint (i32 (i32)* @f2 to i64)
; CHECK-NEXT:    br i1 [[TMP4]], label %[[FUNCTHEN1:.*]], label %[[FUNCELSE2:.*]]
; CHECK:       [[FUNCTHEN1]]:
; CHECK-NEXT:    [[TMP5:%.*]] = call i32 @f2(i32 %s1)
; CHECK-NEXT:    br label %[[ENDINDIRECTCALLBB]]
; CHECK:       [[FUNCELSE2]]:
; CHECK-NEXT:    br label %[[ENDINDIRECTCALLBB]]
; CHECK:       [[ENDINDIRECTCALLBB]]:
; CHECK-NEXT:    [[TMP6:%.*]] = phi i32 [ [[TMP3]], %[[FUNCTHEN]] ], [ [[TMP5]], %[[FUNCTHEN1]] ], [ undef, %[[FUNCELSE2]] ]
; CHECK-NEXT:    ret i32 [[TMP6]]
;
entry:
  %0 = load i32 (i32)*, i32 (i32)** @fptr, align 8
  %call = call i32 %0(i32 %s1), !callees !1
  ret i32 %call
}

define internal i32 @f2(i32 %s1) "referenced-indirectly" {
entry:
  store i32 %s1, i32* @a
  ret i32 1
}

!0 = !{i32 (i32)* @f1}
!1 = !{i32 (i32)* @f1, i32 (i32)* @f2}
