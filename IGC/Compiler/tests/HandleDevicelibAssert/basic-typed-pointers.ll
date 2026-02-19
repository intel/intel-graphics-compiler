;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-handle-devicelib-assert -S < %s 2>&1 | FileCheck %s

; Debug-info related check
;
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test() #0 {
entry:
  call spir_func void @__devicelib_assert_fail(i8 addrspace(4)* null, i8 addrspace(4)* null, i32 2, i8 addrspace(4)* null, i64 1, i64 2, i64 3, i64 1, i64 2, i64 3) #0
  ret void
}

; Function Attrs: alwaysinline builtin convergent nounwind
; CHECK: declare spir_func void @__devicelib_assert_fail(i8 addrspace(4)*, i8 addrspace(4)*, i32, i8 addrspace(4)*, i64, i64, i64, i64, i64, i64) #0
define spir_func void @__devicelib_assert_fail(i8 addrspace(4)* %expr, i8 addrspace(4)* %file, i32 %line, i8 addrspace(4)* %func, i64 %gid0, i64 %gid1, i64 %gid2, i64 %lid0, i64 %lid1, i64 %lid2) #0 {
entry:
; Function body generated e.g. by DPCPP would be here. Only ret in test to have it minimal.
  ret void
}

attributes #0 = { nounwind }
attributes #1 = { alwaysinline builtin convergent nounwind }
attributes #2 = { convergent nounwind }
