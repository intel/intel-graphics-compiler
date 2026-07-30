;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; CreateVisaLabelName sanitizes each byte of a label (basic block) name with
; isalnum(). A non-ASCII byte has the high bit set, so as a signed char it is
; negative; passing a negative value (other than EOF) to isalnum() is undefined
; behavior and asserts under a checked CRT. The block below is a loop target
; (back-edge), so it cannot be folded away and its name is guaranteed to reach
; CreateVisaLabelName.
;
;   * Debug/checked build: reverting the fix (signed char) asserts inside
;     isalnum() when the negative byte is passed -> igc_opt aborts -> test fails.
;   * Release build: the CHECK verifies the non-ASCII bytes were converted to
;     '_'. Block name bytes are "lp" + \C3\A9 ("é", 2 bytes) -> "lp__".

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -platformdg2 -igc-emit-visa %s -regkey DumpVISAASMToConsole=1 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; The sanitized label must contain "lp__" (é -> two underscores), never the
; raw non-ASCII bytes.
; CHECK: {{_test_[0-9]+_lp__}}

define spir_kernel void @test() #0 {
entry:
  br label %"lp\C3\A9"

"lp\C3\A9":
  %i = phi i32 [ 0, %entry ], [ %inc, %"lp\C3\A9" ]
  %inc = add i32 %i, 1
  %cmp = icmp slt i32 %inc, 4
  br i1 %cmp, label %"lp\C3\A9", label %ex

ex:
  ret void
}

attributes #0 = { null_pointer_is_valid }

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test}
!6 = !{!"FuncMDValue[0]"}
