;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers -igc-builtin-import -disable-verify -S < %s | FileCheck %s
; ------------------------------------------------
; BIImport
; ------------------------------------------------
; This test checks that BIImport fixes a direct builtin call whose pointer-
; argument address space (e.g. AS 1) differs from the builtin’s prototype (e.g. AS 4),
; cloning a helper with the right address space and rewriting the call.
; ------------------------------------------------

; CHECK: call void @someBuiltin.{{[0-9]+}}(ptr addrspace(1) %a)
; CHECK: define void @someBuiltin.{{[0-9]+}}(ptr addrspace(1) %a)

define void @someBuiltin(ptr addrspace(4) %a) {
entry:
  ret void
}

define spir_func void @kernel(ptr addrspace(1) %a) {
entry:
  call void @someBuiltin(ptr addrspace(1) %a)
  ret void
}
