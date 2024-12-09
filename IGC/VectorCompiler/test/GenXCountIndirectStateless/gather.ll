;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXCountIndirectStateless -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXCountIndirectStateless -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

declare <4 x i64> @llvm.masked.gather.v4i64.v4p1i64(<4 x i64 addrspace(1)*>, i32, <4 x i1>, <4 x i64>)

define spir_kernel void @kernel(i64 addrspace(1)* %pptr) #0 {
  %vpptr = getelementptr i64, i64 addrspace(1)* %pptr, <4 x i64> <i64 100, i64 200, i64 300, i64 400>
  %gather = call <4 x i64> @llvm.masked.gather.v4i64.v4p1i64(<4 x i64 addrspace(1)*> %vpptr, i32 8, <4 x i1> <i1 true, i1 true, i1 true, i1 true>, <4 x i64> undef)
  %extract = extractelement <4 x i64> %gather, i32 2
  %ptr = inttoptr i64 %extract to i32 addrspace(1)*
  %val = load i32, i32 addrspace(1)* %ptr
  ret void
}

attributes #0 = { nounwind "CMGenxMain" }

; CHECK: !genx.kernel.internal = !{[[KERNEL:![0-9]+]]}
!genx.kernels = !{!0}
!genx.kernel.internal = !{!4}

; CHECK-TYPED-PTRS: [[KERNEL]] = !{void (i64 addrspace(1)*)* @kernel, !{{[0-9]+}}, !{{[0-9]+}}, !{{[0-9]+}}, !{{[0-9]+}}, i32 1}
; CHECK-OPAQUE-PTRS: [[KERNEL]] = !{ptr @kernel, !{{[0-9]+}}, !{{[0-9]+}}, !{{[0-9]+}}, !{{[0-9]+}}, i32 1}
!0 = !{void (i64 addrspace(1)*)* @kernel, !"kernel", !1, i32 0, !2, !1, !3, i32 0}
!1 = !{i32 0}
!2 = !{i32 64}
!3 = !{!"svmptr_t", !"", !""}
!4 = !{void (i64 addrspace(1)*)* @kernel, !1, !5, !6, !7, i32 0}
!5 = !{i32 0}
!6 = !{}
!7 = !{i32 255}
