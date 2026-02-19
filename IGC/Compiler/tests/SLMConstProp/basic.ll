;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-slmconstprop -S < %s | FileCheck %s
; ------------------------------------------------

define spir_kernel void @test_slmconst(float %a, float* %b) {
; CHECK-LABEL: @test_slmconst(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
; CHECK:    [[AA:%.*]] = mul i32 [[TMP0]], 4
; CHECK:    [[TMP1:%.*]] = inttoptr i32 [[AA]] to float addrspace(3)*
; CHECK:    store float 4.000000e+00, float addrspace(3)* [[TMP1]], align 4
; CHECK:    [[TMP2:%.*]] = add i32 [[AA]], 4
; CHECK:    [[TMP3:%.*]] = inttoptr i32 [[TMP2]] to float addrspace(3)*
; CHECK:    store float 4.000000e+00, float addrspace(3)* [[TMP3]], align 4
; CHECK:    [[TMP4:%.*]] = add i32 [[AA]], 8
; CHECK:    [[TMP5:%.*]] = inttoptr i32 [[TMP4]] to float addrspace(3)*
; CHECK:    store float 4.000000e+00, float addrspace(3)* [[TMP5]], align 4
; CHECK:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK:    [[ADD1:%.*]] = fadd float 4.000000e+00, 4.000000e+00
; CHECK:    store float [[ADD1]], float* [[B:%.*]], align 4
; CHECK:    ret void
;
entry:
  %0 = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %aa = mul i32 %0, 4
  %1 = inttoptr i32 %aa to float addrspace(3)*
  store float 4.000000e+00, float addrspace(3)* %1, align 4
  %2 = add i32 %aa, 4
  %3 = inttoptr i32 %2 to float addrspace(3)*
  store float 4.000000e+00, float addrspace(3)* %3, align 4
  %4 = add i32 %aa, 8
  %5 = inttoptr i32 %4 to float addrspace(3)*
  store float 4.000000e+00, float addrspace(3)* %5, align 4
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %load1 = load float, float addrspace(3)* %1, align 4
  %load2 = load float, float addrspace(3)* %3, align 4
  %add1 = fadd float %load1, %load2
  store float %add1, float* %b, align 4
  ret void
}

declare void @llvm.genx.GenISA.threadgroupbarrier()
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32)

!igc.functions = !{!0}

!0 = !{void (float, float*)* @test_slmconst, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
