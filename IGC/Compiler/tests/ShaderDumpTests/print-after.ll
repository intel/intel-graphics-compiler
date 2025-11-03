;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: rm -rf %t && mkdir %t
; RUN: igc_opt %s -S -igc-clone-address-arithmetic --regkey=PrintAfter=CloneAddressArithmetic --regkey=DumpToCustomDir=%t
; RUN: find %t -type f | FileCheck %s

; RUN: rm -rf %t && mkdir %t
; RUN: igc_opt %s -S -igc-clone-address-arithmetic --regkey=PrintAfter="Clone Address Arithmetic" --regkey=DumpToCustomDir=%t
; RUN: find %t -type f | FileCheck %s

; CHECK: OCL_0000_igc_opt_after_CloneAddressArithmetic.ll

define void @main() {
  ret void
}
