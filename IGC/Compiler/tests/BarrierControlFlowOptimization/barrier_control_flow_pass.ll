;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt -platformbmg --regkey EnableBarrierControlFlowOptimizationPass --igc-barrier-control-flow-optimization  -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; BarrierControlFlowOptimizationPass:
; ------------------------------------------------

; Checks barrier control flow optimization scenario
; before optimization( fence(ugm/tgm, scope local and op none), group sync(threadgroupbarrier) )
; after optimization ( fence(ugm/tgm, scope gpu and op evict), group sync(threadgroupbarrier) )
define void @test_ugm_tgm_thread_group() {
; CHECK-LABEL: @test_ugm_tgm_thread_group(
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

declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32)
declare void @llvm.genx.GenISA.threadgroupbarrier()
