;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that null check is not moved before phi instruction creating invalid IR.
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -igc-generic-null-ptr-propagation | FileCheck %s

; CHECK-LABEL: exit:
; CHECK-NEXT: {{.*}} = phi i8 addrspace(4)* [ {{.*}}, %entry ], [ {{.*}}, %do_work ]

%"class.sycl::_V1::nd_item" = type { i8 }

define spir_func void @example_local_to_generic_triggers_pass(i1 %cond) {
entry:
  %local.ptr = addrspacecast i8* null to i8 addrspace(3)*
  %g = addrspacecast i8 addrspace(3)* %local.ptr to i8 addrspace(4)*
  br i1 %cond, label %do_work, label %exit

do_work:
  br label %exit

exit:
  ; %g has exactly one user: this PHI node.
  %g.phi = phi i8 addrspace(4)* [ %g, %entry ], [ %g, %do_work ]
  %isnull = icmp eq i8 addrspace(4)* %g.phi, null
  ret void
}