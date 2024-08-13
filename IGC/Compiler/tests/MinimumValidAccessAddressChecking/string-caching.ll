;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-minimum-valid-address-checking -igc-minimum-valid-address-checking_arg 0x100 -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_kernel void @kernel(i32 addrspace(1)* %input) nounwind {
  %1 = getelementptr inbounds i32, i32 addrspace(1)* %input, i64 2
  store i32 42, i32 addrspace(1)* %1
  %2 = getelementptr inbounds i32, i32 addrspace(1)* %input, i64 4
  store i32 314, i32 addrspace(1)* %2
  ret void
}

!llvm.module.flags = !{!0, !1, !2}
!llvm.dbg.cu = !{!4}
!igc.functions = !{!11}

!0 = !{i32 7, !"Dwarf Version", i32 5}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = !{}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !3)
!5 = !DIFile(filename: "string-caching.ll", directory: "/")
!11 = !{void (i32 addrspace(1)*)* @kernel, !12}
!12 = !{!13}
!13 = !{!"function_type", i32 0}

; CHECK:        @0 = internal unnamed_addr addrspace(2) constant [18 x i8] c"string-caching.ll\00"
; CHECK-NOT:    @1 = internal unnamed_addr addrspace(2) constant [18 x i8] c"string-caching.ll\00"
