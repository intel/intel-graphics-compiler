;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt --regkey "EnableOpaquePointersBackend=1" --igc-promote-bools -S < %s 2>&1 | FileCheck %s

; This test verifies that the pass can handle nested users of opaque pointer function calls
; without generating undef values and deleting said users unnecessarily.
; ------------------------------------------------
; NoUndefUsersOpaque
; ------------------------------------------------

; CHECK-LABEL: define spir_kernel void @test_kernel()
; CHECK: call spir_func ptr addrspace(4) @joint_helper(i8 1)
; CHECK-NOT: undef
; CHECK: ptrtoint ptr addrspace(4)
; CHECK: ret void

; CHECK-LABEL: define internal spir_func ptr addrspace(4) @joint_helper(i8 %flag)
; CHECK-NOT: i1
; CHECK: alloca i8, align 1, addrspace(4)
; CHECK: getelementptr inbounds i8, ptr addrspace(4)
; CHECK: ret ptr addrspace(4)

define spir_kernel void @test_kernel() #0 {
entry:
  %call  = call spir_func ptr addrspace(4) @joint_helper(i1 1) #0
  %bc    = bitcast ptr addrspace(4) %call to ptr addrspace(4)
  %asint = ptrtoint ptr addrspace(4) %bc to i64
  ret void
}

define internal spir_func ptr addrspace(4) @joint_helper(i1 %flag) #0 {
entry:
  %base = alloca i1, align 1, addrspace(4)
  %gep  = getelementptr inbounds i1, ptr addrspace(4) %base, i64 1
  ret ptr addrspace(4) %gep
}

attributes #0 = { nounwind }
