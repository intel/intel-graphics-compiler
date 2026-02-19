;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-minimum-valid-address-checking -igc-minimum-valid-address-checking_arg 0x100 -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_kernel void @kernel(i32 addrspace(1)* %input) nounwind {
  %1 = getelementptr inbounds i32, i32 addrspace(1)* %input, i64 2
  store i32 42, i32 addrspace(1)* %1
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*)* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

; CHECK:          [[ADDRESS:%[0-9]+]] = ptrtoint i32 addrspace(1)* [[POINTER:%[0-9]+]] to i64
; CHECK-NEXT:     [[CONDITION:%[0-9]+]] = icmp uge i64 [[ADDRESS]], {{[0-9]+}}
; CHECK-NEXT:     br i1 [[CONDITION]], label %minimumvalidaddresschecking.valid, label %minimumvalidaddresschecking.invalid

; CHECK:        minimumvalidaddresschecking.valid:
; CHECK-NEXT:     store i32 [[VALUE:[0-9]+]], i32 addrspace(1)* [[POINTER]]
; CHECK-NEXT:     br label %minimumvalidaddresschecking.end

; CHECK:        minimumvalidaddresschecking.invalid:
; CHECK:          call spir_func void @__minimumvalidaddresschecking_assert
; CHECK:          store i32 [[VALUE]], i32 addrspace(1)* null

; CHECK:        declare void @__minimumvalidaddresschecking
