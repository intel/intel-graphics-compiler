;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s | FileCheck %s
; ------------------------------------------------
; Ensure that "intel_sub_group_ballot" is excluded from i1 to i8 promotion.
; ------------------------------------------------

; CHECK: define dso_local spir_func i32 @intel_sub_group_ballot(i1 noundef zeroext %0)
; CHECK: [[TMP0:%[0-9]+]] = tail call spir_func i32 @__builtin_IB_WaveBallot(i1

; Function Attrs: convergent norecurse nounwind
define dso_local spir_func i32 @intel_sub_group_ballot(i1 noundef zeroext %0) {
  %2 = tail call spir_func i32 @__builtin_IB_WaveBallot(i1 noundef zeroext %0)
  ret i32 %2
}

; CHECK: declare spir_func i32 @__builtin_IB_WaveBallot(i1 noundef zeroext)

; Function Attrs: convergent nounwind
declare spir_func i32 @__builtin_IB_WaveBallot(i1 noundef zeroext) local_unnamed_addr
