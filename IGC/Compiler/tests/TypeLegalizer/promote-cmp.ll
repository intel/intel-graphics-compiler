;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-type-legalizer -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: icmp (i40 -> i64 promotion mask correctness)
; ------------------------------------------------
;
; Ensure that unsigned icmp on an illegal integer width (i40) is promoted
; with a correct low-40-bit mask (0xFFFFFFFFFF = 1099511627775).
;
; CHECK-LABEL: define i1 @test_icmp_i40
; CHECK: %[[TRUNC:.*]] = and i64 %[[X:.*]], 1099511627775
; CHECK: %[[MASKED:.*]] = and i64 %[[TRUNC]], 1099511627775
; CHECK: icmp ult i64 %[[MASKED]], 4294967296
; CHECK: ret i1

define i1 @test_icmp_i40(i64 %x) {
  ; Truncate to illegal width i40, then compare with 1<<32
  %t = trunc i64 %x to i40
  %cmp = icmp ult i40 %t, 4294967296
  ret i1 %cmp
}
