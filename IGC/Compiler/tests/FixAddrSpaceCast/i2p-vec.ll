;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; COM: check that vector of pointers compiles wo assert
; RUN: igc_opt --opaque-pointers --igc-addrspacecast-fix -S < %s
; ------------------------------------------------
; FixAddrSpaceCast
; ------------------------------------------------

define spir_kernel void @test(<8 x i32> %payloadHeader) {
entry:
  %C = inttoptr <8 x i32> undef to <8 x ptr>
  store <8 x ptr> %C, ptr null, align 64
  ret void
}

