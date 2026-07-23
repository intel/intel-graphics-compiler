;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-programscope-constant-analysis -igc-serialize-metadata \
; RUN:   -S < %s | FileCheck %s
; ------------------------------------------------
; ProgramScopeConstantAnalysis
; ------------------------------------------------

; Below LLVM IR was produced from the following OpenCL C code:
;
; kernel void test()
; {
;    printf("%s","foo\0foo");
; }

; CHECK: !{!"stringConstants", ![[FORMAT_STR_MD:.*]], ![[FOO_STR_MD:.*]]}
; CHECK: ![[FORMAT_STR_MD]] = !{!"stringConstantsSet[0]", [3 x i8] addrspace(2)* @.str}
; CHECK: ![[FOO_STR_MD]] = !{!"stringConstantsSet[1]", [8 x i8] addrspace(2)* @.str.1}

@.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%s\00", align 1
@.str.1 = internal unnamed_addr addrspace(2) constant [8 x i8] c"foo\00foo\00", align 1

define spir_kernel void @test() {
entry:
  %0 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @.str, i64 0, i64 0
  %1 = getelementptr inbounds [8 x i8], [8 x i8] addrspace(2)* @.str.1, i64 0, i64 0
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %0, i8 addrspace(2)* %1)
  ret void
}

declare i32 @printf(i8 addrspace(2)*, ...)
