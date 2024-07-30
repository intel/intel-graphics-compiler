;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; This is to test breaking constant struct that has ConstantExpr and ConstantVector.
; The test is derived from sycl test (multi_ptr_comparison_op).
;
;
; RUN: igc_opt -igc-break-const-expr -S %s | FileCheck %s
;=========================== begin_copyright_notice ============================
;
; CHECK-LABEL: define spir_kernel void @sycl_multi_ptr_comparison_op
; CHECK:       insertvalue %__StructSOALayout_ {{.*}}, 0, 0
; CHECK:       insertvalue %__StructSOALayout_ {{.*}}, 0, 1
; CHECK:       insertvalue %__StructSOALayout_ {{.*}}, 0, 2
; CHECK:       %[[#n2:]] = call i32 @llvm.genx.GenISA.bitcastfromstruct.i32.__StructSOALayout_(%__StructSOALayout_ %[[#n1:]])
; CHECK:       ret void

%half_impl = type { half }
%mptr_nullptr_result = type { i8, i8, i8, i8 }
%sycl_V1 = type { %sycl_array }
%sycl_array = type { [1 x i64] }
%__StructSOALayout_ = type <{ %__StructAOSLayout_ }>
%__StructAOSLayout_ = type <{ < 2 x i8 >, i8, i8 }>

@sycl_multi_ptr_comparison_op-ExtSLM = external addrspace(3) global [0 x i8]

; Function Attrs: convergent nounwind
define spir_kernel void @sycl_multi_ptr_comparison_op(%half_impl addrspace(3)*, %half_impl addrspace(1)* readonly, %sycl_V1*, %mptr_nullptr_result addrspace(1)*, %sycl_V1*, i64 %const_reg_qword, i64 %const_reg_qword1) {
  %6 = ptrtoint %mptr_nullptr_result addrspace(1)* %3 to i64
  %7 = shl i64 %const_reg_qword1, 2
  %8 = add i64 %7, %6
  %9 = call i32 @llvm.genx.GenISA.bitcastfromstruct.i32.__StructSOALayout_(%__StructSOALayout_ <{ %__StructAOSLayout_ <{ <2 x i8 > < i8 select (i1 icmp eq (%half_impl addrspace(4)* addrspacecast (%half_impl addrspace(3)* null to %half_impl addrspace(4)*), %half_impl addrspace(4)* null), i8 1, i8 0), i8 1 >, i8 select (i1 trunc (i8 or (i8 zext (i1 icmp ugt ([0 x i8] addrspace(3)* @sycl_multi_ptr_comparison_op-ExtSLM, [0 x i8] addrspace(3)* null) to i8), i8 zext (i1 icmp eq (i64 ptrtoint ([0 x i8] addrspace(3)* @sycl_multi_ptr_comparison_op-ExtSLM to i64), i64 ptrtoint (%half_impl addrspace(3)* addrspacecast (%half_impl addrspace(4)* null to %half_impl addrspace(3)*) to i64)) to i8)) to i1), i8 1, i8 0), i8 select (i1 icmp uge ([0 x i8] addrspace(3)* @sycl_multi_ptr_comparison_op-ExtSLM, [0 x i8] addrspace(3)* null), i8 1, i8 0) }> }>)
  %10 = inttoptr i64 %8 to i32 addrspace(1)*
  store i32 %9, i32 addrspace(1)* %10, align 1
  ret void
}

declare i32 @llvm.genx.GenISA.bitcastfromstruct.i32.__StructSOALayout_(%__StructSOALayout_)
