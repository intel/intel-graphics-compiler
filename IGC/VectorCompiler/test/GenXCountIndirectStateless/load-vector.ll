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

define spir_kernel void @kernel(i64 addrspace(1)* %pptr, i64 %i, i64 %j) #0 {
  %pptri = getelementptr i64, i64 addrspace(1)* %pptr, i64 %i
  %pcast = bitcast i64 addrspace(1)* %pptri to <4 x i64> addrspace(1)*
  %vptr = load <4 x i64>, <4 x i64> addrspace(1)* %pcast
  %extract = extractelement <4 x i64> %vptr, i32 2
  %ptr = inttoptr i64 %extract to i32 addrspace(1)*
  %ptrj = getelementptr i32, i32 addrspace(1)* %ptr, i64 %j
  %val = load i32, i32 addrspace(1)* %ptrj
  ret void
}

attributes #0 = { nounwind "CMGenxMain" }

; CHECK: !genx.kernel.internal = !{[[KERNEL:![0-9]+]]}
!genx.kernels = !{!0}
!genx.kernel.internal = !{!4}

; CHECK-TYPED-PTRS: [[KERNEL]] = !{void (i64 addrspace(1)*, i64, i64)* @kernel, !{{[0-9]+}}, !{{[0-9]+}}, !{{[0-9]+}}, !{{[0-9]+}}, i32 1}
; CHECK-OPAQUE-PTRS: [[KERNEL]] = !{ptr @kernel, !{{[0-9]+}}, !{{[0-9]+}}, !{{[0-9]+}}, !{{[0-9]+}}, i32 1}
!0 = !{void (i64 addrspace(1)*, i64, i64)* @kernel, !"kernel", !1, i32 0, !2, !1, !3, i32 0}
!1 = !{i32 0, i32 0, i32 0}
!2 = !{i32 64, i32 72, i32 80}
!3 = !{!"svmptr_t", !"", !""}
!4 = !{void (i64 addrspace(1)*, i64, i64)* @kernel, !1, !5, !6, !7, i32 0}
!5 = !{i32 0, i32 1, i32 2}
!6 = !{}
!7 = !{i32 255, i32 -1, i32 -1}
