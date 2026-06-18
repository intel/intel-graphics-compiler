;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test generates DWARF debug info to test unwinding stack for prologue and epilogue for platforms supporting 64-bit addressing.
; For prologue we emit DW_CFA_val_expression with DW_OP_INTEL_regval_bits 64 in CIE block.
; For epilogue we emit DW_CFA_restore for r0 in FDE block to indicate that it is restored before return instruction.

; UNSUPPORTED: sys32
; REQUIRES: regkeys, oneapi-readelf, llvm-16-plus, cri-supported

; RUN: llvm-as %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device cri -internal_options "-cl-intel-use-bindless-mode -cl-intel-use-bindless-advanced-mode -ze-intel-has-buffer-offset-arg -cl-intel-greater-than-4GB-buffer-required -cl-store-cache-default=2 -cl-load-cache-default=4 -ze-intel-64bit-addressing" -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, TotalGRFNum=128, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd16_kernel.elf | FileCheck %s

; CHECK: Contents of the .debug_frame section:
; CHECK: [[#%x,CIE_ID:]] 000000000000013{{.+}} ffffffffffffffff CIE
; CHECK: DW_CFA_val_expression: r0 (rax) (DW_OP_drop; DW_OP_const4u: 143; DW_OP_const2u: 128; DW_OP_INTEL_regval_bits: 64)

; CHECK: {{.+}} FDE cie=[[#%.8x,CIE_ID]] pc={{.+}}..{{.+}}
; CHECK: DW_CFA_restore: r0 (rax)

; Function Attrs: convergent noinline nounwind optnone
define spir_func i32 @func(i32 %x) #2 !dbg !100 {
entry:
  %add = add nsw i32 %x, 5, !dbg !101
  ret i32 %add, !dbg !102
}

; Function Attrs: convergent nounwind optnone
define spir_kernel void @kernel(i32 addrspace(1)* %output) #0 !dbg !200 {
entry:
  %call = call spir_func i32 @func(i32 7) #5, !dbg !201
  store i32 %call, i32 addrspace(1)* %output, align 4, !dbg !202
  ret void, !dbg !203
}

attributes #0 = { convergent nounwind }
attributes #2 = { convergent noinline nounwind optnone "visaStackCall" }
attributes #5 = { convergent }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !1, producer: "clang version 16.0.0", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "test.cpp", directory: "/tmp")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!100 = distinct !DISubprogram(name: "func", scope: !1, file: !1, line: 1, type: !103, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!101 = !DILocation(line: 2, column: 12, scope: !100)
!102 = !DILocation(line: 2, column: 5, scope: !100)
!103 = !DISubroutineType(types: !2)
!200 = distinct !DISubprogram(name: "kernel", scope: !1, file: !1, line: 5, type: !103, scopeLine: 5, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!201 = !DILocation(line: 6, column: 14, scope: !200)
!202 = !DILocation(line: 6, column: 12, scope: !200)
!203 = !DILocation(line: 7, column: 1, scope: !200)
