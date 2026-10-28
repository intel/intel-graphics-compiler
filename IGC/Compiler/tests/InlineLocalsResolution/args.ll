;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers --enable-debugify -igc-resolve-inline-locals -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers --enable-debugify -igc-resolve-inline-locals -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}
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
; CHECK-TYPED:    [[LOCALTOCHAR:%.*]] = bitcast i32 addrspace(3)* [[B:%.*]] to i8 addrspace(3)*
; CHECK-OPAQUE:    [[LOCALTOCHAR:%.*]] = bitcast ptr addrspace(3) [[B:%.*]] to ptr addrspace(3)
; CHECK-TYPED:    [[MOVEDLOCAL:%.*]] = getelementptr i8, i8 addrspace(3)* [[LOCALTOCHAR]], i32 0
; CHECK-OPAQUE:    [[MOVEDLOCAL:%.*]] = getelementptr i8, ptr addrspace(3) [[LOCALTOCHAR]], i32 0
; CHECK-TYPED:    [[CHARTOLOCAL:%.*]] = bitcast i8 addrspace(3)* [[MOVEDLOCAL]] to i32 addrspace(3)*
; CHECK-OPAQUE:    [[CHARTOLOCAL:%.*]] = bitcast ptr addrspace(3) [[MOVEDLOCAL]] to ptr addrspace(3)
; CHECK-TYPED:    [[TMP1:%.*]] = load i32, i32 addrspace(3)* bitcast ([0 x i8] addrspace(3)* [[EXT_SLM]] to i32 addrspace(3)*), align 4
; CHECK-OPAQUE:    [[TMP1:%.*]] = load i32, ptr addrspace(3) [[EXT_SLM]], align 4
; CHECK-TYPED:    [[TMP2:%.*]] = load i32, i32 addrspace(3)* [[CHARTOLOCAL]], align 4
; CHECK-OPAQUE:    [[TMP2:%.*]] = load i32, ptr addrspace(3) [[CHARTOLOCAL]], align 4
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK-TYPED:    store i32 [[TMP3]], i32 addrspace(3)* [[CHARTOLOCAL]], align 4
; CHECK-OPAQUE:    store i32 [[TMP3]], ptr addrspace(3) [[CHARTOLOCAL]], align 4
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
