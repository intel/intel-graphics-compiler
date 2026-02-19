;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-buffer-bounds-checking -igc-add-implicit-args -igc-buffer-bounds-checking-patcher -S %s -o %t.ll
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

; CHECK:          [[BASE_ADDRESS:%[0-9]+]] = ptrtoint i32 addrspace(1)* %input to i64
; CHECK-NEXT:     [[ADDRESS:%[0-9]+]] = ptrtoint i32 addrspace(1)* %1 to i64
; CHECK-NEXT:     [[OFFSET:%[0-9]+]] = sub i64 [[ADDRESS]], [[BASE_ADDRESS]]
; CHECK-NEXT:     [[BUFFER_SIZE_EQ_ZERO:%[0-9]+]] = icmp eq i64 %bufferSize, 0
; CHECK-NEXT:     [[LOWER_BOUND_CHECK:%[0-9]+]] = icmp sge i64 [[OFFSET]], 0
; CHECK-NEXT:     [[UPPER_BOUND:%[0-9]+]] = sub i64 %bufferSize, 4
; CHECK-NEXT:     [[UPPER_BOUND_CHECK:%[0-9]+]] = icmp slt i64 [[OFFSET]], [[UPPER_BOUND]]
; CHECK-NEXT:     [[OFFSET_IN_BOUNDS:%[0-9]+]] = and i1 [[LOWER_BOUND_CHECK]], [[UPPER_BOUND_CHECK]]
; CHECK-NEXT:     [[CONDITION:%[0-9]+]] = or i1 [[BUFFER_SIZE_EQ_ZERO]], [[OFFSET_IN_BOUNDS]]
; CHECK-NEXT:     br i1 [[CONDITION]], label %bufferboundschecking.valid, label %bufferboundschecking.invalid

; CHECK:        bufferboundschecking.valid:
; CHECK-NEXT:     [[VALID:%[0-9]+]] = load i32, i32 addrspace(1)* {{%[0-9]+}}
; CHECK-NEXT:     br label %bufferboundschecking.end

; CHECK:        bufferboundschecking.invalid:
; CHECK-NEXT:     call spir_func void @__bufferoutofbounds_assert_

; CHECK:        bufferboundschecking.end:
; CHECK-NEXT:     [[LOADED_VALUE:%[0-9]+]] = phi i32 [ [[VALID]], %bufferboundschecking.valid ], [ 0, %bufferboundschecking.invalid ]
; CHECK-NEXT:     {{%[0-9]+}} = add i32 [[LOADED_VALUE]], 0

; CHECK:        declare void @__bufferoutofbounds
