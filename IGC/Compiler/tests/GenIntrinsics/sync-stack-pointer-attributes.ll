;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -S %s | FileCheck %s --implicit-check-not=noduplicate

; igc_opt restores canonical GenISA attributes without any optimization passes.
; SyncStackPtr marks a pointer and can be duplicated by loop unswitching. Keep
; its no-memory-access, no-unwind and termination contracts.
; CHECK: declare ptr addrspace(1) @llvm.genx.GenISA.SyncStackPtr{{.*}}(ptr addrspace(1)) #[[ATTR:[0-9]+]]
; CHECK: attributes #[[ATTR]] = { {{(nounwind readnone willreturn|nounwind willreturn memory\(none\))}} }

declare ptr addrspace(1) @llvm.genx.GenISA.SyncStackPtr.p1.p1(ptr addrspace(1))

define ptr addrspace(1) @sync_stack_pointer(ptr addrspace(1) %base) {
entry:
  %stack = call ptr addrspace(1) @llvm.genx.GenISA.SyncStackPtr.p1.p1(ptr addrspace(1) %base)
  ret ptr addrspace(1) %stack
}
