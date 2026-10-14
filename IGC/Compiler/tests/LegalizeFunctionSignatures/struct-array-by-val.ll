;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-legalize-function-signatures -S < %s 2>&1 | FileCheck %s

; Struct and array values passed to and returned from subroutines, which SPIR-V
; allows, are passed in memory (byval and sret) if codegen can't handle them in
; registers: arrays and structs with aggregate members. Struct arguments are also
; passed in memory if a byval struct of the same type wouldn't be promoted to
; registers: structs with vector members or over 512 bits. Other structs stay in
; registers.

%flat = type { i64, i32 }
%vec = type { <4 x i32>, i32 }
%nested = type { i32, [2 x i32] }

; CHECK-LABEL: define spir_kernel void @kernel(%flat %f, %vec %v, %nested %n, [2 x i32] %a)
; CHECK:         call spir_func i32 @flat_arg(%flat %f)
; CHECK:         [[V:%.*]] = alloca %vec
; CHECK-NEXT:    store %vec %v, ptr [[V]]
; CHECK-NEXT:    call spir_func i32 @vec_arg(ptr byval(%vec) [[V]])
; CHECK:         [[N:%.*]] = alloca %nested
; CHECK-NEXT:    store %nested %n, ptr [[N]]
; CHECK-NEXT:    call spir_func i32 @nested_arg(ptr byval(%nested) [[N]])
; CHECK:         [[A:%.*]] = alloca [2 x i32]
; CHECK-NEXT:    store [2 x i32] %a, ptr [[A]]
; CHECK-NEXT:    call spir_func i32 @array_arg(ptr byval([2 x i32]) [[A]])
; CHECK:         call spir_func void @nested_ret(ptr noalias sret(%nested) {{%.*}}, ptr byval(%nested) {{%.*}})
define spir_kernel void @kernel(%flat %f, %vec %v, %nested %n, [2 x i32] %a) {
  %r0 = call spir_func i32 @flat_arg(%flat %f)
  %rv = call spir_func i32 @vec_arg(%vec %v)
  %r1 = call spir_func i32 @nested_arg(%nested %n)
  %r2 = call spir_func i32 @array_arg([2 x i32] %a)
  %r3 = call spir_func %nested @nested_ret(%nested %n)
  ret void
}

; CHECK-LABEL: define internal spir_func i32 @flat_arg(%flat %s)
; CHECK-NEXT:    extractvalue %flat %s, 1
define internal spir_func i32 @flat_arg(%flat %s) {
  %x = extractvalue %flat %s, 1
  ret i32 %x
}

; CHECK-LABEL: define internal spir_func i32 @vec_arg(ptr %s)
; CHECK-NEXT:    [[S:%.*]] = load %vec, ptr %s
; CHECK-NEXT:    extractvalue %vec [[S]], 1
define internal spir_func i32 @vec_arg(%vec %s) {
  %x = extractvalue %vec %s, 1
  ret i32 %x
}

; CHECK-LABEL: define internal spir_func i32 @nested_arg(ptr %s)
; CHECK-NEXT:    [[S:%.*]] = load %nested, ptr %s
; CHECK-NEXT:    extractvalue %nested [[S]], 1, 1
define internal spir_func i32 @nested_arg(%nested %s) {
  %x = extractvalue %nested %s, 1, 1
  ret i32 %x
}

; CHECK-LABEL: define internal spir_func i32 @array_arg(ptr %s)
; CHECK-NEXT:    [[S:%.*]] = load [2 x i32], ptr %s
; CHECK-NEXT:    extractvalue [2 x i32] [[S]], 1
define internal spir_func i32 @array_arg([2 x i32] %s) {
  %x = extractvalue [2 x i32] %s, 1
  ret i32 %x
}

; CHECK-LABEL: define internal spir_func void @nested_ret(ptr noalias sret(%nested) {{%.*}}, ptr %s)
; CHECK-NEXT:    [[S:%.*]] = load %nested, ptr %s
define internal spir_func %nested @nested_ret(%nested %s) {
  ret %nested %s
}

!igc.functions = !{!0}
!0 = !{ptr @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
