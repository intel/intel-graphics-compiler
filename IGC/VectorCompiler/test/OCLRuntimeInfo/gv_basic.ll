;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 -mattr=+ocl_runtime -vc-analyze=GenXOCLRuntimeInfo \
; RUN: -vc-choose-pass-manager-override=false -o /dev/null 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK: Printing analysis 'GenXOCLRuntimeInfo':

; CHECK: ModuleInfo:
; CHECK: Constant
; CHECK: Data:
; CHECK: Buffer: [
@const.int.32 = addrspace(2) constant i32 65536, align 1
; CHECK-SAME: 0, 0, 1, 0,
; CHECK-NOT: ,
@const.int.8 = addrspace(2) constant i8 7, align 1
; CHECK-SAME: 7
; CHECK-NOT: ,
; CHECK-SAME: ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   4
; CHECK-NEXT:   s_name:   const.int.32
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 4
; CHECK-NEXT:   s_size:   1
; CHECK-NEXT:   s_name:   const.int.8

; CHECK: Global:
; CHECK: Data:
; CHECK: Buffer: [
@global.int.16 = addrspace(1) global i16 256, align 1
; CHECK-SAME: 0, 1,
; CHECK-NOT: ,
@global.int.8 = addrspace(1) global i8 15, align 1
; CHECK-SAME: 15
; CHECK-NOT: ,
; CHECK-SAME: ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   2
; CHECK-NEXT:   s_name:   global.int.16
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 2
; CHECK-NEXT:   s_size:   1
; CHECK-NEXT:   s_name:   global.int.8

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test_kernel(i64 %privBase) local_unnamed_addr #0 {
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" "VCFunction" "VCNamedBarrierCount"="0" "VCSLMSize"="0" }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!opencl.enable.FP_CONTRACT = !{}
!genx.kernels = !{!6}
!genx.kernel.internal = !{!10}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "kernel_genx.cpp", directory: "/the_directory/")
!4 = !{}
!5 = !{}
!6 = !{void (i64)* @test_kernel, !"test_kernel", !7, i32 0, !8, !5, !9, i32 0}
!7 = !{i32 96}
!8 = !{i32 0}
!9 = !{}
!10 = !{void (i64)* @test_kernel, !11, !11, null, !11}
!11 = !{i32 0}
