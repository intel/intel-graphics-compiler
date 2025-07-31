;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers -igc-programscope-constant-analysis -igc-serialize-metadata \
; RUN:   -S < %s | FileCheck %s
; ------------------------------------------------
; ProgramScopeConstantAnalysis
; ------------------------------------------------

; Test checks that metadata is updated with printf string, while there is reference to this
; string in another global.

; CHECK:     !{!"stringConstants",
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(2)* @str}

@str = internal unnamed_addr addrspace(2) constant [3 x i8] c"A\0A\00", align 1
@ref = addrspace(2) constant [1 x i64] [i64 ptrtoint ([3 x i8] addrspace(2)* @str to i64)]
; decl of opencl printf
declare spir_func i32 @printf(i8 addrspace(2)*, ...)

; Function Attrs: convergent noinline nounwind optnone
define spir_func void @foo() {
  %1 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @str, i64 0, i64 0
  %2 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %1)
  ret void
}
