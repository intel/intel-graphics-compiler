;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that null check is placed closer to use of alloca even if we have multiple users.
; RUN: igc_opt --typed-pointers %s -S -igc-generic-null-ptr-propagation | FileCheck %s

; CHECK-LABEL: do_work:
; CHECK-NEXT: [[CAST:%.*]] = addrspacecast i8 addrspace(3)* %local.ptr to i8 addrspace(4)*
; CHECK-NEXT: [[PRED:%.*]] = icmp ne i8 addrspace(3)* %local.ptr, null
; CHECK-NEXT: [[SEL:%.*]] = select i1 [[PRED]], i8 addrspace(4)* [[CAST]], i8 addrspace(4)* null

%"class.sycl::_V1::nd_item" = type { i8 }

define spir_func void @example_local_to_generic_triggers_pass(i1 %cond1, i1 %cond2) {
entry:
  %local.ptr = addrspacecast i8* null to i8 addrspace(3)*
  %g = addrspacecast i8 addrspace(3)* %local.ptr to i8 addrspace(4)*
  br i1 %cond1, label %do_work, label %exit

do_work:
  br i1 %cond2, label %case1, label %case2

case1:
  %isnull1 = icmp eq i8 addrspace(4)* %g, null
  br label %exit

case2:
  %isnull2 = icmp eq i8 addrspace(4)* %g, null
  br label %exit

exit:
  ret void
}