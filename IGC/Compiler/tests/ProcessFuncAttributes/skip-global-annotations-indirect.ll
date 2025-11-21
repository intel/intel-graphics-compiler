;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-process-func-attributes -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------

; When pointer to function is in @llvm.global.annotations we should not
; consider this user when applying "referenced-indirectly" attribute.

@0 = private unnamed_addr constant [9 x i8] zeroinitializer, section "llvm.metadata"
@llvm.global.annotations = appending global [1 x { i8*, i8*, i8*, i32 }] [{ i8*, i8*, i8*, i32 } { i8* bitcast (void ()* @func to i8*), i8* getelementptr inbounds ([9 x i8], [9 x i8]* @0, i32 0, i32 0), i8* undef, i32 undef }], section "llvm.metadata"

define internal spir_func void @func() #0 {
  ret void
}

define spir_kernel void @kernel() {
  call void @func()
  ret void
}

; CHECK-NOT: "referenced-indirectly"
attributes #0 = { nounwind }

!igc.functions = !{!1}
!1 = !{void ()* @kernel, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
