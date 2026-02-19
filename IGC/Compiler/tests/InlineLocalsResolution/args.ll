;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --enable-debugify -igc-resolve-inline-locals -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK
; ------------------------------------------------
; InlineLocalsResolution
; ------------------------------------------------

; Debug-info related checks
;
; For llvm 14 check-debugify treats missing debug location on function arguments substitution
; at the begining of BB as a warning, while on earlier llvm versions its treated as an error.
;
; CHECK: CheckModuleDebugify: PASS

; CHECK: [[EXT_SLM:@[A-z_-]*]] = external addrspace(3) global [0 x i8]

define spir_kernel void @test_inline(i32 addrspace(3)* %a, i32 addrspace(3)* %b) {
; CHECK-LABEL: @test_inline(
; CHECK:    [[LOCALTOCHAR:%.*]] = bitcast i32 addrspace(3)* [[B:%.*]] to i8 addrspace(3)*
; CHECK:    [[MOVEDLOCAL:%.*]] = getelementptr i8, i8 addrspace(3)* [[LOCALTOCHAR]], i32 0
; CHECK:    [[CHARTOLOCAL:%.*]] = bitcast i8 addrspace(3)* [[MOVEDLOCAL]] to i32 addrspace(3)*
; CHECK:    [[TMP1:%.*]] = load i32, i32 addrspace(3)* bitcast ([0 x i8] addrspace(3)* [[EXT_SLM]] to i32 addrspace(3)*), align 4
; CHECK:    [[TMP2:%.*]] = load i32, i32 addrspace(3)* [[CHARTOLOCAL]], align 4
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK:    store i32 [[TMP3]], i32 addrspace(3)* [[CHARTOLOCAL]], align 4
; CHECK:    ret void
;
  %1 = load i32, i32 addrspace(3)* %a, align 4
  %2 = load i32, i32 addrspace(3)* %b, align 4
  %3 = add i32 %1, %2
  store i32 %3, i32 addrspace(3)* %b, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(3)*, i32 addrspace(3)*)* @test_inline, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
