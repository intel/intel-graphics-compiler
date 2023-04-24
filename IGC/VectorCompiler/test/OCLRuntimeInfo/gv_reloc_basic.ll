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
; CHECK-NOT: ,
@const.array = addrspace(2) constant [4 x i8] [i8 42, i8 43, i8 44, i8 45], align 1
; CHECK-SAME: 42, 43, 44, 45,
; CHECK-NOT: ,
@const.char = addrspace(2) constant i8 142, align 1
; CHECK-SAME: 142,
; CHECK-NOT: ,
; COM: padding
; CHECK-SAME: 0, 0, 0,
; CHECK-NOT: ,
@rel.gep.array = addrspace(2) constant [2 x i8 addrspace(2)*] [i8 addrspace(2)* getelementptr inbounds ([4 x i8], [4 x i8] addrspace(2)* @const.array, i32 0, i32 0), i8 addrspace(2)* getelementptr inbounds ([4 x i8], [4 x i8] addrspace(2)* @const.array, i32 0, i32 3)], align 8
; COM: first relocation with 0 offset
; CHECK-SAME: 0, 0, 0, 0, 0, 0,
; CHECK-NOT: ,
; COM: yaml serializer inserts new line here
; CHECK-NEXT: 0, 0,
; CHECK-NOT: ,
; COM: second relocation with offset 3
; CHECK-SAME: 3, 0, 0, 0, 0, 0, 0, 0,
; CHECK-NOT: ,
@rel.gv = addrspace(2) constant i8 addrspace(2)* @const.char, align 8
; CHECK-SAME: 0, 0, 0, 0, 0, 0,
; CHECK-NOT: ,
; COM: yaml serializer inserts new line here
; CHECK-NEXT: 0, 0
; CHECK-NOT: ,
; CHECK-SAME: ]
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   4
; CHECK-NEXT:   s_name:   const.array
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 4
; CHECK-NEXT:   s_size:   1
; CHECK-NEXT:   s_name:   const.char
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 8
; CHECK-NEXT:   s_size:   16
; CHECK-NEXT:   s_name:   rel.gep.array
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR_CONST
; CHECK-NEXT:   s_offset: 24
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.gv
; CHECK-NOT: s_
; CHECK: Relocations:
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 8
; CHECK-NEXT:   r_symbol: const.array
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 16
; CHECK-NEXT:   r_symbol: const.array
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 24
; CHECK-NEXT:   r_symbol: const.char
; CHECK-NOT: r_

; CHECK: Global:
; CHECK: Data:
; CHECK: Buffer: [
; CHECK-NOT: ,
@rel.cross = addrspace(1) global i8 addrspace(2)* addrspace(2)* getelementptr inbounds ([2 x i8 addrspace(2)*], [2 x i8 addrspace(2)*] addrspace(2)* @rel.gep.array, i32 0, i32 1), align 8
; CHECK-SAME: 8, 0, 0, 0, 0, 0, 0, 0,
@rel.mixed = addrspace(1) global [2 x i64 addrspace(1)*] [i64 addrspace(1)* @global.a, i64 addrspace(1)* @global.b], align 8
@global.a = addrspace(1) global i64 10, align 8
@global.b = addrspace(1) global i64 11, align 8
; CHECK: Symbols:
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 0
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   rel.cross
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 8
; CHECK-NEXT:   s_size:   16
; CHECK-NEXT:   s_name:   rel.mixed
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 24
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   global.a
; CHECK-NEXT: - s_type:   S_GLOBAL_VAR
; CHECK-NEXT:   s_offset: 32
; CHECK-NEXT:   s_size:   8
; CHECK-NEXT:   s_name:   global.b
; CHECK-NOT: s_
; CHECK: Relocations:
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 0
; CHECK-NEXT:   r_symbol: rel.gep.array
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 8
; CHECK-NEXT:   r_symbol: global.a
; CHECK-NEXT: - r_type:   R_SYM_ADDR
; CHECK-NEXT:   r_offset: 16
; CHECK-NEXT:   r_symbol: global.b
; CHECK-NOT: r_

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
