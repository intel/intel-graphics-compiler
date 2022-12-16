;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -TrivialLocalMemoryOpsElimination -S < %s | FileCheck %s
; ------------------------------------------------
; TrivialLocalMemoryOpsElimination
; ------------------------------------------------
; This test checks that dead local load/stores and barriers
; are removed by this pass
; ------------------------------------------------

define spir_kernel void @test_load(i32 addrspace(3)* %b) {
; CHECK-LABEL: @test_load(
; CHECK-NEXT:    ret void
;
  %1 = load i32, i32 addrspace(3)* %b, align 4
  ret void
}

define spir_kernel void @test_store(i32 addrspace(3)* %b) {
; CHECK-LABEL: @test_store(
; CHECK-NEXT:    ret void
;
  store i32 13, i32 addrspace(3)* %b, align 4
  ret void
}

define spir_kernel void @test_barrier_remove(i32 addrspace(3)* %b) {
; CHECK-LABEL: @test_barrier_remove(
; CHECK-NEXT:    ret void
;
  store i32 14, i32 addrspace(3)* %b, align 4
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 true)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  ret void
}

define spir_kernel void @test_barrier_keep(i32 addrspace(3)* %b) {
; CHECK-LABEL: @test_barrier_keep(
; CHECK-NEXT:    call void @llvm.genx.GenISA.memoryfence(i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 true)
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    ret void
;
  store i32 15, i32 addrspace(3)* %b, align 4
  call void @llvm.genx.GenISA.memoryfence(i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 true)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  ret void
}

declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1)
declare void @llvm.genx.GenISA.threadgroupbarrier()


!igc.functions = !{!0, !3, !4, !5}

!0 = !{void (i32 addrspace(3)*)* @test_load, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i32 addrspace(3)*)* @test_store, !1}
!4 = !{void (i32 addrspace(3)*)* @test_barrier_remove, !1}
!5 = !{void (i32 addrspace(3)*)* @test_barrier_keep, !1}
