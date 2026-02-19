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
  %2 = load i32, i32 addrspace(1)* %1
  %3 = add i32 %2, 0
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
; CHECK-NEXT:     [[VALID:%[0-9]+]] = load i32, i32 addrspace(1)* [[POINTER]]
; CHECK-NEXT:     br label %minimumvalidaddresschecking.end

; CHECK:        minimumvalidaddresschecking.invalid:
; CHECK-NEXT:     call spir_func void @__minimumvalidaddresschecking_assert_

; CHECK:        minimumvalidaddresschecking.end:
; CHECK-NEXT:     [[LOADED_VALUE:%[0-9]+]] = phi i32 [ [[VALID]], %minimumvalidaddresschecking.valid ], [ 0, %minimumvalidaddresschecking.invalid ]
; CHECK-NEXT:     {{%[0-9]+}} = add i32 [[LOADED_VALUE]], 0

; CHECK:        declare void @__minimumvalidaddresschecking
