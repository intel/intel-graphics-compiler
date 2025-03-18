;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check inlined __kmpc_barrier is removed from igc.functions and FuncMD metadata.

; RUN: igc_opt -SubroutineInliner -S < %s 2>&1 | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @__omp_offloading_MAIN(i8 addrspace(2)* %constBase, i8 addrspace(1)* %globalBase) {
DIR.OMP.END.LOOP.4:
  call spir_func void @__kmpc_barrier(i8 addrspace(2)* %constBase, i8 addrspace(1)* %globalBase)
  ret void
}

define internal spir_func void @__kmpc_barrier(i8 addrspace(2)* nocapture readnone %constBase, i8 addrspace(1)* nocapture readnone %globalBase) {
entry:
  ret void
}

; CHECK: !igc.functions = !{[[F:![0-9]+]]}
; CHECK: [[F]] = !{{{.*}} @__omp_offloading_MAIN
; CHECK-NOT: = !{{{.*}} @__kmpc_barrier
; CHECK: !{!"FuncMDMap[0]", {{.*}} @__omp_offloading_MAIN}
; CHECK: !{!"FuncMDMap[1]", null}

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3, !9}
!IGCMetadata = !{!12}
!opencl.ocl.version = !{!18}
!opencl.spir.version = !{!18}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 200000}
!2 = !{i16 6, i16 14}
!3 = !{void (i8 addrspace(2)*, i8 addrspace(1)*)* @__omp_offloading_MAIN, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8}
!7 = !{i32 11}
!8 = !{i32 12}
!9 = !{void (i8 addrspace(2)*, i8 addrspace(1)*)* @__kmpc_barrier, !10}
!10 = !{!11, !6}
!11 = !{!"function_type", i32 2}
!12 = !{!"ModuleMD", !13}
!13 = !{!"FuncMD", !14, !15, !16, !17}
!14 = !{!"FuncMDMap[0]", void (i8 addrspace(2)*, i8 addrspace(1)*)* @__omp_offloading_MAIN}
!15 = !{!"FuncMDValue[0]"}
!16 = !{!"FuncMDMap[1]", void (i8 addrspace(2)*, i8 addrspace(1)*)* @__kmpc_barrier}
!17 = !{!"FuncMDValue[1]"}
!18 = !{i32 2, i32 0}
