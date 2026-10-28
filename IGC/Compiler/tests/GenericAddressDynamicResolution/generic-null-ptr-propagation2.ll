;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that null check is placed closer to use of alloca.
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -igc-generic-null-ptr-propagation | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -igc-generic-null-ptr-propagation | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}

; CHECK-LABEL: do_work:
; CHECK-TYPED-NEXT: [[CAST:%.*]] = addrspacecast i8 addrspace(3)* %local.ptr to i8 addrspace(4)*
; CHECK-OPAQUE-NEXT: [[CAST:%.*]] = addrspacecast ptr addrspace(3) %local.ptr to ptr addrspace(4)
; CHECK-TYPED-NEXT: [[PRED:%.*]] = icmp ne i8 addrspace(3)* %local.ptr, null
; CHECK-OPAQUE-NEXT: [[PRED:%.*]] = icmp ne ptr addrspace(3) %local.ptr, null
; CHECK-TYPED-NEXT: [[SEL:%.*]] = select i1 [[PRED]], i8 addrspace(4)* [[CAST]], i8 addrspace(4)* null
; CHECK-OPAQUE-NEXT: [[SEL:%.*]] = select i1 [[PRED]], ptr addrspace(4) [[CAST]], ptr addrspace(4) null

%"class.sycl::_V1::nd_item" = type { i8 }

define spir_func void @example_local_to_generic_triggers_pass(i1 %cond) {
entry:
  %local.ptr = addrspacecast i8* null to i8 addrspace(3)*
  %g = addrspacecast i8 addrspace(3)* %local.ptr to i8 addrspace(4)*
  br i1 %cond, label %do_work, label %exit

do_work:
  %isnull = icmp eq i8 addrspace(4)* %g, null
  br label %exit

exit:
  ret void
}