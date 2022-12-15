;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-poison-fp64-kernels -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PoisonFP64Kernels
; ------------------------------------------------

; Test checks that function that illegally uses fp64 instructions(has corresponding attr)
; is removed, and kernel has it's body substituted with a warning printf call.
;

; Check global with warning message
;
; CHECK: @[[POISON_MESSAGE_TEST_KERNEL:[a-zA-Z0-9_$"\\.-]+]] = internal unnamed_addr addrspace(2) constant [173 x i8] c"[CRITICAL ERROR] Kernel 'test_kernel' removed due to usage of FP64 instructions unsupported by the targeted hardware. Running this kernel may result in unexpected results.\0A\00"


define void @test_kernel(double %src, double* %dst) {
; CHECK: @test_kernel(
; CHECK-SAME: #[[ATTR:[0-9]*]]
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[GID0:%.*]] = call i32 (i32, ...) @__builtin_IB_get_group_id(i32 0)
; CHECK-NEXT:    [[GID1:%.*]] = call i32 (i32, ...) @__builtin_IB_get_group_id(i32 1)
; CHECK-NEXT:    [[GID2:%.*]] = call i32 (i32, ...) @__builtin_IB_get_group_id(i32 2)
; CHECK-NEXT:    [[LID0:%.*]] = call i32 (...) @__builtin_IB_get_local_id_x()
; CHECK-NEXT:    [[LID1:%.*]] = call i32 (...) @__builtin_IB_get_local_id_y()
; CHECK-NEXT:    [[LID2:%.*]] = call i32 (...) @__builtin_IB_get_local_id_z()
; CHECK-NEXT:    [[ID_MERGE:%.*]] = or i32 [[GID0]], [[GID1]]
; CHECK-NEXT:    [[ID_MERGE1:%.*]] = or i32 [[ID_MERGE]], [[GID2]]
; CHECK-NEXT:    [[ID_MERGE2:%.*]] = or i32 [[ID_MERGE1]], [[LID0]]
; CHECK-NEXT:    [[ID_MERGE3:%.*]] = or i32 [[ID_MERGE2]], [[LID1]]
; CHECK-NEXT:    [[ID_MERGE4:%.*]] = or i32 [[ID_MERGE3]], [[LID2]]
; CHECK-NEXT:    [[ID_IS_ZERO:%.*]] = icmp eq i32 [[ID_MERGE4]], 0
; CHECK-NEXT:    br i1 [[ID_IS_ZERO]], label %[[THEN:.*]], label %[[ELSE:.*]]
; CHECK:       [[THEN]]:
; CHECK-NEXT:    [[POSION_MESSAGE_GEP:%.*]] = getelementptr inbounds [173 x i8], [173 x i8] addrspace(2)* @poison.message.test_kernel, i32 0, i32 0
; CHECK-NEXT:    [[PRINTF_RESULT:%.*]] = call i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* [[POSION_MESSAGE_GEP]])
; CHECK-NEXT:    ret void
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    ret void
;
  %1 = call double @test_func(double %src)
  store double %1, double* %dst
  ret void
}

; Check that function is removed
;
; CHECK-NOT: define double @test_func

; Check that invalid_kernel attribute is added
;
; CHECK: attributes #[[ATTR]] = { "invalid_kernel("uses-fp64-math")" }

define double @test_func(double %src) #0 {
  %1 = fadd double %src, 2.0
  ret double %1
}

attributes #0 = { "uses-fp64-math" }
declare void @use.i64(i64)

!igc.functions = !{!0}

!0 = !{void (double, double*)* @test_kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{double (double)* @test_func, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 2}

