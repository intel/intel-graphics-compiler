;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-sub-group-func-resolution -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SubGroupFuncsResolution
; ------------------------------------------------
; This test checks that SubGroupFuncsResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __kernel void test_sg_max(global int *dst)
; {
;     int max_size = get_max_sub_group_size();
;     dst[0] = max_size;
; }
;
; ------------------------------------------------

; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 3
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_sg_max(i32 addrspace(1)* %dst) #0 {
; CHECK-LABEL: @test_sg_max(
; CHECK-NEXT:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[TMP0:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP0]], i64 0
; CHECK:    store i32 16, i32 addrspace(1)* [[ARRAYIDX]], align 4
; CHECK:    ret void
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  %call.i = call spir_func i32 @__builtin_IB_get_simd_size()
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 0
  store i32 %call.i, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent
declare spir_func i32 @__builtin_IB_get_simd_size() local_unnamed_addr #2

attributes #0 = { convergent noinline nounwind optnone }
attributes #2 = { convergent }

!IGCMetadata = !{!0}
!igc.functions = !{!3}

!0 = !{!"ModuleMD", !1}
!1 = !{!"msInfo", !2}
!2 = !{!"SubgroupSize", i32 16}
!3 = !{void (i32 addrspace(1)*)* @test_sg_max, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"sub_group_size", i32 16}
