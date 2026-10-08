;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt --opaque-pointers --regkey DumpHasNonKernelArgLdSt=1 --regkey EnableOptionalBufferOffset=1 --regkey EnableSupportBufferOffset=1 --regkey EnableStatefulAtomic=1 -igc-stateless-to-stateful-resolution -igc-serialize-metadata -S < %s | FileCheck %s
; ------------------------------------------------
; StatelessToStateful
; ------------------------------------------------

; Pointers made with inttoptr aren't derived from a buffer kernel argument, so
; the accesses stay stateless and are reported as non-kernel-arg ones.

declare i32 @llvm.genx.GenISA.LSCLoad.i32.p1(ptr addrspace(1), i32, i32, i32, i32)
declare void @llvm.genx.GenISA.LSCStore.p1.i32(ptr addrspace(1), i32, i32, i32, i32, i32)
declare i32 @llvm.genx.GenISA.intatomicraw.i32.p1(ptr addrspace(1), i32, i32, i32)

define spir_kernel void @test_func(i64 %a1, i64 %a2) {
; CHECK-LABEL: @test_func(
; CHECK:         call i32 @llvm.genx.GenISA.LSCLoad.i32.p1(ptr addrspace(1) %s2,
; CHECK-NEXT:    call i32 @llvm.genx.GenISA.intatomicraw.i32.p1(ptr addrspace(1) %s1,
; CHECK-NEXT:    call void @llvm.genx.GenISA.LSCStore.p1.i32(ptr addrspace(1) %s2,
entry:
  %s1 = inttoptr i64 %a1 to ptr addrspace(1)
  %s2 = inttoptr i64 %a2 to ptr addrspace(1)
  %0 = call i32 @llvm.genx.GenISA.LSCLoad.i32.p1(ptr addrspace(1) %s2, i32 0, i32 3, i32 1, i32 2)
  %1 = call i32 @llvm.genx.GenISA.intatomicraw.i32.p1(ptr addrspace(1) %s1, i32 0, i32 %0, i32 0)
  call void @llvm.genx.GenISA.LSCStore.p1.i32(ptr addrspace(1) %s2, i32 0, i32 %1, i32 3, i32 1, i32 2)
  ret void
}

; CHECK-DAG: !{{[0-9]*}} = !{!"hasNonKernelArgLoad", i1 true}
; CHECK-DAG: !{{[0-9]*}} = !{!"hasNonKernelArgStore", i1 true}
; CHECK-DAG: !{{[0-9]*}} = !{!"hasNonKernelArgAtomic", i1 true}

!IGCMetadata = !{!0}
!igc.functions = !{!281}

!0 = !{!"ModuleMD", !65}
!65 = !{!"FuncMD", !66, !67}
!66 = !{!"FuncMDMap[0]", ptr @test_func}
!67 = !{!"FuncMDValue[0]", !121, !122, !123}
!121 = !{!"hasNonKernelArgLoad", i1 false}
!122 = !{!"hasNonKernelArgStore", i1 false}
!123 = !{!"hasNonKernelArgAtomic", i1 false}
!281 = !{ptr @test_func, !282}
!282 = !{!283}
!283 = !{!"function_type", i32 0}
