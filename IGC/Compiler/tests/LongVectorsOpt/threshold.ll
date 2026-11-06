;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s --check-prefix=SPLIT
; RUN: igc_opt --opaque-pointers -regkey LongVectorsOptThreshold=255 -igc-long-vectors-opt -S < %s | FileCheck %s --check-prefix=SPLIT
; RUN: igc_opt --opaque-pointers -regkey LongVectorsOptThreshold=256 -igc-long-vectors-opt -S < %s | FileCheck %s --check-prefix=KEEP

; Only vectors bigger than LongVectorsOptThreshold bytes are processed. <64 x float> is 256 bytes.

; SPLIT-LABEL: @threshold(
; SPLIT-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i32 %j
; SPLIT-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
; SPLIT-NEXT:    ret float [[E]]
; KEEP-LABEL:  @threshold(
; KEEP-NEXT:     %v = load <64 x float>, ptr addrspace(1) %p, align 4
; KEEP-NEXT:     %e = extractelement <64 x float> %v, i32 %j
define float @threshold(ptr addrspace(1) %p, i32 %j) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}
