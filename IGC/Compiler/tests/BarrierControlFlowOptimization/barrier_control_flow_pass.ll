;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --platformbmg --regkey EnableBarrierControlFlowOptimizationPass --igc-barrier-control-flow-optimization  -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; BarrierControlFlowOptimizationPass:
; ------------------------------------------------

; Checks barrier control flow optimization scenario
; before optimization( fence(ugm/tgm, scope local and op none), group sync(threadgroupbarrier) )
; after optimization ( fence(ugm/tgm, scope gpu and op evict), group sync(threadgroupbarrier) )
define void @test_ugm_tgm_thread_group() {
; CHECK-LABEL: @test_ugm_tgm_thread_group(
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-DAG: br
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 0, i32 3, i32 1)
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
; CHECK-DAG: br
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK: ret void
;
  call void @llvm.genx.GenISA.LSCFence(i32 0, i32 1, i32 0)
  call void @llvm.genx.GenISA.LSCFence(i32 2, i32 1, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  ret void
}

; Checks barrier control flow optimization scenario
; before optimization( fence(slm, scope group and op none)/fence((tgm, scope local and op none), group sync(threadgroupbarrier) )
; after optimization ( fence(slm, scope group and op none)/fence((tgm, scope gpu and op evict), group sync(threadgroupbarrier) )
define void @test_slm_tgm_thread_group() {
; CHECK-LABEL: @test_slm_tgm_thread_group(
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-DAG: br
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
; CHECK-DAG: br
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK: ret void
;
  call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
  call void @llvm.genx.GenISA.LSCFence(i32 2, i32 1, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  ret void
}

; Checks barrier control flow optimization scenario
; before optimization( fence(ugm, scope local and op none)/fence((tgm, scope local and op none)/group sync(threadgroupbarrier), fence(ugm, scope gpu and op evict)/fence((tgm, scope gpu and op evict) )
; after optimization ( group sync(threadgroupbarrier)/fence(ugm, scope gpu and op evict)/fence((tgm, scope gpu and op evict), group sync(threadgroupbarrier)/fence(ugm, scope gpu and op evict)/fence((tgm, scope gpu and op evict), )
define void @test_slm_tgm_bar_into_slm_tgm() {
; CHECK-LABEL: @test_slm_tgm_bar_into_slm_tgm(
; CHECK:  call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-DAG: br
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 0, i32 3, i32 1)
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
; CHECK-DAG: br
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-DAG: br
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 0, i32 3, i32 1)
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
; CHECK-DAG: br
; CHECK: ret void
  call void @llvm.genx.GenISA.LSCFence(i32 0, i32 1, i32 0)
  call void @llvm.genx.GenISA.LSCFence(i32 2, i32 1, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  br label %test2
test2:
  call void @llvm.genx.GenISA.LSCFence(i32 0, i32 3, i32 1)
  call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
  br label %finish
finish:
  ret void
}

; Checks barrier control flow optimization scenario
; before optimization( fence(slm, scope none and op none), group sync(threadgroupbarrier) )
; after optimization ( fence(slm, scope none and op none), group sync(threadgroupbarrier) )
define void @test_no_redundant_barrier() {
; CHECK-LABEL: @test_no_redundant_barrier(
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-DAG: br
; CHECK-DAG: br
; CHECK-NOT: call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-DAG: br
; CHECK: [[TMP10:%.*]] = and i32 3, 31
; CHECK: [[TMP11:%.*]] = shl nuw i32 1, [[TMP10]]
; CHECK: [[TMP12:%.*]] = inttoptr i32 16 to ptr addrspace(3)
; CHECK: [[TMP13:%.*]] = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(ptr addrspace(3) [[TMP12]], ptr addrspace(3) [[TMP12]], i32 [[TMP11]], i32 9)
; CHECK-DAG: br
; CHECK-NOT: call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-DAG: br
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 0, i32 3, i32 1)
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
; CHECK-DAG: br
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK: [[TMP17:%.*]] = load i32, ptr addrspace(3) [[TMP12]], align 16, !tbaa !0
; CHECK: [[TMP18:%.*]] = and i32 [[TMP17]], 31
; CHECK: ret void
;
  call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  br label %1
1:
  %2 = and i32 3, 31
  %3 = shl nuw i32 1, %2
  %4 = inttoptr i32 16 to i32 addrspace(3)*
  %5 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)* %4, i32 addrspace(3)* %4, i32 %3, i32 9)
  br label %6

6:
  call void @llvm.genx.GenISA.LSCFence(i32 0, i32 3, i32 1)
  call void @llvm.genx.GenISA.LSCFence(i32 2, i32 3, i32 1)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %7 = load i32, i32 addrspace(3)* %4, align 16, !tbaa !507
  %8 = and i32 %7, 31

  ret void
}

declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32)
declare void @llvm.genx.GenISA.threadgroupbarrier()
declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)*, i32 addrspace(3)*, i32, i32)

!507 = !{!508, !508, i64 0}
!508 = !{!"int", !509, i64 0}
!509 = !{!"omnipotent char", !510, i64 0}
!510 = !{!"Simple C/C++ TBAA"}
