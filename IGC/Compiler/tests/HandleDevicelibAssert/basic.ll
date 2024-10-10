;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -enable-debugify -igc-handle-devicelib-assert -S < %s 2>&1 | FileCheck %s

; Debug-info related check
;
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test() #0 {
entry:
  call spir_func void @__devicelib_assert_fail(ptr addrspace(4) null, ptr addrspace(4) null, i32 2, ptr addrspace(4) null, i64 1, i64 2, i64 3, i64 1, i64 2, i64 3) #0
  ret void
}

; Function Attrs: alwaysinline builtin convergent nounwind
; CHECK: declare spir_func void @__devicelib_assert_fail(ptr addrspace(4), ptr addrspace(4), i32, ptr addrspace(4), i64, i64, i64, i64, i64, i64) #0
define spir_func void @__devicelib_assert_fail(ptr addrspace(4) %expr, ptr addrspace(4) %file, i32 %line, ptr addrspace(4) %func, i64 %gid0, i64 %gid1, i64 %gid2, i64 %lid0, i64 %lid1, i64 %lid2) #0 {
entry:
; Function body generated e.g. by DPCPP would be here. Only ret in test to have it minimal.
  ret void
}

attributes #0 = { nounwind }
attributes #1 = { alwaysinline builtin convergent nounwind }
attributes #2 = { convergent nounwind }
