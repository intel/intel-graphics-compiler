;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXAggregatePseudoLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; CHECK-LABEL: define i32 @simple_cmpxchg
define i32 @simple_cmpxchg(i32 addrspace(1)* %ptr) {
; CHECK: %res = cmpxchg i32 addrspace(1)* %ptr, i32 0, i32 1 syncscope("all_devices") acq_rel acquire
; CHECK-NEXT: %extr = extractvalue { i32, i1 } %res, 0
; CHECK-NEXT: ret i32 %extr
  %res = cmpxchg i32 addrspace(1)* %ptr, i32 0, i32 1 syncscope("all_devices") acq_rel acquire
  %extr = extractvalue { i32, i1 } %res, 0
  ret i32 %extr
}


