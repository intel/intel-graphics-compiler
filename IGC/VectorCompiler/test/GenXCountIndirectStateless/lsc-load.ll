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

declare <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v2i8.v1i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, <1 x i64>, i16, i32, <4 x i32>)

define spir_kernel void @kernel(i64 addrspace(1)* %pptr, i64 %i, i64 %j) #0 {
  %pptri = getelementptr i64, i64 addrspace(1)* %pptr, i64 %i
  %pcast = ptrtoint i64 addrspace(1)* %pptri to i64
  %pvcast = bitcast i64 %pcast to <1 x i64>
  %vptr = call <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v2i8.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <1 x i64> %pvcast, i16 0, i32 0, <4 x i32> undef)
  %cast = bitcast <4 x i32> %vptr to <2 x i64>
  %extract = extractelement <2 x i64> %cast, i32 1
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
