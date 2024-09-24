;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: phi i1
; ------------------------------------------------

; Checks legalization of phi with i1 type
; to new phi with i32 type and icmp

define i1 @test_phi(i32 %a, i32 %b, i1 %c) {
; CHECK-LABEL: define i1 @test_phi(
; CHECK-SAME: i32 [[A:%.*]], i32 [[B:%.*]], i1 [[C:%.*]]) {
; CHECK:  [[ENTRY:.*:]]
; CHECK:    br i1 [[C]], label %[[BL1:.*]], label %[[BL2:.*]]
; CHECK:  [[BL1]]:
; CHECK:    [[TMP0:%.*]] = icmp ne i32 [[A]], 13
; CHECK:    [[TMP1:%.*]] = sext i1 [[TMP0]] to i32
; CHECK:    br label %[[END:.*]]
; CHECK:  [[BL2]]:
; CHECK:    [[TMP2:%.*]] = icmp ne i32 [[B]], 144
; CHECK:    [[TMP3:%.*]] = sext i1 [[TMP2]] to i32
; CHECK:    br label %[[END]]
; CHECK:  [[END]]:
; CHECK:    [[TMP4:%.*]] = phi i32 [ [[TMP1]], %[[BL1]] ], [ [[TMP3]], %[[BL2]] ]
; CHECK:    [[TMP5:%.*]] = icmp ne i32 [[TMP4]], 0
; CHECK:    ret i1 [[TMP5]]
;
entry:
  br i1 %c, label %bl1, label %bl2
bl1:
  %0 = icmp ne i32 %a, 13
  br label %end
bl2:
  %1 = icmp ne i32 %b, 144
  br label %end
end:
  %2 = phi i1 [%0, %bl1], [%1, %bl2]
  ret i1 %2
}

!igc.functions = !{!0}

!0 = !{i1 (i32, i32, i1)* @test_phi, !1}
!1 = !{}
